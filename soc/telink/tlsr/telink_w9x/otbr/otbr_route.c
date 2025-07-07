/*
 * Copyright (c) 2025 Telink Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/net/net_pkt.h>
#include <zephyr/net/ethernet.h>

extern int __real_net_route_packet(struct net_pkt *pkt, struct in6_addr *nexthop);

int __wrap_net_route_packet(struct net_pkt *pkt, struct in6_addr *nexthop)
{
	if (net_if_l2(net_pkt_orig_iface(pkt)) == &NET_L2_GET_NAME(OPENTHREAD) &&
	    !net_pkt_ll_proto_type(pkt)) {
		net_pkt_set_ll_proto_type(pkt, NET_ETH_PTYPE_IPV6);
	}
	return __real_net_route_packet(pkt, nexthop);
}
