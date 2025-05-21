/*
 * Copyright (c) 2025 Telink Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <openthread/platform/infra_if.h>
#include <openthread/ip6.h>

#include <zephyr/net/net_if.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/net/net_pkt.h>

#define LOG_LEVEL CONFIG_OPENTHREAD_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(otbr_ext);

static inline uint32_t icmpv6_crc_partial(const void *data, size_t data_len)
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

static uint16_t icmpv6_crc(const struct in6_addr *src, const struct in6_addr *dst,
			   const void *icmp_data, size_t icmp_len)
{
	uint32_t sum = 0;

	sum += htonl(icmp_len);
	sum += htons(IPPROTO_ICMPV6);
	sum += icmpv6_crc_partial(src, sizeof(struct in6_addr));
	sum += icmpv6_crc_partial(dst, sizeof(struct in6_addr));
	sum += icmpv6_crc_partial(icmp_data, icmp_len > 2 ? 2 : icmp_len);
	if (icmp_len > 4) {
		sum += icmpv6_crc_partial(&((uint8_t *)icmp_data)[4], icmp_len - 4);
	}
	while (sum >> 16) {
		sum = (sum & 0xffff) + (sum >> 16);
	}
	return (uint16_t)~sum;
}

static struct net_pkt *icmpv6_pkt_create(int if_idx, const struct in6_addr *dst, const void *data,
					 uint16_t data_length)
{
	bool created = false;
	struct net_if *iface = net_if_get_by_index(if_idx);
	struct net_pkt *pkt =
		iface ? net_pkt_alloc_with_buffer(iface, data_length < 4 ? 4 : data_length,
						  AF_INET6, IPPROTO_ICMPV6, K_NO_WAIT)
		      : NULL;

	do {
		if (!data || !pkt) {
			break;
		}

		const struct in6_addr *src = net_if_ipv6_get_ll(iface, NET_ADDR_ANY_STATE);
		struct net_ipv6_hdr ipv6_hdr = {.vtc = 0x60,
						.tcflow = 0,
						.len = htons(data_length),
						.nexthdr = IPPROTO_ICMPV6,
						.hop_limit = 255};

		memcpy(&ipv6_hdr.src, src, sizeof(ipv6_hdr.src));
		memcpy(&ipv6_hdr.dst, dst, sizeof(ipv6_hdr.src));
		if (net_pkt_write(pkt, &ipv6_hdr, sizeof(ipv6_hdr))) {
			break;
		}
		net_pkt_set_ip_hdr_len(pkt, sizeof(ipv6_hdr));

		struct net_icmp_hdr icmp_hdr = {.type = data_length >= 1 ? ((uint8_t *)data)[0] : 0,
						.code = data_length >= 2 ? ((uint8_t *)data)[1] : 0,
						.chksum = icmpv6_crc(src, dst, data, data_length)};
		if (net_pkt_write(pkt, &icmp_hdr, sizeof(icmp_hdr))) {
			break;
		}
		if (data_length > sizeof(struct net_icmp_hdr)) {
			if (net_pkt_write(pkt, &((uint8_t *)data)[sizeof(struct net_icmp_hdr)],
					  data_length - sizeof(struct net_icmp_hdr))) {
				break;
			}
		}
		net_pkt_cursor_init(pkt);
		created = true;
	} while (0);
	if (!created && pkt) {
		net_pkt_unref(pkt);
		pkt = NULL;
	}
	return pkt;
}

otError otPlatInfraIfSendIcmp6Nd(uint32_t aInfraIfIndex, const otIp6Address *aDestAddress,
				 const uint8_t *aBuffer, uint16_t aBufferLength)
{
	otError result = OT_ERROR_FAILED;
	struct in6_addr dest;

	memcpy(&dest, aDestAddress, sizeof(dest));

	struct net_pkt *pkt = icmpv6_pkt_create(aInfraIfIndex, &dest, aBuffer, aBufferLength);

	if (pkt) {
		if (!net_send_data(pkt)) {
			result = OT_ERROR_NONE;
		} else {
			net_pkt_unref(pkt);
			LOG_ERR("can't send icmpv6 nd");
		}
	} else {
		LOG_ERR("can't create icmpv6 nd");
	}
	return result;
}

bool otPlatInfraIfHasAddress(uint32_t aInfraIfIndex, const otIp6Address *aAddress)
{
	bool result = false;
	struct net_if *iface = net_if_get_by_index(aInfraIfIndex);
	struct in6_addr addr;

	memcpy(&addr, aAddress, sizeof(addr));
	if (iface) {
		if (net_if_ipv6_addr_lookup_by_iface(iface, &addr)) {
			result = true;
		}
	}
	return result;
}
