/*
 * Copyright (c) 2025 Telink
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "connections.h"

#include <zephyr/net/openthread.h>
#include <openthread/border_routing.h>
#include <openthread/platform/infra_if.h>

#include <zephyr/net/icmp.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(main, LOG_LEVEL_INF);

static struct net_icmp_ctx icmp_rs_ctx, icmp_ra_ctx, icmp_na_ctx;

static int icmp_input(struct net_icmp_ctx *ctx, struct net_pkt *pkt, struct net_icmp_ip_hdr *hdr,
		      struct net_icmp_hdr *icmp_hdr, void *user_data)
{
	if (hdr->family != AF_INET6) {
		return 0;
	}
	if (hdr->ipv6->hop_limit != 255) {
		return 0;
	}
	if (!net_ipv6_is_ll_addr((struct in6_addr *)hdr->ipv6->src)) {
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
			otPlatInfraIfRecvIcmp6Nd(
				openthread_get_default_instance(), net_if_get_by_iface(ctx->iface),
				(otIp6Address *)hdr->ipv6->src, icmp_data, sizeof(icmp_data));
		} while (0);
		net_pkt_cursor_restore(pkt, &bkp);
	}
	return 0;
}

void wifi_changed(struct net_if *iface, bool is_connected)
{
	if (is_connected) {
		LOG_INF("** wifi connected");
		otError err = otBorderRoutingInit(openthread_get_default_instance(),
						  net_if_get_by_iface(iface), true);

		if (err == OT_ERROR_NONE) {
			(void)net_icmp_init_ctx(&icmp_rs_ctx, 133, 0, icmp_input);
			icmp_rs_ctx.iface = iface;
			(void)net_icmp_init_ctx(&icmp_ra_ctx, 134, 0, icmp_input);
			icmp_ra_ctx.iface = iface;
			(void)net_icmp_init_ctx(&icmp_na_ctx, 136, 0, icmp_input);
			icmp_na_ctx.iface = iface;
			err = otBorderRoutingSetEnabled(openthread_get_default_instance(), true);
			if (err == OT_ERROR_NONE) {
				LOG_INF("openthread border router enabled");
			} else {
				LOG_ERR("openthread border router enabling failed %d", err);
			}
		} else {
			LOG_ERR("openthread border router init failed %d", err);
		}
	} else {
		LOG_WRN("** wifi disconnected");
		otError err = otBorderRoutingSetEnabled(openthread_get_default_instance(), false);
		(void)net_icmp_cleanup_ctx(&icmp_rs_ctx);
		(void)net_icmp_cleanup_ctx(&icmp_ra_ctx);
		(void)net_icmp_cleanup_ctx(&icmp_na_ctx);

		if (err == OT_ERROR_NONE) {
			LOG_INF("openthread border router disabled");
		} else {
			LOG_ERR("openthread border router disabling failed %d", err);
		}
	}
}

void thread_changed(otInstance *instance, otDeviceRole role)
{
	ARG_UNUSED(instance);

	LOG_INF("** Openthread %s", otThreadDeviceRoleToString(role));
}

int main(void)
{
	LOG_INF("***** Network CLI on Zephyr (%s) *****", net_if_get_default()->config.name);

	connections_init(wifi_changed, thread_changed);

	return 0;
}
