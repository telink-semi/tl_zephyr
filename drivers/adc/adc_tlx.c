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
 * ADC configuration checker
 ************************************************************************/

#ifndef CONFIG_ADC_CONFIGURABLE_INPUTS
#error CONFIG_ADC_CONFIGURABLE_INPUTS should be set
#endif /* CONFIG_ADC_CONFIGURABLE_INPUTS */

#ifdef CONFIG_ADC_CONFIGURABLE_EXCITATION_CURRENT_SOURCE_PIN
#error CONFIG_ADC_CONFIGURABLE_EXCITATION_CURRENT_SOURCE_PIN should not be set
#endif /* CONFIG_ADC_CONFIGURABLE_EXCITATION_CURRENT_SOURCE_PIN */

#ifdef CONFIG_ADC_CONFIGURABLE_VBIAS_PIN
#error CONFIG_ADC_CONFIGURABLE_VBIAS_PIN should not be set
#endif /* CONFIG_ADC_CONFIGURABLE_VBIAS_PIN */

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
	struct k_sem ready_sem;
	struct telink_tlx_adc_data_channel {
		bool valid;
		enum adc_gain gain;
		uint16_t acquisition_time;
		uint8_t input_positive;
		uint8_t input_negative;
	} channel[32];
	const struct adc_sequence *sequence;
};

struct telink_tlx_adc_config {
	struct k_msgq *queue;
	uintptr_t address;
	const struct pinctrl_dev_config *pcfg;
	uint32_t sample_freq;
};

/************************************************************************
 * ADC low-level wrappers
 ************************************************************************/

static inline int telink_tlx_adc_hw_init(uintptr_t base_addr)
{
	int result = -ENXIO;

	if (base_addr == (REG_RW_BASE_ADDR | ADC_BASE_ADDR)) {
		adc_init(NDMA_M_CHN);
		result = 0;
	}
	return result;
}

static inline int telink_tlx_adc_hw_set_channel(uintptr_t base_addr,
	const struct telink_tlx_adc_data_channel *channel,
	uint32_t sample_freq, uint16_t ref_internal)
{
	int result = -ENXIO;

	if (base_addr == (REG_RW_BASE_ADDR | ADC_BASE_ADDR)) {
		/* TODO: Dummy */
		LOG_INF("adc [sample rate: %u, reference: %u]", sample_freq, ref_internal);
		result = 0;
	}
	return result;
}

static inline int telink_tlx_adc_hw_get_data(uintptr_t base_addr, uint8_t oversampling,
	int16_t *sample)
{
	int result = -ENXIO;

	if (base_addr == (REG_RW_BASE_ADDR | ADC_BASE_ADDR)) {
		/* TODO: Dummy */
		*sample = 2047;
		result = 0;
	}
	return result;
}

/************************************************************************
 * ADC APIs
 ************************************************************************/

static int telink_tlx_adc_init(const struct device *dev)
{
	int result = 0;

	do {
		const struct telink_tlx_adc_config *config = dev->config;
		struct telink_tlx_adc_data *data = dev->data;

		result = pinctrl_apply_state(config->pcfg, PINCTRL_STATE_DEFAULT);
		if (result) {
			LOG_ERR("adc pinctrl failed %d", result);
			break;
		}
		result = telink_tlx_adc_hw_init(config->address);
		if (result) {
			LOG_ERR("adc init failed %d", result);
			break;
		}
		result = k_sem_init(&data->ready_sem, 1, 1);
		if (result) {
			LOG_ERR("adc ready semaphore failed %d", result);
			break;
		}
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
		if (channel_cfg->acquisition_time != ADC_ACQ_TIME_DEFAULT &&
			ADC_ACQ_TIME_UNIT(channel_cfg->acquisition_time) != ADC_ACQ_TIME_TICKS) {
			LOG_ERR("adc not supported acquisition time %u %u",
				(unsigned int)ADC_ACQ_TIME_VALUE(channel_cfg->acquisition_time),
				(unsigned int)ADC_ACQ_TIME_UNIT(channel_cfg->acquisition_time));
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
		result = k_sem_take(&data->ready_sem, K_NO_WAIT);
		if (result) {
			LOG_ERR("adc access error %u", result);
			break;
		}
		data->channel[channel_cfg->channel_id].valid = true;
		data->channel[channel_cfg->channel_id].gain = channel_cfg->gain;
		data->channel[channel_cfg->channel_id].acquisition_time = channel_cfg->acquisition_time;
		data->channel[channel_cfg->channel_id].input_positive = channel_cfg->input_positive;
		data->channel[channel_cfg->channel_id].input_negative = channel_cfg->input_negative;
		k_sem_give(&data->ready_sem);
		LOG_INF("adc channel[%u] = "
			"{.gain=%u, .acquisition_time=%u, .input_positive=%u, input_negative=%u}",
			channel_cfg->channel_id, channel_cfg->gain, channel_cfg->acquisition_time,
			channel_cfg->input_positive, channel_cfg->input_negative);
		result = 0;
	} while (0);

	return result;
}

static int telink_tlx_adc_start(const struct device *dev,
			const struct adc_sequence *sequence)
{
	const struct telink_tlx_adc_config *config = dev->config;
	struct telink_tlx_adc_data *data = dev->data;
	int result = -EINVAL;

	do {
		if (!sequence) {
			LOG_ERR("adc no sequence");
			break;
		}
		if (sequence->buffer_size < sizeof(int16_t) * POPCOUNT(sequence->channels) *
			(sequence->options ? sequence->options->extra_samplings + 1 : 1)) {
			LOG_ERR("adc buffer too small");
			break;
		}
		if (!ARRAY_CONTAINS(((uint8_t[]){8, 10, 12}),
			sequence->resolution)) {
			LOG_ERR("adc not supported resolution: %u", sequence->resolution);
			break;
		}
		result = 0;
		for (size_t i = 0; i < ARRAY_SIZE(data->channel); ++ i) {
			if ((sequence->channels & (1 << i)) && !data->channel[i].valid) {
				result = -EINVAL;
				break;
			}
		}
		if (result) {
			LOG_ERR("adc sequence channel not set");
			break;
		}
		result = k_sem_take(&data->ready_sem, K_NO_WAIT);
		if (result) {
			LOG_ERR("adc access error %u", result);
			break;
		}
		data->sequence = sequence;
		result = k_msgq_put(config->queue, &dev, K_FOREVER);
		if (result) {
			LOG_ERR("adc internal sw error %u", result);
			break;
		}
		result = 0;
	} while (0);
	return result;
}

static int telink_tlx_adc_read(const struct device *dev,
			const struct adc_sequence *sequence)
{
	struct telink_tlx_adc_data *data = dev->data;
	int result = -EINVAL;

	do {
		result = telink_tlx_adc_start(dev, sequence);
		if (result) {
			LOG_ERR("adc start error %u", result);
			break;
		}
		result = k_sem_take(&data->ready_sem, K_FOREVER);
		if (result) {
			LOG_ERR("adc access error %u", result);
			break;
		}
		k_sem_give(&data->ready_sem);
		result = 0;
	} while (0);
	return result;
}

#ifdef CONFIG_ADC_ASYNC
static int telink_tlx_adc_read_async(const struct device *dev,
			      const struct adc_sequence *sequence,
			      struct k_poll_signal *async)
{
	return telink_tlx_adc_start(dev, sequence);
}
#endif /* CONFIG_ADC_ASYNC */

static void telink_tlx_adc_thread_handler(struct k_msgq *queue)
{
	for (int result = 0;;) {
		const struct device *dev;

		result = k_msgq_get(queue, &dev, K_FOREVER);
		if (result) {
			continue;
		}
		if (!dev) {
			LOG_ERR("adc internal sw error");
			continue;
		}
		const struct telink_tlx_adc_config *config = dev->config;
		struct telink_tlx_adc_data *data = dev->data;
		const struct adc_driver_api *api = dev->api;

		for (uint16_t sampling_index = 0;;) {
			int16_t *adc_buffer = (int16_t *)data->sequence->buffer +
				POPCOUNT(data->sequence->channels) * sampling_index;

			for (size_t ch_id = 0; ch_id < ARRAY_SIZE(data->channel); ++ch_id) {
				if (!(data->sequence->channels & (1 << ch_id))) {
					continue;
				}
				int adc_hw_result = 0;

				do {
					adc_hw_result = telink_tlx_adc_hw_set_channel(config->address,
						&data->channel[ch_id], config->sample_freq, api->ref_internal);
					if (adc_hw_result) {
						LOG_ERR("adc channel failed %d", adc_hw_result);
						break;
					}
					for (uint16_t i = 0;
						data->channel[ch_id].acquisition_time != ADC_ACQ_TIME_DEFAULT &&
						i < ADC_ACQ_TIME_VALUE(data->channel[ch_id].acquisition_time); ++i) {
						__asm __volatile ("nop");
					}
					adc_hw_result = telink_tlx_adc_hw_get_data(config->address,
						data->sequence->oversampling, adc_buffer);
					if (adc_hw_result) {
						LOG_ERR("adc sample failed %d", adc_hw_result);
						break;
					}
				} while (0);
				if (adc_hw_result) {
					*adc_buffer = 0;
				}
				adc_buffer++;
			}
			enum adc_action action = ADC_ACTION_CONTINUE;

			if (data->sequence->options && data->sequence->options->callback) {
				action = data->sequence->options->callback(dev, data->sequence, sampling_index);
			}
			if (action == ADC_ACTION_REPEAT) {
				continue;
			} else if (action == ADC_ACTION_FINISH) {
				break;
			}
			sampling_index++;
			if (sampling_index <
				(data->sequence->options ? data->sequence->options->extra_samplings + 1 : 1)) {
				if (data->sequence->options && data->sequence->options->interval_us) {
					k_usleep(data->sequence->options->interval_us);
				}
			} else {
				break;
			}
		}
		k_sem_give(&data->ready_sem);
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

#define TELINK_TLX_ADC_DEFINE(n)                                                                   \
                                                                                                   \
	static const struct adc_driver_api telink_tlx_adc_api##n = {                                   \
		.channel_setup = telink_tlx_adc_channel_setup,                                             \
		.read = telink_tlx_adc_read,                                                               \
		IF_ENABLED(CONFIG_ADC_ASYNC, (.read_async = telink_tlx_adc_read_async,))                   \
		.ref_internal = DT_INST_PROP(n, vref_internal_mv)                                          \
	};                                                                                             \
                                                                                                   \
	PINCTRL_DT_INST_DEFINE(n);                                                                     \
	                                                                                               \
	static const struct telink_tlx_adc_config tlx_adc_config##n = {                                \
		.queue = &telink_tlx_adc_msgq,                                                             \
		.address = DT_INST_REG_ADDR(n),                                                            \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),                                                 \
		.sample_freq = DT_INST_PROP(0, sample_freq)                                                \
	};                                                                                             \
                                                                                                   \
	static struct telink_tlx_adc_data tlx_adc_data##n;                                             \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, telink_tlx_adc_init, NULL, &tlx_adc_data##n, &tlx_adc_config##n,      \
		POST_KERNEL, CONFIG_ADC_INIT_PRIORITY, &telink_tlx_adc_api##n);

DT_INST_FOREACH_STATUS_OKAY(TELINK_TLX_ADC_DEFINE)
