/*
 * Copyright (c) 2025 Telink Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef RCP_TRANSPORT_H_
#define RCP_TRANSPORT_H_

#include <stdlib.h>
#include <stdbool.h>

#include <zephyr/kernel.h>
#include <zephyr/device.h>

typedef void (*rcp_transport_irq_t)(const struct device *transport_dev, void *user_data);

int rcp_transport_receive(const struct device *rcp_transport_dev, struct ring_buf *dest_buffer);
int rcp_transport_transmit(const struct device *rcp_transport_dev, const uint8_t *data,
			   size_t length);

int rcp_transport_set_callback(const struct device *rcp_transport_dev, rcp_transport_irq_t cb,
			       void *user_data);
void rcp_transport_irq_enable(const struct device *rcp_transport_dev);
void rcp_transport_irq_disable(const struct device *rcp_transport_dev);

#endif
