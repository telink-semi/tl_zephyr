/*
 * Copyright (c) 2026 Telink Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "st25dv.h"

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(main, LOG_LEVEL_INF);

int main(void)
{
	static ST25DV_DATA_DEFINE(st25dv, st25dv_app);
	int result = -ENODEV;

	do {
		result = st25dv_init(&st25dv);
		if (result) {
			LOG_ERR("st25dv init failed %d", result);
			break;
		}

		result = st25dv_deinit(&st25dv);
		if (result) {
			LOG_ERR("st25dv deinit failed %d", result);
			break;
		}
	} while (0);

	return result;
}
