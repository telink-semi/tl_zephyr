/*
 * Copyright (c) 2025 Telink Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef OTBR_CONTEXT_H
#define OTBR_CONTEXT_H

#include <zephyr/net/openthread.h>
#include <zephyr/net/net_if.h>

#define IN6ADDR_ROUTER_MULTICAST_INIT                                                              \
	{                                                                                          \
		{{0xff, 0x02, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x02}}                        \
	}

#define IN6ADDR_DNS_MULTICAST_INIT                                                                 \
	{                                                                                          \
		{{0xff, 0x02, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0xfb}}                        \
	}

struct otbr_context {
	struct net_if *infra_if;
	struct openthread_context *ot_ctx;
};

#endif /* OTBR_CONTEXT_H */
