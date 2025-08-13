/*
 * Copyright (c) 2025 Telink Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "rcp_transport.h"

#define LOG_LEVEL CONFIG_IEEE802154_DRIVER_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(rcp_transport_uart);

#include <zephyr/drivers/uart.h>
#include <zephyr/sys/ring_buffer.h>

int rcp_transport_set_callback(const struct device *rcp_transport_dev, rcp_transport_irq_t cb,
			       void *user_data)
{
	return uart_irq_callback_user_data_set(rcp_transport_dev, cb, user_data);
}

void rcp_transport_irq_enable(const struct device *rcp_transport_dev)
{
	uart_irq_rx_enable(rcp_transport_dev);
}

int rcp_transport_transmit(const struct device *rcp_transport_dev, const uint8_t *data,
			   size_t length)
{
	return uart_fifo_fill(rcp_transport_dev, data, length);
}

int rcp_transport_receive(const struct device *rcp_transport_dev, struct ring_buf *dest_buffer)
{
	if (!uart_irq_update(rcp_transport_dev)) {
		return -ENOTSUP;
	}
	while (uart_irq_rx_ready(rcp_transport_dev)) {
		uint8_t bt;

		if (ring_buf_space_get(dest_buffer) > sizeof(bt)) {
			int r = uart_fifo_read(rcp_transport_dev, &bt, sizeof(bt));

			if (r < 0) {
				LOG_ERR("rcp uart reception error");
			} else if (r > 0) {
				(void)ring_buf_put(dest_buffer, &bt, r);
			}
		} else {
			/* that's ok for hw flow control */
			uart_irq_rx_disable(rcp_transport_dev);
			return 0;
		}
	}

	return 0;
}
