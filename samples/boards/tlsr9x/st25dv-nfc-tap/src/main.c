/*
 * Copyright (c) 2026 Telink Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "st25dv.h"

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(main, LOG_LEVEL_INF);

static ST25DV_DATA_DEFINE(st25dv_app);

int main(void)
{
	LOG_INF("st25dv i2c: %s", st25dv_app.i2c_bus->name);
	LOG_INF("st25dv isr: %s %u", st25dv_app.isr_port->name, st25dv_app.isr_port_pin);
	return 0;
}
