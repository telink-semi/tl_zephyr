/*
 * Copyright (c) 2026 Telink Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT telink_tlx_kscan

#include <zephyr/input/input.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/interrupt_controller/riscv_plic.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(input_tlx_kscan, CONFIG_INPUT_LOG_LEVEL);

#include <keyscan.h>

#define TLX_KSCAN_MAX_ROWS    8
#define TLX_KSCAN_MAX_COLUMNS 31

struct tlx_kscan_config {
	uintptr_t address;
	void (*irq_connect)(void);
	const uint8_t *pins_row;
	size_t pins_row_num;
	const uint8_t *pins_col;
	size_t pins_col_num;
	ks_col_pull_type_e col_pull_type;
	ks_debounce_period_e debounce_period;
	uint8_t idle_period;
	ks_scan_times_e scan_times;
};

struct tlx_kscan_data {
	uint8_t row_state[TLX_KSCAN_MAX_COLUMNS];
};

static void tlx_kscan_irq_handler(const struct device *dev)
{
	if (keyscan_get_irq_status()) {
		keyscan_clr_irq_status();
		const struct tlx_kscan_config *cfg = dev->config;
		struct tlx_kscan_data *data = dev->data;
		uint8_t current_row_state[cfg->pins_col_num];

		memset(current_row_state, 0, sizeof(current_row_state));
		for (;;) {
			uint8_t key = keyscan_get_ks_value();

			if (key == KESYCAN_END_FLAG) {
				break;
			}
			current_row_state[key & 0x1f] |= BIT(key >> 5);
		}
		for (uint8_t col = 0; col < cfg->pins_col_num; ++col) {
			uint8_t diff = data->row_state[col] ^ current_row_state[col];

			for (uint8_t row = 0; row < cfg->pins_row_num; ++row) {
				if (diff & BIT(row)) {
					uint16_t key_code = (uint16_t)row << 8 | col;

					if (data->row_state[col] & BIT(row)) {
						(void)input_report_key(dev, key_code, 0, true,
								       K_FOREVER);
						LOG_DBG("kscan released (%u %u)", row, col);
					} else {
						(void)input_report_key(dev, key_code, 1, true,
								       K_FOREVER);
						LOG_DBG("kscan pressed (%u %u)", row, col);
					}
				}
			}
			data->row_state[col] = current_row_state[col];
		}
	}
}

static int tlx_kscan_init(const struct device *dev)
{
	const struct tlx_kscan_config *cfg = dev->config;
	struct tlx_kscan_data *data = dev->data;
	int result = -ENXIO;

	if (cfg->address == (REG_RW_BASE_ADDR | REG_KEYSCAN_BASE)) {
		keyscan_set_martix((uint8_t *)cfg->pins_row, cfg->pins_row_num,
				   (uint8_t *)cfg->pins_col, cfg->pins_col_num, cfg->col_pull_type);
		keyscan_init(cfg->debounce_period, cfg->idle_period, cfg->scan_times);
		memset(data->row_state, 0, sizeof(data->row_state));
		keyscan_enable();
		cfg->irq_connect();
		result = 0;
		LOG_DBG("kscan inited");
	} else {
		LOG_ERR("kscan no device");
	}

	return result;
}

#define TLX_KSCAN_GET_INP(soc_pin)                                                                 \
	(((soc_pin) == GPIO_PD3)   ? (uint8_t)KS_PD3                                               \
	 : ((soc_pin) == GPIO_PD4) ? (uint8_t)KS_PD4                                               \
	 : ((soc_pin) == GPIO_PD5) ? (uint8_t)KS_PD5                                               \
	 : ((soc_pin) == GPIO_PD6) ? (uint8_t)KS_PD6                                               \
	 : ((soc_pin) == GPIO_PD7) ? (uint8_t)KS_PD7                                               \
	 : ((soc_pin) == GPIO_PE0) ? (uint8_t)KS_PE0                                               \
	 : ((soc_pin) == GPIO_PE1) ? (uint8_t)KS_PE1                                               \
	 : ((soc_pin) == GPIO_PE2) ? (uint8_t)KS_PE2                                               \
	 : ((soc_pin) == GPIO_PC3) ? (uint8_t)KS_PC3                                               \
	 : ((soc_pin) == GPIO_PC4) ? (uint8_t)KS_PC4                                               \
	 : ((soc_pin) == GPIO_PC5) ? (uint8_t)KS_PC5                                               \
	 : ((soc_pin) == GPIO_PC6) ? (uint8_t)KS_PC6                                               \
	 : ((soc_pin) == GPIO_PC7) ? (uint8_t)KS_PC7                                               \
	 : ((soc_pin) == GPIO_PD0) ? (uint8_t)KS_PD0                                               \
	 : ((soc_pin) == GPIO_PD1) ? (uint8_t)KS_PD1                                               \
	 : ((soc_pin) == GPIO_PD2) ? (uint8_t)KS_PD2                                               \
	 : ((soc_pin) == GPIO_PB3) ? (uint8_t)KS_PB3                                               \
	 : ((soc_pin) == GPIO_PB7) ? (uint8_t)KS_PB7                                               \
	 : ((soc_pin) == GPIO_PC0) ? (uint8_t)KS_PC0                                               \
	 : ((soc_pin) == GPIO_PC1) ? (uint8_t)KS_PC1                                               \
	 : ((soc_pin) == GPIO_PC2) ? (uint8_t)KS_PC2                                               \
	 : ((soc_pin) == GPIO_PA0) ? (uint8_t)KS_PA0                                               \
	 : ((soc_pin) == GPIO_PA1) ? (uint8_t)KS_PA1                                               \
	 : ((soc_pin) == GPIO_PA2) ? (uint8_t)KS_PA1                                               \
	 : ((soc_pin) == GPIO_PA3) ? (uint8_t)KS_PA3                                               \
	 : ((soc_pin) == GPIO_PA4) ? (uint8_t)KS_PA4                                               \
	 : ((soc_pin) == GPIO_PB0) ? (uint8_t)KS_PB0                                               \
	 : ((soc_pin) == GPIO_PB1) ? (uint8_t)KS_PB1                                               \
	 : ((soc_pin) == GPIO_PH3) ? (uint8_t)KS_PH3                                               \
	 : ((soc_pin) == GPIO_PH4) ? (uint8_t)KS_PH4                                               \
	 : ((soc_pin) == GPIO_PH5) ? (uint8_t)KS_PH5                                               \
	 : ((soc_pin) == GPIO_PH7) ? (uint8_t)KS_PH7                                               \
	 : ((soc_pin) == GPIO_PG3) ? (uint8_t)KS_PG3                                               \
	 : ((soc_pin) == GPIO_PG4) ? (uint8_t)KS_PG4                                               \
	 : ((soc_pin) == GPIO_PG5) ? (uint8_t)KS_PG5                                               \
	 : ((soc_pin) == GPIO_PG6) ? (uint8_t)KS_PG6                                               \
	 : ((soc_pin) == GPIO_PG7) ? (uint8_t)KS_PG7                                               \
	 : ((soc_pin) == GPIO_PH0) ? (uint8_t)KS_PH0                                               \
	 : ((soc_pin) == GPIO_PH1) ? (uint8_t)KS_PH1                                               \
	 : ((soc_pin) == GPIO_PH2) ? (uint8_t)KS_PH2                                               \
	 : ((soc_pin) == GPIO_PF3) ? (uint8_t)KS_PF3                                               \
	 : ((soc_pin) == GPIO_PF4) ? (uint8_t)KS_PF4                                               \
	 : ((soc_pin) == GPIO_PF5) ? (uint8_t)KS_PF5                                               \
	 : ((soc_pin) == GPIO_PF6) ? (uint8_t)KS_PF6                                               \
	 : ((soc_pin) == GPIO_PF7) ? (uint8_t)KS_PF7                                               \
	 : ((soc_pin) == GPIO_PG0) ? (uint8_t)KS_PG0                                               \
	 : ((soc_pin) == GPIO_PG1) ? (uint8_t)KS_PG1                                               \
	 : ((soc_pin) == GPIO_PG2) ? (uint8_t)KS_PG2                                               \
	 : ((soc_pin) == GPIO_PE3) ? (uint8_t)KS_PE3                                               \
	 : ((soc_pin) == GPIO_PE4) ? (uint8_t)KS_PE4                                               \
	 : ((soc_pin) == GPIO_PE5) ? (uint8_t)KS_PE5                                               \
	 : ((soc_pin) == GPIO_PE6) ? (uint8_t)KS_PE6                                               \
	 : ((soc_pin) == GPIO_PE7) ? (uint8_t)KS_PE7                                               \
	 : ((soc_pin) == GPIO_PF0) ? (uint8_t)KS_PF0                                               \
	 : ((soc_pin) == GPIO_PF1) ? (uint8_t)KS_PF1                                               \
				   : (1 / 0) /* kscan invalid pin assigned! */                     \
	)

#define TLX_KSCAN_GET_INP_AT_ID(node_id, prop, idx)                                                \
	TLX_KSCAN_GET_INP(TLX_PINMUX_GET_PIN(DT_PROP(DT_PROP_BY_IDX(node_id, prop, idx), pinmux)))

#define TLX_KSCAN_GET_INPS(node_id, prop)                                                          \
	{DT_FOREACH_PROP_ELEM_SEP(node_id, prop, TLX_KSCAN_GET_INP_AT_ID, (,)) }

#define TLX_KSCAN_INIT(i)                                                                          \
                                                                                                   \
	BUILD_ASSERT((DT_PROP_LEN(DT_DRV_INST(i), pinctrl_row) >= 1) &&                            \
		     (DT_PROP_LEN(DT_DRV_INST(i), pinctrl_row) <= TLX_KSCAN_MAX_ROWS));            \
	BUILD_ASSERT((DT_PROP_LEN(DT_DRV_INST(i), pinctrl_col) >= 1) &&                            \
		     (DT_PROP_LEN(DT_DRV_INST(i), pinctrl_col) <= TLX_KSCAN_MAX_COLUMNS));         \
                                                                                                   \
	static void tlx_kscan_irq_connect_##i(void);                                               \
                                                                                                   \
	static const uint8_t pins_row##i[] = TLX_KSCAN_GET_INPS(DT_DRV_INST(i), pinctrl_row);      \
	static const uint8_t pins_col##i[] = TLX_KSCAN_GET_INPS(DT_DRV_INST(i), pinctrl_col);      \
                                                                                                   \
	static const struct tlx_kscan_config tlx_kscan_config_##i = {                              \
		.address = DT_INST_REG_ADDR(i),                                                    \
		.irq_connect = tlx_kscan_irq_connect_##i,                                          \
		.pins_row = pins_row##i,                                                           \
		.pins_row_num = ARRAY_SIZE(pins_row##i),                                           \
		.pins_col = pins_col##i,                                                           \
		.pins_col_num = ARRAY_SIZE(pins_col##i),                                           \
		.col_pull_type = DT_INST_PROP(i, col_pull_type),                                   \
		.debounce_period = DT_INST_PROP(i, debounce_period),                               \
		.idle_period = DT_INST_PROP(i, idle_period),                                       \
		.scan_times = DT_INST_PROP(i, scan_times),                                         \
	};                                                                                         \
                                                                                                   \
	static struct tlx_kscan_data tlx_kscan_data_##i;                                           \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(i, &tlx_kscan_init, NULL, &tlx_kscan_data_##i,                       \
			      &tlx_kscan_config_##i, POST_KERNEL, CONFIG_INPUT_INIT_PRIORITY,      \
			      NULL);                                                               \
                                                                                                   \
	static void tlx_kscan_irq_connect_##i(void)                                                \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQN(i), DT_INST_IRQ(i, priority), tlx_kscan_irq_handler,      \
			    DEVICE_DT_INST_GET(i), 0);                                             \
		riscv_plic_irq_enable(DT_INST_IRQN(i));                                            \
		riscv_plic_set_priority(DT_INST_IRQN(i), DT_INST_IRQ(i, priority));                \
	}

DT_INST_FOREACH_STATUS_OKAY(TLX_KSCAN_INIT)
