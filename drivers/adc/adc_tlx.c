/*
 * Copyright (c) 2022-2026 Telink Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT telink_tlx_adc

#include <zephyr/drivers/adc.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/dt-bindings/adc/tlx-adc.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(adc_tlx, CONFIG_ADC_TELINK_TLX_LOG_LEVEL);

#include <adc.h>

/************************************************************************
 * Helpers
 ************************************************************************/

#define ARRAY_CONTAINS(arr, val)                            \
	({                                                      \
		bool _found = false;                                \
		for (size_t _i = 0; _i < ARRAY_SIZE(arr); _i++) {   \
			if ((arr)[_i] == (val)) {                       \
				_found = true;                              \
				break;                                      \
			}                                               \
		}                                                   \
		_found;                                             \
	})

/************************************************************************
 * ADC driver data types
 ************************************************************************/

struct telink_tlx_adc_data {
	struct {
		bool valid;
		enum adc_gain gain;
		uint16_t acquisition_time;
		uint8_t input_positive;
		uint8_t input_negative;
	} channel[32];
};

struct telink_tlx_adc_config {
	struct k_msgq *queue;
	uintptr_t address;
	const struct pinctrl_dev_config *pcfg;
};

/************************************************************************
 * ADC low-level wrappers
 ************************************************************************/

static inline bool telink_tlx_adc_hw_valid(uintptr_t base_addr)
{
	bool result = false;

	if (base_addr == (REG_RW_BASE_ADDR | ADC_BASE_ADDR)) {
		result = true;
	}
	return result;
}

static inline int telink_tlx_adc_hw_init(uintptr_t base_addr)
{
	ARG_UNUSED(base_addr);

	adc_init(NDMA_M_CHN);
	return 0;
}

/************************************************************************
 * ADC APIs
 ************************************************************************/

static int telink_tlx_adc_init(const struct device *dev)
{
	int result = 0;

	do {
		const struct telink_tlx_adc_config *config = dev->config;

		result = pinctrl_apply_state(config->pcfg, PINCTRL_STATE_DEFAULT);
		if (result) {
			LOG_ERR("adc pinctrl failed %d", result);
			break;
		}
		if (!telink_tlx_adc_hw_valid(config->address)) {
			LOG_ERR("adc no low-level hal");
			result = -ENXIO;
			break;
		}
		result = telink_tlx_adc_hw_init(config->address);
		if (result) {
			LOG_ERR("adc init failed %d", result);
			break;
		}

		struct telink_tlx_adc_data *data = dev->data;

		for (size_t i = 0; i < ARRAY_SIZE(data->channel); ++i) {
			data->channel[i].valid = false;
		}
		LOG_INF("adc inited: %s", dev->name);
	} while (0);
	return result;
}

static int telink_tlx_adc_channel_setup(const struct device *dev,
				 const struct adc_channel_cfg *channel_cfg)
{
	struct telink_tlx_adc_data *data = dev->data;
	int result = -EINVAL;

	do {
		if (!ARRAY_CONTAINS(((enum adc_gain[]){ADC_GAIN_1_4, ADC_GAIN_1_2, ADC_GAIN_1}),
			channel_cfg->gain)) {
			LOG_ERR("adc not supported gain: %u", channel_cfg->gain);
			break;
		}
		if (!ARRAY_CONTAINS(((enum adc_reference[]){ADC_REF_INTERNAL}),
			channel_cfg->reference)) {
			LOG_ERR("adc not supported reference: %u", channel_cfg->reference);
			break;
		}
		if (!channel_cfg->differential) {
			LOG_ERR("adc not supported input type: %u", channel_cfg->differential);
			break;
		}
		if (!ARRAY_CONTAINS(((uint8_t[]){
			DT_ADC_GPIO_PB0, DT_ADC_GPIO_PB1, DT_ADC_GPIO_PB2, DT_ADC_GPIO_PB3,
			DT_ADC_GPIO_PB4, DT_ADC_GPIO_PB5, DT_ADC_GPIO_PB6, DT_ADC_GPIO_PB7,
			DT_ADC_GPIO_PD0, DT_ADC_GPIO_PD1, DT_ADC_GND, DT_ADC_VBAT}),
			channel_cfg->input_positive)) {
			LOG_ERR("adc not supported positive input: %u", channel_cfg->input_positive);
			break;
		}
		if (!ARRAY_CONTAINS(((uint8_t[]){
			DT_ADC_GPIO_PB0, DT_ADC_GPIO_PB1, DT_ADC_GPIO_PB2, DT_ADC_GPIO_PB3,
			DT_ADC_GPIO_PB4, DT_ADC_GPIO_PB5, DT_ADC_GPIO_PB6, DT_ADC_GPIO_PB7,
			DT_ADC_GPIO_PD0, DT_ADC_GPIO_PD1, DT_ADC_GND, DT_ADC_VBAT}),
			channel_cfg->input_negative)) {
			LOG_ERR("adc not supported negative input: %u", channel_cfg->input_negative);
			break;
		}
		data->channel[channel_cfg->channel_id].valid = true;
		data->channel[channel_cfg->channel_id].gain = channel_cfg->gain;
		data->channel[channel_cfg->channel_id].acquisition_time = channel_cfg->acquisition_time;
		data->channel[channel_cfg->channel_id].input_positive = channel_cfg->input_positive;
		data->channel[channel_cfg->channel_id].input_negative = channel_cfg->input_negative;
		LOG_INF("adc channel[%u] = "
			"{.gain=%u, .acquisition_time=%u, .input_positive=%u, input_negative=%u}",
			channel_cfg->channel_id, channel_cfg->gain, channel_cfg->acquisition_time,
			channel_cfg->input_positive, channel_cfg->input_negative);
		result = 0;
	} while (0);

	return result;
}

static int telink_tlx_adc_read(const struct device *dev,
			const struct adc_sequence *sequence)
{
	LOG_INF("%s %s", __func__, dev->name);

	const struct telink_tlx_adc_config *config = dev->config;

	(void)k_msgq_put(config->queue, &dev, K_FOREVER);

	return 0;
}

#ifdef CONFIG_ADC_ASYNC
static int telink_tlx_adc_read_async(const struct device *dev,
			      const struct adc_sequence *sequence,
			      struct k_poll_signal *async)
{
	LOG_INF("%s %s", __func__, dev->name);

	return 0;
}
#endif /* CONFIG_ADC_ASYNC */

static void telink_tlx_adc_thread_handler(struct k_msgq *queue)
{
	for (;;) {
		const struct device *dev;

		if (!k_msgq_get(queue, &dev, K_FOREVER)) {
			if (dev) {
				LOG_INF("%s %s", __func__, dev->name);
			} else {
				LOG_ERR("adc internal sw error");
			}
		}
	}
}

/************************************************************************
 * ADCs common data
 ************************************************************************/

K_MSGQ_DEFINE(telink_tlx_adc_msgq, sizeof(const struct device *),
	CONFIG_ADC_TLX_ACQUISITION_QUEUE_SIZE, __alignof__(const struct device *));
K_THREAD_DEFINE(telink_tlx_adc_thread, CONFIG_ADC_TLX_ACQUISITION_THREAD_STACK_SIZE,
	telink_tlx_adc_thread_handler, &telink_tlx_adc_msgq, NULL, NULL,
	CONFIG_ADC_TLX_ACQUISITION_THREAD_PRIO, 0, 0);

/************************************************************************
 * ADC driver registration
 ************************************************************************/

static const struct adc_driver_api telink_tlx_adc_api = {
	.channel_setup = telink_tlx_adc_channel_setup,
	.read = telink_tlx_adc_read,
#ifdef CONFIG_ADC_ASYNC
	.read_async = telink_tlx_adc_read_async
#endif
};

#define TELINK_TLX_ADC_DEFINE(n)                                                                   \
                                                                                                   \
	PINCTRL_DT_INST_DEFINE(n);                                                                     \
	                                                                                               \
	static const struct telink_tlx_adc_config tlx_adc_config##n = {                                \
		.queue = &telink_tlx_adc_msgq,                                                             \
	    .address = DT_INST_REG_ADDR(n),                                                            \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n)                                                  \
	};                                                                                             \
	                                                                                               \
	static struct telink_tlx_adc_data tlx_adc_data##n;                                             \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, telink_tlx_adc_init, NULL, &tlx_adc_data##n, &tlx_adc_config##n,      \
		POST_KERNEL, CONFIG_ADC_INIT_PRIORITY, &telink_tlx_adc_api);

DT_INST_FOREACH_STATUS_OKAY(TELINK_TLX_ADC_DEFINE)
