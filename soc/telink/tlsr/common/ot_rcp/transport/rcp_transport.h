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

typedef void (*rpc_transport_reception_t)(const void *, uint8_t);
struct rcp_transport_data {
	const void *device;
	uint8_t tx_data[CONFIG_TELINK_OT_RCP_BUFFER_SIZE];
	size_t tx_data_size;
	rpc_transport_reception_t reception_handler;
	const void *ctx;
#if defined(CONFIG_TELINK_OT_RCP_TRANSPORT_UART)
	struct rcp_transport_uart_data {
		struct k_work work;
		struct ring_buf rb;
		uint8_t rb_data[CONFIG_TELINK_OT_RCP_BUFFER_SIZE];
	} data;
#elif defined(CONFIG_TELINK_OT_RCP_TRANSPORT_DUMMY)
	struct rcp_transport_dummy_data {
	} data;
#endif
};

typedef void (*rcp_transport_irq_t)(struct rcp_transport_data rcp_transport);

int rcp_transport_init(struct rcp_transport_data *rcp_transport, const void *transport_device,
		       const void *ctx);
int rcp_transport_deinit(struct rcp_transport_data *rcp_transport);

int rcp_transport_transmit(struct rcp_transport_data *rcp_transport);
void rcp_transport_put_byte(struct rcp_transport_data *rcp_transport, uint8_t bt);
void rcp_transport_reception_handler_set(struct rcp_transport_data *rcp_transport,
					 rpc_transport_reception_t rcp_transport_reception_handler);
#endif
