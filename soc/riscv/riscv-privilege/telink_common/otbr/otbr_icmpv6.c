/*
 * Copyright (c) 2025 Telink Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "otbr_icmpv6.h"
#include <zephyr/net/net_pkt.h>
#include <../subsys/net/ip/icmpv6.h>
#include <zephyr/net/ethernet.h>

/************************************************************************
 * ICMPv6 internal data
 ************************************************************************/

#define OTBR_ICMPV6_CTX_CNT 3

static struct otbr_icmpv6_set {
	otbr_icmpv6_receive icmpv6_receive;
	void *icmpv6_context;
} otbr_icmpv6_setup;

static enum net_verdict otbr_icmpv6_input(struct net_pkt *pkt,
			     struct net_ipv6_hdr *hdr, struct net_icmp_hdr *icmp_hdr);

struct net_icmpv6_handler otbr_icmpv6_handler[OTBR_ICMPV6_CTX_CNT] = {
	{
		.handler = otbr_icmpv6_input,
		.type = 133
	},
		{
		.handler = otbr_icmpv6_input,
		.type = 134
	},
	{
		.handler = otbr_icmpv6_input,
		.type = 136
	},
};

/************************************************************************
 * ICMPv6 internal functions
 ************************************************************************/

static inline uint32_t otbr_icmpv6_crc_partial(const void *data, size_t data_len)
{
	uint32_t crc = 0;
	const uint16_t *data_16 = data;

	while (data_len > 1) {
		crc += *data_16;
		data_16++;
		data_len -= 2;
	}
	if (data_len) {
		crc += *(uint8_t *)data_16;
	}
	return crc;
}

static uint16_t otbr_icmpv6_crc(const struct in6_addr *src, const struct in6_addr *dst,
				const void *icmp_data, size_t icmp_len)
{
	uint32_t sum = 0;

	sum += htonl(icmp_len);
	sum += htons(IPPROTO_ICMPV6);
	sum += otbr_icmpv6_crc_partial(src, sizeof(struct in6_addr));
	sum += otbr_icmpv6_crc_partial(dst, sizeof(struct in6_addr));
	sum += otbr_icmpv6_crc_partial(icmp_data, icmp_len > 2 ? 2 : icmp_len);
	if (icmp_len > 4) {
		sum += otbr_icmpv6_crc_partial(&((uint8_t *)icmp_data)[4], icmp_len - 4);
	}
	while (sum >> 16) {
		sum = (sum & 0xffff) + (sum >> 16);
	}
	return (uint16_t)~sum;
}

static enum net_verdict otbr_icmpv6_input(struct net_pkt *pkt,
			     struct net_ipv6_hdr *hdr, struct net_icmp_hdr *icmp_hdr)
{
	if (hdr->hop_limit != 255) {
		return 0;
	}
	if (!net_ipv6_is_ll_addr((struct in6_addr *)hdr->src)) {
		return 0;
	}

	size_t tot_len = net_pkt_get_len(pkt);
	uint8_t ip_len = net_pkt_ip_hdr_len(pkt);

	if (tot_len > ip_len) {
		uint8_t icmp_data[tot_len - ip_len];
		struct net_pkt_cursor bkp;

		net_pkt_cursor_backup(pkt, &bkp);
		net_pkt_cursor_init(pkt);
		do {
			if (net_pkt_skip(pkt, ip_len) < 0) {
				break;
			}
			if (net_pkt_read(pkt, icmp_data, sizeof(icmp_data)) < 0) {
				break;
			}
			if (otbr_icmpv6_setup.icmpv6_receive) {
				otbr_icmpv6_setup.icmpv6_receive(
					net_if_get_by_iface(net_pkt_iface(pkt)),
					(const struct in6_addr *)hdr->src, icmp_data,
					sizeof(icmp_data), otbr_icmpv6_setup.icmpv6_context);
			}
		} while (0);
		net_pkt_cursor_restore(pkt, &bkp);
	}
	return NET_OK;
}

/************************************************************************
 * ICMPv6 external functions
 ************************************************************************/

int otbr_icmpv6_send(int if_idx, const struct in6_addr *dst, const void *icmp_data,
		     size_t icmp_data_length)
{
	int result = -EINVAL;
	struct net_pkt *pkt = NULL;

	do {
		if (!dst || !icmp_data) {
			break;
		}

		struct net_if *iface = net_if_get_by_index(if_idx);

		if (!iface) {
			break;
		}
		pkt = net_pkt_alloc_with_buffer(iface, icmp_data_length < 4 ? 4 : icmp_data_length,
						AF_INET6, IPPROTO_ICMPV6, K_NO_WAIT);

		if (!pkt) {
			result = -ENOMEM;
			break;
		}

		const struct in6_addr *src = net_if_ipv6_get_ll(iface, NET_ADDR_ANY_STATE);
		struct net_ipv6_hdr ipv6_hdr = {.vtc = 0x60,
						.tcflow = 0,
						.len = htons(icmp_data_length),
						.nexthdr = IPPROTO_ICMPV6,
						.hop_limit = 255};

		memcpy(&ipv6_hdr.src, src, sizeof(ipv6_hdr.src));
		memcpy(&ipv6_hdr.dst, dst, sizeof(ipv6_hdr.src));
		result = net_pkt_write(pkt, &ipv6_hdr, sizeof(ipv6_hdr));
		if (result < 0) {
			break;
		}
		net_pkt_set_ip_hdr_len(pkt, sizeof(ipv6_hdr));

		struct net_icmp_hdr icmp_hdr = {
			.type = icmp_data_length >= 1 ? ((uint8_t *)icmp_data)[0] : 0,
			.code = icmp_data_length >= 2 ? ((uint8_t *)icmp_data)[1] : 0,
			.chksum = otbr_icmpv6_crc(src, dst, icmp_data, icmp_data_length)};

		result = net_pkt_write(pkt, &icmp_hdr, sizeof(icmp_hdr));
		if (result < 0) {
			break;
		}
		if (icmp_data_length > sizeof(struct net_icmp_hdr)) {
			result = net_pkt_write(pkt,
					       &((uint8_t *)icmp_data)[sizeof(struct net_icmp_hdr)],
					       icmp_data_length - sizeof(struct net_icmp_hdr));
			if (result < 0) {
				break;
			}
		}
		net_pkt_set_ll_proto_type(pkt, NET_ETH_PTYPE_IPV6);
		net_pkt_cursor_init(pkt);
		result = net_send_data(pkt);
	} while (0);
	if (result < 0 && pkt) {
		net_pkt_unref(pkt);
	}
	return result;
}

void otbr_icmpv6_start_listen(int if_idx, otbr_icmpv6_receive rx_cb, void *ctx)
{
	struct net_if *iface = net_if_get_by_index(if_idx);

	if (iface) {
		otbr_icmpv6_setup.icmpv6_receive = rx_cb;
		otbr_icmpv6_setup.icmpv6_context = ctx;
		for (size_t i = 0; i < ARRAY_SIZE(otbr_icmpv6_handler); i++) {
			net_icmpv6_register_handler(&otbr_icmpv6_handler[i]);
		}
	}
}

void otbr_icmpv6_stop_listen(int if_idx)
{
	struct net_if *iface = net_if_get_by_index(if_idx);

	if (iface) {
		for (size_t i = 0; i < ARRAY_SIZE(otbr_icmpv6_handler); i++) {
			net_icmpv6_unregister_handler(&otbr_icmpv6_handler[i]);
		}
		otbr_icmpv6_setup.icmpv6_receive = NULL;
		otbr_icmpv6_setup.icmpv6_context = NULL;
	}
}
