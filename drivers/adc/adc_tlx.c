/*
 * Copyright (c) 2022-2026 Telink Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT telink_tlx_adc

#include <zephyr/drivers/adc.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(adc_tlx, CONFIG_ADC_TELINK_TLX_LOG_LEVEL);

#include <adc.h>

/************************************************************************
 * ADC driver data types
 ************************************************************************/

struct telink_tlx_adc_data {
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
		LOG_INF("adc inited: %s", dev->name);
	} while (0);
	return result;
}

static int telink_tlx_adc_channel_setup(const struct device *dev,
				 const struct adc_channel_cfg *channel_cfg)
{
	LOG_INF("%s %s", __func__, dev->name);

	return 0;
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
