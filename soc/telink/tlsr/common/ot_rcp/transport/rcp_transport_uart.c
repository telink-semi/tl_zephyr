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

/************************************************************************
 * RCP transport reception work queue
 ************************************************************************/

static K_KERNEL_STACK_DEFINE(rcp_transport_uart_work_q_stack,
			     CONFIG_TELINK_OT_RCP_THREAD_STACK_SIZE);

static struct k_work_q rcp_transport_uart_work_q;

static int rcp_transport_uart_work_q_init(void)
{
	struct k_work_queue_config cfg = {
		.name = "rcp_transport_uart_workq", .no_yield = false, .essential = false};

	k_work_queue_start(&rcp_transport_uart_work_q, rcp_transport_uart_work_q_stack,
			   K_KERNEL_STACK_SIZEOF(rcp_transport_uart_work_q_stack),
			   CONFIG_TELINK_OT_RCP_THREAD_PRIORITY, &cfg);
	return 0;
}

SYS_INIT(rcp_transport_uart_work_q_init, POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);

/************************************************************************
 * RCP transport internal functions
 ************************************************************************/

static void rcp_transport_reception_work(struct k_work *item)
{
	struct rcp_transport_uart_data *rcp_transport_uart =
		CONTAINER_OF(item, struct rcp_transport_uart_data, work);
	struct rcp_transport_data *rcp_transport =
		CONTAINER_OF(rcp_transport_uart, struct rcp_transport_data, data);

	for (;;) {
		uint8_t bt;

		if (ring_buf_get(&rcp_transport->data.rb, &bt, sizeof(bt)) == sizeof(bt)) {
			rcp_transport->reception_handler(rcp_transport->ctx, bt);
		} else {
			break;
		}
	}
	uart_irq_rx_enable((const struct device *)rcp_transport->device);
}

static void rcp_transport_reception_isr(const struct device *dev, void *user_data)
{
	struct rcp_transport_data *rcp_transport = (struct rcp_transport_data *)user_data;

	if (!uart_irq_update(dev)) {
		return;
	}
	while (uart_irq_rx_ready(dev)) {
		uint8_t bt;

		if (ring_buf_space_get(&rcp_transport->data.rb) > sizeof(bt)) {
			int r = uart_fifo_read(dev, &bt, sizeof(bt));

			if (r < 0) {
				LOG_ERR("rcp uart reception error");
			} else if (r > 0) {
				(void)ring_buf_put(&rcp_transport->data.rb, &bt, r);
			}
		} else {
			/* that's ok for hw flow control */
			uart_irq_rx_disable(dev);
			break;
		}
	}
	if (!ring_buf_is_empty(&rcp_transport->data.rb)) {
		k_work_submit_to_queue(&rcp_transport_uart_work_q, &rcp_transport->data.work);
	}
}

/************************************************************************
 * RCP transport interface functions
 ************************************************************************/

void rcp_transport_put_byte(struct rcp_transport_data *rcp_transport, uint8_t bt)
{
	if (rcp_transport->tx_data_size < sizeof(rcp_transport->tx_data)) {
		rcp_transport->tx_data[rcp_transport->tx_data_size] = bt;
		rcp_transport->tx_data_size++;
	} else {
		LOG_ERR("rcp tx buffer overflow");
		rcp_transport->tx_data_size = 0;
	}
}

int rcp_transport_transmit(struct rcp_transport_data *rcp_transport)
{
	int result = 0;
	const uint8_t *tx_ptr = rcp_transport->tx_data;
	size_t tx_len = rcp_transport->tx_data_size;

	while (tx_len) {
		int r = uart_fifo_fill((const struct device *)rcp_transport->device, tx_ptr,
				       tx_len);

		if (r < 0) {
			result = r;
			break;
		}
		tx_ptr += r;
		tx_len -= r;
		result += r;
	}
	if (result != rcp_transport->tx_data_size) {
		LOG_ERR("rcp rcp_transport transmission error %u", result);
		result = result < 0 ? result : -EIO;
	}
	rcp_transport->tx_data_size = 0;

	return result;
}

void rcp_transport_reception_handler_set(struct rcp_transport_data *rcp_transport,
					 rpc_transport_reception_t rcp_transport_reception_handler)
{
	rcp_transport->reception_handler = rcp_transport_reception_handler;
	uart_irq_rx_enable((const struct device *)rcp_transport->device);
}

int rcp_transport_init(struct rcp_transport_data *rcp_transport, const void *transport_device,
		       const void *ctx)
{
	LOG_DBG("%s", __func__);

	int result = 0;

	rcp_transport->device = transport_device;
	rcp_transport->ctx = ctx;

	do {
		rcp_transport->tx_data_size = 0;
		ring_buf_init(&rcp_transport->data.rb, sizeof(rcp_transport->data.rb_data),
			      rcp_transport->data.rb_data);
		k_work_init(&rcp_transport->data.work, rcp_transport_reception_work);
		if (uart_irq_callback_user_data_set((const struct device *)rcp_transport->device,
						    rcp_transport_reception_isr,
						    (void *)rcp_transport)) {
			result = -EIO;
			LOG_ERR("spinel uart interrupt setting failed");
			break;
		}
	} while (0);

	return result;
}

int rcp_transport_deinit(struct rcp_transport_data *rcp_transport)
{
	int result = 0;

	uart_irq_rx_disable((const struct device *)rcp_transport->device);

	do {
		if (uart_irq_callback_user_data_set((const struct device *)rcp_transport->device,
						    NULL, NULL)) {
			result = -EIO;
			break;
		}

		struct k_work_sync work_sync;
		(void)k_work_cancel_sync(&rcp_transport->data.work, &work_sync);

		ring_buf_reset(&rcp_transport->data.rb);
	} while (0);

	return result;
}
