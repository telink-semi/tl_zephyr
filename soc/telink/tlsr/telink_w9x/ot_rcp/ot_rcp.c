/*
 * Copyright (c) 2025 Telink Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "ot_rcp.h"

#define LOG_LEVEL CONFIG_IEEE802154_DRIVER_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(ot_rcp);

#include <zephyr/drivers/uart.h>

static void openthread_rcp_reception(const struct device *dev, void *user_data)
{
	LOG_INF("%s", __func__);
}

int openthread_rcp_init(struct openthread_rcp_data *ot_rcp)
{
	LOG_INF("%s", __func__);

	int result = 0;

	do {
		if (!device_is_ready(ot_rcp->uart)) {
			LOG_ERR("spinel serial not ready");
			result = -EIO;
			break;
		}
		if (uart_irq_callback_user_data_set(ot_rcp->uart, openthread_rcp_reception, ot_rcp)) {
			LOG_ERR("can't set serial isr");
			result = -EIO;
			break;
		}

	} while (0);

	return result;
}

int openthread_rcp_deinit(struct openthread_rcp_data *ot_rcp)
{
	LOG_INF("%s", __func__);
	return 0;
}

int openthread_rcp_reset(struct openthread_rcp_data *ot_rcp)
{
	LOG_INF("%s", __func__);
	return 0;
}
