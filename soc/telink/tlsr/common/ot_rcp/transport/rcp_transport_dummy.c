/*
 * Copyright (c) 2025 Telink Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "rcp_transport.h"

#define LOG_LEVEL CONFIG_IEEE802154_DRIVER_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(rcp_transport_dummy);

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
	return 0;
}

void rcp_transport_reception_handler_set(struct rcp_transport_data *rcp_transport,
					 rpc_transport_reception_t rcp_transport_reception_handler)
{
	rcp_transport->reception_handler = rcp_transport_reception_handler;
}

int rcp_transport_init(struct rcp_transport_data *rcp_transport, const void *transport_device,
		       const void *ctx)
{
	rcp_transport->device = transport_device;
	rcp_transport->ctx = ctx;
	return 0;
}

int rcp_transport_deinit(struct rcp_transport_data *rcp_transport)
{
	return 0;
}
