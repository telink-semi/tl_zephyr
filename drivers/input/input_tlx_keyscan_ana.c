/*
 * Copyright (c) 2026 Telink Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT telink_tlx_kscan_ana

#include <zephyr/input/input.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/interrupt_controller/riscv_plic.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(input_tlx_kscan_ana, CONFIG_INPUT_LOG_LEVEL);

#if CONFIG_INPUT_LOG_LEVEL == LOG_LEVEL_DBG
#define TLX_KSCAN_ANA_LOG_RAW_DBG(...) LOG_PRINTK(__VA_ARGS__)
#else
#define TLX_KSCAN_ANA_LOG_RAW_DBG(...)
#endif

#include <keyscan_ana.h>
#include <clock.h>

#define TLX_KSCAN_ANA_ADC_BUFFER_SIZE 64
#define TLX_KSCAN_ANA_REQUIRED_PCLK   192

struct tlx_kscan_ana_config {
	uintptr_t address;
	void (*irq_connect)(void);
	ks_ana_gpio_pin_t ks_ana_gpio_pin;
	ks_ana_threshold_t ks_ana_threshold;
	uint8_t hall_init_time;
};

struct tlx_kscan_ana_data {
	int16_t adc_buf[2][TLX_KSCAN_ANA_ADC_BUFFER_SIZE] __aligned(4);
	ks_ana_value_t ks_ana_value;
	struct k_work_delayable work;
	const struct device *const dev;
};

static void tlx_kscan_ana_get_key(const struct device *dev)
{
	const struct tlx_kscan_ana_config *cfg = dev->config;
	struct tlx_kscan_ana_data *data = dev->data;

	TLX_KSCAN_ANA_LOG_RAW_DBG("kscan_ana adc\n");
	for (uint8_t row = 0; row < KS_FIXED_ROWS; ++row) {
		data->ks_ana_value.now_key_bits[row] = data->ks_ana_value.last_key_bits[row];
		for (uint8_t col = 0; col < KS_FIXED_COLS; ++col) {
			int16_t adc_value =
				((int16_t *)data->adc_buf)[(size_t)row * KS_FIXED_COLS + col];

			TLX_KSCAN_ANA_LOG_RAW_DBG(" %d", adc_value);
			if (data->ks_ana_value.now_key_bits[row] & (1 << col)) {
				/* was pressed */
				if (adc_value < cfg->ks_ana_threshold.release_threshold) {
					/* now released */
					data->ks_ana_value.now_key_bits[row] &= ~(1 << col);
					/* report released key */
					(void)input_report_key(dev, ((uint16_t)row << 8) | col, 0,
							       true, K_FOREVER);
				}
			} else {
				/* was released */
				if (adc_value > cfg->ks_ana_threshold.press_threshold) {
					/* now pressed */
					data->ks_ana_value.now_key_bits[row] |= (1 << col);
					(void)input_report_key(dev, ((uint16_t)row << 8) | col, 1,
							       true, K_FOREVER);
				}
			}
		}
		data->ks_ana_value.last_key_bits[row] = data->ks_ana_value.now_key_bits[row];
		TLX_KSCAN_ANA_LOG_RAW_DBG("\n");
	}
}

#if CONFIG_INPUT_TELINK_TLX_KSCAN_ANA_DMA_LLP_ENABLE
static void tlx_kscan_ana_work(struct k_work *item)
{
	struct tlx_kscan_ana_data *data =
		CONTAINER_OF(k_work_delayable_from_work(item), struct tlx_kscan_ana_data, work);

	(void)k_work_schedule(&data->work,
			      K_MSEC(CONFIG_INPUT_TELINK_TLX_KSCAN_ANA_SCAN_PERIOD_MS));
	tlx_kscan_ana_get_key(data->dev);
}
#endif /* CONFIG_INPUT_TELINK_TLX_KSCAN_ANA_DMA_LLP_ENABLE */

static void tlx_kscan_ana_irq_handler(const struct device *dev)
{
	const struct tlx_kscan_ana_config *cfg = dev->config;
	struct tlx_kscan_ana_data *data = dev->data;

	if (cfg->address == (REG_RW_BASE_ADDR | REG_KEYSCAN_BASE)) {
		uint8_t irq_status = ks_ana_get_irq_status();

		if (irq_status & (FLD_KS_FRM_END_STA | FLD_KS_FRM_END1_STA)) {
			ks_ana_clr_irq_status(irq_status);
			tlx_kscan_ana_get_key(dev);
			ks_ana_set_rx_dma_config(ADC0, DMA0);
			ks_ana_receive_dma(ADC0, DMA0, (uint16_t *)data->adc_buf[0],
					   sizeof(data->adc_buf[0]));
			ks_ana_set_rx_dma_config(ADC1, DMA1);
			ks_ana_receive_dma(ADC1, DMA1, (uint16_t *)data->adc_buf[1],
					   sizeof(data->adc_buf[1]));
		} else {
			LOG_ERR("kscan_ana unhanded interrupt %x", irq_status);
		}
	}
}

static int tlx_kscan_ana_init(const struct device *dev)
{
	const struct tlx_kscan_ana_config *cfg = dev->config;
	struct tlx_kscan_ana_data *data = dev->data;

	if (sys_clk.pll_clk != TLX_KSCAN_ANA_REQUIRED_PCLK) {
		LOG_ERR("kscan_ana invalid PCLK");
		return -EIO;
	}
	if (cfg->address != (REG_RW_BASE_ADDR | REG_KEYSCAN_BASE)) {
		LOG_ERR("kscan_ana no device");
		return -ENXIO;
	}
	memset(data->adc_buf, 0, sizeof(data->adc_buf));
	memset(&data->ks_ana_value, 0, sizeof(data->ks_ana_value));
#if CONFIG_INPUT_TELINK_TLX_KSCAN_ANA_DMA_LLP_ENABLE
	ks_ana_rx_dma_chain_init(ADC0, DMA0, (uint16_t *)data->adc_buf[0],
				 sizeof(data->adc_buf[0]));
	ks_ana_rx_dma_chain_init(ADC1, DMA1, (uint16_t *)data->adc_buf[1],
				 sizeof(data->adc_buf[1]));
#else
	ks_ana_set_rx_dma_config(ADC0, DMA0);
	ks_ana_receive_dma(ADC0, DMA0, (uint16_t *)data->adc_buf[0], sizeof(data->adc_buf[0]));
	ks_ana_set_rx_dma_config(ADC1, DMA1);
	ks_ana_receive_dma(ADC1, DMA1, (uint16_t *)data->adc_buf[1], sizeof(data->adc_buf[1]));
#endif /* CONFIG_INPUT_TELINK_TLX_KSCAN_ANA_DMA_LLP_ENABLE */
	ks_ana_init(cfg->ks_ana_gpio_pin, KEYSCAN_2XADC_8K_ONCE_MODE, cfg->hall_init_time,
		    cfg->ks_ana_threshold);
#if CONFIG_INPUT_TELINK_TLX_KSCAN_ANA_DMA_LLP_ENABLE
	k_work_init_delayable(&data->work, tlx_kscan_ana_work);
	while (k_work_schedule(&data->work, K_NO_WAIT) == -EBUSY) {
		k_usleep(10); /* Let other stuffs run */
	}
#else
	cfg->irq_connect();
#endif /* CONFIG_INPUT_TELINK_TLX_KSCAN_ANA_DMA_LLP_ENABLE */
	LOG_DBG("kscan_ana inited");
	return 0;
}

#define TLX_KSCAN_ANA_GET_PIN(node_id, prop)                                                       \
	(TLX_PINMUX_GET_PIN(DT_PROP(DT_PROP(node_id, prop), pinmux)))

#define TLX_KSCAN_ANA_GET_PIN_AT_ID(node_id, prop, idx)                                            \
	(TLX_PINMUX_GET_PIN(DT_PROP(DT_PROP_BY_IDX(node_id, prop, idx), pinmux)))

#define TLX_KSCAN_ANA_GET_PINS(node_id, prop)                                                      \
	{DT_FOREACH_PROP_ELEM_SEP(node_id, prop, TLX_KSCAN_ANA_GET_PIN_AT_ID, (,)) }

#define TLX_KSCAN_ANA_INIT(i)                                                                      \
                                                                                                   \
	BUILD_ASSERT(DT_PROP_LEN(DT_DRV_INST(i), switch_channel_pin) ==                            \
		     ANA_SWITCH_CHANNEL_NUMBER);                                                   \
	BUILD_ASSERT(DT_PROP_LEN(DT_DRV_INST(i), hall_power_pin) == HALL_ROWS_POWER_NUMBER);       \
	BUILD_ASSERT(DT_INST_PROP(i, release_threshold) < DT_INST_PROP(i, press_threshold));       \
                                                                                                   \
	static void tlx_kscan_ana_irq_connect_##i(void);                                           \
                                                                                                   \
	static const struct tlx_kscan_ana_config tlx_kscan_ana_config_##i = {                      \
		.address = DT_INST_REG_ADDR(i),                                                    \
		.irq_connect = tlx_kscan_ana_irq_connect_##i,                                      \
		.ks_ana_gpio_pin =                                                                 \
			{                                                                          \
				.pin_group_id = DT_INST_PROP(i, pin_group),                        \
				.ana_switch_enable_pin =                                           \
					TLX_KSCAN_ANA_GET_PIN(DT_DRV_INST(i), switch_enable_pin),  \
				.ana_switch_channel_pin = TLX_KSCAN_ANA_GET_PINS(                  \
					DT_DRV_INST(i), switch_channel_pin),                       \
				.hall_rows_power_pin =                                             \
					TLX_KSCAN_ANA_GET_PINS(DT_DRV_INST(i), hall_power_pin),    \
			},                                                                         \
		.ks_ana_threshold =                                                                \
			{                                                                          \
				.release_threshold = DT_INST_PROP(i, release_threshold),           \
				.press_threshold = DT_INST_PROP(i, press_threshold),               \
			},                                                                         \
		.hall_init_time = DT_INST_PROP(i, hall_init_time),                                 \
	};                                                                                         \
                                                                                                   \
	static struct tlx_kscan_ana_data tlx_kscan_ana_data_##i = {                                \
		.dev = DEVICE_DT_INST_GET(i),                                                      \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(i, &tlx_kscan_ana_init, NULL, &tlx_kscan_ana_data_##i,               \
			      &tlx_kscan_ana_config_##i, POST_KERNEL, CONFIG_INPUT_INIT_PRIORITY,  \
			      NULL);                                                               \
                                                                                                   \
	static void tlx_kscan_ana_irq_connect_##i(void)                                            \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQN(i), DT_INST_IRQ(i, priority), tlx_kscan_ana_irq_handler,  \
			    DEVICE_DT_INST_GET(i), 0);                                             \
		riscv_plic_irq_enable(DT_INST_IRQN(i));                                            \
		riscv_plic_set_priority(DT_INST_IRQN(i), DT_INST_IRQ(i, priority));                \
	}

DT_INST_FOREACH_STATUS_OKAY(TLX_KSCAN_ANA_INIT)
