/*
 * Copyright (c) 2026 Telink Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT telink_tlx_kscan

#include <zephyr/input/input.h>
#include <zephyr/drivers/pinctrl.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(input_tlx_kscan, CONFIG_INPUT_LOG_LEVEL);

#include <keyscan.h>

#define TYPE_COMPAT(a, b)                                                                          \
	__builtin_types_compatible_p(__typeof__(a), __typeof__(b)) ||                              \
		__builtin_types_compatible_p(const __typeof__(a), __typeof__(b)) ||                \
		__builtin_types_compatible_p(__typeof__(a), const __typeof__(b))

#define ARRAY_REMAP(arr_inp, arr_out, val)                                                         \
	({                                                                                         \
		BUILD_ASSERT(ARRAY_SIZE(arr_inp) == ARRAY_SIZE(arr_out));                          \
		BUILD_ASSERT(TYPE_COMPAT((arr_inp)[0], val));                                      \
		static const __typeof__((arr_out)[0]) _rom_arr_out[] = arr_out;                    \
		const __typeof__((arr_out)[0]) *_out = NULL;                                       \
                                                                                                   \
		for (size_t _i = 0; _i < ARRAY_SIZE(arr_inp); ++_i) {                              \
			if ((arr_inp)[_i] == (val)) {                                              \
				_out = &_rom_arr_out[_i];                                          \
				break;                                                             \
			}                                                                          \
		}                                                                                  \
		_out;                                                                              \
	})

struct tlx_kscan_config {
	uintptr_t address;
	void (*irq_connect)(void);
	const pinctrl_soc_pin_t *pins_row;
	size_t pins_row_num;
	const pinctrl_soc_pin_t *pins_col;
	size_t pins_col_num;
	ks_col_pull_type_e col_pull_type;
	ks_debounce_period_e debounce_period;
	uint8_t idle_period;
	ks_scan_times_e scan_times;
};

struct tlx_kscan_data {
	uint8_t *row_states;
};

static const ks_value_e *tlx_kscan_get_input(pinctrl_soc_pin_t pin)
{
	return ARRAY_REMAP(
		((pinctrl_soc_pin_t[]){
			GPIO_PD3, GPIO_PD4, GPIO_PD5, GPIO_PD6, GPIO_PD7, GPIO_PE0, GPIO_PE1,
			GPIO_PE2, GPIO_PC3, GPIO_PC4, GPIO_PC5, GPIO_PC6, GPIO_PC7, GPIO_PD0,
			GPIO_PD1, GPIO_PD2, GPIO_PB3, GPIO_PB7, GPIO_PC0, GPIO_PC1, GPIO_PC2,
			GPIO_PA0, GPIO_PA1, GPIO_PA2, GPIO_PA3, GPIO_PA4, GPIO_PB0, GPIO_PB1,
			GPIO_PH3, GPIO_PH4, GPIO_PH5, GPIO_PH7, GPIO_PG3, GPIO_PG4, GPIO_PG5,
			GPIO_PG6, GPIO_PG7, GPIO_PH0, GPIO_PH1, GPIO_PH2, GPIO_PF3, GPIO_PF4,
			GPIO_PF5, GPIO_PF6, GPIO_PF7, GPIO_PG0, GPIO_PG1, GPIO_PG2, GPIO_PE3,
			GPIO_PE4, GPIO_PE5, GPIO_PE6, GPIO_PE7, GPIO_PF0, GPIO_PF1}),
		((ks_value_e[]){KS_PD3, KS_PD4, KS_PD5, KS_PD6, KS_PD7, KS_PE0, KS_PE1, KS_PE2,
				KS_PC3, KS_PC4, KS_PC5, KS_PC6, KS_PC7, KS_PD0, KS_PD1, KS_PD2,
				KS_PB3, KS_PB7, KS_PC0, KS_PC1, KS_PC2, KS_PA0, KS_PA1, KS_PA2,
				KS_PA3, KS_PA4, KS_PB0, KS_PB1, KS_PH3, KS_PH4, KS_PH5, KS_PH7,
				KS_PG3, KS_PG4, KS_PG5, KS_PG6, KS_PG7, KS_PH0, KS_PH1, KS_PH2,
				KS_PF3, KS_PF4, KS_PF5, KS_PF6, KS_PF7, KS_PG0, KS_PG1, KS_PG2,
				KS_PE3, KS_PE4, KS_PE5, KS_PE6, KS_PE7, KS_PF0, KS_PF1}),
		pin);
}

static void tlx_kscan_irq_handler(const struct device *dev)
{
	if (keyscan_get_irq_status()) {
		keyscan_clr_irq_status();
		struct tlx_kscan_data *data = dev->data;
		const struct tlx_kscan_config *cfg = dev->config;
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
			uint8_t diff = data->row_states[col] ^ current_row_state[col];

			for (uint8_t row = 0; row < 8; ++row) {
				if (diff & BIT(row)) {
					uint16_t key_code = (uint16_t)row << 8 | col;

					if (data->row_states[col] & BIT(row)) {
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
			data->row_states[col] = current_row_state[col];
		}
	}
}

static int tlx_kscan_init(const struct device *dev)
{
	const struct tlx_kscan_config *cfg = dev->config;
	struct tlx_kscan_data *data = dev->data;
	int result = -ENXIO;

	if (cfg->address == (REG_RW_BASE_ADDR | REG_KEYSCAN_BASE)) {
		uint8_t row[cfg->pins_row_num], col[cfg->pins_col_num];

		result = 0;
		for (size_t i = 0; !result && i < ARRAY_SIZE(row); ++i) {
			const ks_value_e *ks =
				tlx_kscan_get_input(TLX_PINMUX_GET_PIN(cfg->pins_row[i]));

			if (ks) {
				row[i] = *ks;
			} else {
				result = -EINVAL;
			}
		}
		for (size_t i = 0; !result && i < ARRAY_SIZE(col); ++i) {
			const ks_value_e *ks =
				tlx_kscan_get_input(TLX_PINMUX_GET_PIN(cfg->pins_col[i]));

			if (ks) {
				col[i] = *ks;
			} else {
				result = -EINVAL;
			}
		}
		if (!result) {
			keyscan_set_martix(row, ARRAY_SIZE(row), col, ARRAY_SIZE(col),
					   cfg->col_pull_type);
			keyscan_init(cfg->debounce_period, cfg->idle_period, cfg->scan_times);
			memset(data->row_states, 0, cfg->pins_col_num);
			keyscan_enable();
			cfg->irq_connect();
			LOG_DBG("kscan inited");
		} else {
			LOG_ERR("kscan invalid input pin(s)");
		}
	} else {
		LOG_ERR("kscan no device");
	}

	return result;
}

#define TLX_KSCAN_INIT(i)                                                                          \
                                                                                                   \
	BUILD_ASSERT(DT_PROP_LEN(DT_DRV_INST(i), pinctrl_row) <= 8);                               \
	BUILD_ASSERT(DT_PROP_LEN(DT_DRV_INST(i), pinctrl_col) <= 32);                              \
                                                                                                   \
	static void tlx_kscan_irq_connect_##i(void);                                               \
                                                                                                   \
	static const pinctrl_soc_pin_t pins_row##i[] =                                             \
		Z_PINCTRL_STATE_PINS_INIT(DT_DRV_INST(i), pinctrl_row);                            \
	static const pinctrl_soc_pin_t pins_col##i[] =                                             \
		Z_PINCTRL_STATE_PINS_INIT(DT_DRV_INST(i), pinctrl_col);                            \
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
	static uint8_t row_state##i[ARRAY_SIZE(pins_col##i)];                                      \
                                                                                                   \
	static struct tlx_kscan_data tlx_kscan_data_##i = {                                        \
		.row_states = row_state##i,                                                        \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(i, &tlx_kscan_init, NULL, &tlx_kscan_data_##i,                       \
			      &tlx_kscan_config_##i, POST_KERNEL, CONFIG_INPUT_INIT_PRIORITY,      \
			      NULL);                                                               \
                                                                                                   \
	static void tlx_kscan_irq_connect_##i(void)                                                \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQN(i), DT_INST_IRQ(i, priority), tlx_kscan_irq_handler,      \
			    DEVICE_DT_INST_GET(i), 0);                                             \
		riscv_plic_irq_enable(DT_INST_IRQN(i) - CONFIG_2ND_LVL_ISR_TBL_OFFSET);            \
		riscv_plic_set_priority(DT_INST_IRQN(i) - CONFIG_2ND_LVL_ISR_TBL_OFFSET,           \
					DT_INST_IRQ(i, priority));                                 \
	}

DT_INST_FOREACH_STATUS_OKAY(TLX_KSCAN_INIT)
