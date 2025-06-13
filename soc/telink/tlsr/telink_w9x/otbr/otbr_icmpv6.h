/*
 * Copyright (c) 2025 Telink Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef OTBR_ICMPV6_H
#define OTBR_ICMPV6_H

#include <zephyr/net/net_ip.h>

typedef void (*otbr_icmpv6_receive)(int if_idx, const struct in6_addr *src, const void *icmp_data,
				    size_t icmp_data_length, void *ctx);

int otbr_icmpv6_send(int if_idx, const struct in6_addr *dst, const void *icmp_data,
		     size_t icmp_data_length);
void otbr_icmpv6_start_listen(int if_idx, otbr_icmpv6_receive rx_cb, void *ctx);
void otbr_icmpv6_stop_listen(int if_idx);

#endif /* OTBR_ICMPV6_H */
