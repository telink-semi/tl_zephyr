/** @file app_battery.c
 *  @brief
 */

/*
 * Copyright (c) 2025 Telink Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/types.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/kernel.h>

#include <zephyr/settings/settings.h>
#include <zephyr/drivers/adc.h>

#include "app_public.h"

#define LOG_LEVEL LOG_LEVEL_DBG
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(app_bat);



#if BATT_CHECK_ENABLE
/*ADC*/
#if !DT_NODE_EXISTS(DT_PATH(zephyr_user)) || \
	!DT_NODE_HAS_PROP(DT_PATH(zephyr_user), io_channels)
#error "No suitable devicetree overlay specified"
#endif

#define DT_SPEC_AND_COMMA(node_id, prop, idx) \
	ADC_DT_SPEC_GET_BY_IDX(node_id, idx),

/* Data of ADC io-channels specified in devicetree. */
static const struct adc_dt_spec adc_channels[] = {
	DT_FOREACH_PROP_ELEM(DT_PATH(zephyr_user), io_channels,
			     DT_SPEC_AND_COMMA)
};

uint32_t lowBattDet_tick = 0;


uint16_t buf;
struct adc_sequence sequence = {
    .buffer = &buf,
    /* buffer size in bytes, not number of samples */
    .buffer_size = sizeof(buf),
};

void app_battery_check_init(void)
{
    int err;

	/* Configure channels individually prior to sampling. */
	for (size_t i = 0U; i < ARRAY_SIZE(adc_channels); i++) {
		if (!device_is_ready(adc_channels[i].dev)) {
			LOG_INF("ADC controller device %s not ready\n", adc_channels[i].dev->name);
			return 0;
		}

		err = adc_channel_setup_dt(&adc_channels[i]);
		if (err < 0) {
			LOG_INF("Could not setup channel #%d (%d)\n", i, err);
			return 0;
		}
	}

    LOG_INF("ADC battery check init\n");
}

_attribute_ram_code_sec_ void app_battery_power_check(uint16_t alarm_vol_mv)
{
    int err;

    LOG_INF("ADC reading:\n");
    int32_t val_mv;

    (void)adc_sequence_init_dt(&adc_channels[0], &sequence);

    err = adc_read(adc_channels[0].dev, &sequence);
    if (err < 0) {
        LOG_INF("Could not read (%d)\n", err);
    }

    val_mv = (int32_t)buf;

    LOG_INF("%"PRId32, val_mv);
    err = adc_raw_to_millivolts_dt(&adc_channels[0],
                        &val_mv);
    /* conversion to mV may not be supported, skip if not */
    if (err < 0) {
        LOG_INF(" (value in mV not available)\n");
    } else {
        LOG_INF(" = %"PRId32" mV\n", val_mv);
    }
}

#endif