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

	LOG_INF("st25dv i2c: %s", st25dv.i2c_bus->name);
	LOG_INF("st25dv isr: %s %u", st25dv.isr_port->name, st25dv.isr_port_pin);

	int result = -ENODEV;

	do {
		if (!device_is_ready(st25dv.i2c_bus)) {
			LOG_ERR("st25dv i2c bus %s not ready", st25dv.i2c_bus->name);
			break;
		}
		if (!device_is_ready(st25dv.isr_port)) {
			LOG_ERR("st25dv gpio %s not ready", st25dv.isr_port->name);
			break;
		}
		result = st25dv_probe(&st25dv);
		if (result) {
			LOG_ERR("st25dv_probe failed");
			break;
		}
		result = st25dv_reset(&st25dv);
		if (result) {
			LOG_ERR("st25dv_reset failed");
			break;
		}
		LOG_INF("st25dv sequence done");
	} while (0);

	return result;
}
