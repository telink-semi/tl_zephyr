/*
 * Copyright (c) 2026 Telink Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ST25DV_H
#define ST25DV_H

#include <zephyr/device.h>

struct st25dv_data {
	const struct device *const i2c_bus;
	const struct device *const isr_port;
	const uint8_t isr_port_pin;
};

#define ST25DV_DATA_DEFINE(name, st25dv_node_label)                                                \
	struct st25dv_data name = {                                                                \
		.i2c_bus = DEVICE_DT_GET(DT_PHANDLE(DT_NODELABEL(st25dv_node_label), i2c_bus)),    \
		.isr_port = DEVICE_DT_GET(DT_PHANDLE(DT_NODELABEL(st25dv_node_label), isr_port)),  \
		.isr_port_pin = DT_PROP(DT_NODELABEL(st25dv_node_label), isr_port_pin),            \
	}

int st25dv_probe(struct st25dv_data *data);
int st25dv_reset(struct st25dv_data *data);

#endif /* ST25DV_H */
