/*
 * Copyright (c) 2025 Telink Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "otbr_ext.h"
#include "otbr_context.h"
#include "otbr_srp.h"
#include "otbr_mdns.h"
#include <zephyr/init.h>
#include <zephyr/net/net_config.h>
#include <zephyr/net/wifi_mgmt.h>
#include <zephyr/net/ethernet_mgmt.h>
#include <../subsys/net/ip/ipv6.h>

#define LOG_LEVEL CONFIG_TELINK_OTBR_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(otbr_init);

/************************************************************************
 * OTBR init Check configuration
 ************************************************************************/

#if CONFIG_NET_CONFIG_AUTO_INIT
#error network should be started manually
#endif /* CONFIG_NET_CONFIG_AUTO_INIT */

/************************************************************************
 * OTBR init Definitions
 ************************************************************************/

#define OTBR_INIT_INFRA_AUX_PERIOD_MS 500

/************************************************************************
 * OTBR init Data types
 ************************************************************************/
struct otbr_init_ctx {
	struct otbr_context otbr_ctx;
	struct net_mgmt_event_callback infra_cb;
	struct k_work_delayable infra_aux_work;
};

/************************************************************************
 * OTBR init Internals
 ************************************************************************/

static void otbr_init_infra_auxiliary_work_handler(struct k_work *item)
{
#if CONFIG_WIFI
	static struct wifi_connect_req_params con_req = {
		.ssid = CONFIG_TELINK_OTBR_WIFI_SSID,
		.ssid_length = strlen(CONFIG_TELINK_OTBR_WIFI_SSID),
		.psk = CONFIG_TELINK_OTBR_WIFI_PASSWORD,
		.psk_length = strlen(CONFIG_TELINK_OTBR_WIFI_PASSWORD),
		.security = WIFI_SECURITY_TYPE_PSK};
	struct otbr_init_ctx *otbr_context = CONTAINER_OF(k_work_delayable_from_work(item),
							  struct otbr_init_ctx, infra_aux_work);

	if (net_if_is_wifi(otbr_context->otbr_ctx.infra_if)) {
		LOG_INF("wifi '%s' connecting...", CONFIG_TELINK_OTBR_WIFI_SSID);
		int req = net_mgmt(NET_REQUEST_WIFI_CONNECT, otbr_context->otbr_ctx.infra_if,
				   &con_req, sizeof(con_req));

		if (req && req != -EALREADY) {
			LOG_WRN("wifi request connect fail %d", req);
			(void)k_work_schedule(&otbr_context->infra_aux_work,
					      K_MSEC(OTBR_INIT_INFRA_AUX_PERIOD_MS));
		}
	}
#else
	ARG_UNUSED(item);
#endif /* CONFIG_WIFI */
}

static void ot_change(otChangedFlags flags, struct openthread_context *ot_context, void *user_data)
{
	ARG_UNUSED(user_data);

	if (flags & OT_CHANGED_THREAD_NETDATA) {
		otbr_ext_apply_omr_prefix(ot_context);
		/* Inform ready route */
		if (otbr_ext_omr_ipaddr_show(ot_context)) {
			otbr_ext_thread_dataset_show(ot_context);
		}
	}
}

static void infra_change(struct net_mgmt_event_callback *cb, uint32_t mgmt_event,
			 struct net_if *iface)
{
	struct otbr_init_ctx *otbr_context = CONTAINER_OF(cb, struct otbr_init_ctx, infra_cb);

	switch (mgmt_event) {
	case NET_EVENT_ETHERNET_CARRIER_ON: {
		LOG_INF("infra carrier on");
		otbr_ext_infra_up(&otbr_context->otbr_ctx);
	} break;
	case NET_EVENT_ETHERNET_CARRIER_OFF:
		LOG_INF("infra carrier off");
		otbr_ext_infra_down(&otbr_context->otbr_ctx);
		(void)k_work_schedule(&otbr_context->infra_aux_work, K_NO_WAIT);
		break;
	}
}

static void otbr_init_start(void)
{
	static struct otbr_init_ctx otbr_context;

	STRUCT_SECTION_FOREACH(net_if, iface) {
		if (net_if_l2(iface) == &NET_L2_GET_NAME(ETHERNET)) {
			otbr_context.otbr_ctx.infra_if = iface;
		}
	}
	otbr_context.otbr_ctx.ot_ctx = openthread_get_default_context();
	if (otbr_context.otbr_ctx.infra_if && otbr_context.otbr_ctx.ot_ctx) {

		static struct openthread_state_changed_cb ot_cb = {.state_changed_cb = ot_change};

		openthread_state_changed_cb_register(otbr_context.otbr_ctx.ot_ctx, &ot_cb);
		otbr_ext_thread_start(otbr_context.otbr_ctx.ot_ctx);

		static const struct in6_addr router_addr = IN6ADDR_ROUTER_MULTICAST_INIT;
		static const struct in6_addr mdns_addr = IN6ADDR_DNS_MULTICAST_INIT;

		net_if_ipv6_maddr_join(
			otbr_context.otbr_ctx.infra_if,
			net_if_ipv6_maddr_add(otbr_context.otbr_ctx.infra_if, &router_addr));
		net_if_ipv6_maddr_join(
			otbr_context.otbr_ctx.infra_if,
			net_if_ipv6_maddr_add(otbr_context.otbr_ctx.infra_if, &mdns_addr));

		const struct ethernet_context *infra_ctx =
			net_if_l2_data(otbr_context.otbr_ctx.infra_if);

		if (infra_ctx->is_net_carrier_up) {
			otbr_ext_infra_up(&otbr_context.otbr_ctx);
		}
		net_mgmt_init_event_callback(&otbr_context.infra_cb, infra_change,
					     NET_EVENT_ETHERNET_CARRIER_ON |
						     NET_EVENT_ETHERNET_CARRIER_OFF);
		net_mgmt_add_event_callback(&otbr_context.infra_cb);
		k_work_init_delayable(&otbr_context.infra_aux_work,
				      otbr_init_infra_auxiliary_work_handler);
		(void)k_work_schedule(&otbr_context.infra_aux_work, K_NO_WAIT);

		struct in6_addr *infra_ll =
			net_if_ipv6_get_ll(otbr_context.otbr_ctx.infra_if, NET_ADDR_ANY_STATE);
		const struct net_linkaddr *infra_mac =
			net_if_get_link_addr(otbr_context.otbr_ctx.infra_if);
		struct in6_addr *ot_ll =
			net_if_ipv6_get_ll(otbr_context.otbr_ctx.ot_ctx->iface, NET_ADDR_ANY_STATE);
		const struct net_linkaddr *ot_mac =
			net_if_get_link_addr(otbr_context.otbr_ctx.ot_ctx->iface);

		if (!net_ipv6_nbr_add(otbr_context.otbr_ctx.ot_ctx->iface, infra_ll, infra_mac,
				      true, NET_IPV6_NBR_STATE_REACHABLE)) {
			LOG_ERR("otbr ot neighbor failed");
		}
		if (!net_ipv6_nbr_add(otbr_context.otbr_ctx.infra_if, ot_ll, ot_mac, true,
				      NET_IPV6_NBR_STATE_REACHABLE)) {
			LOG_ERR("otbr infra neighbor failed");
		}
#if CONFIG_TELINK_OTBR_LOG_LEVEL_DEBG
		otbr_srp_init(otbr_context.otbr_ctx.ot_ctx);
#endif /* CONFIG_TELINK_OTBR_LOG_LEVEL_DEBG */
		otbr_mdns_start(&otbr_context.otbr_ctx);
	} else {
		LOG_ERR("otbr start failed: infra_if=%p, ot_ctx=%p", otbr_context.otbr_ctx.infra_if,
			otbr_context.otbr_ctx.ot_ctx);
	}
}

static void otbr_init_network_init(void)
{
	STRUCT_SECTION_FOREACH(net_if, iface) {
		if (net_if_is_up(iface)) {
			(void)net_config_init_app(net_if_get_device(iface), NULL);
		} else {
			char if_name[CONFIG_NET_INTERFACE_NAME_LEN + 1];

			if (net_if_get_name(iface, if_name, sizeof(if_name)) >= 0) {
				LOG_INF("interface: %s is down", if_name);
			}
		}
	}
}

/************************************************************************
 * OTBR init Networking services
 ************************************************************************/

static int otbr_init_process(void)
{
	otbr_init_network_init();
	otbr_init_start();
	return 0;
}

SYS_INIT(otbr_init_process, APPLICATION, CONFIG_TELINK_OTBR_INIT_PRIO);
