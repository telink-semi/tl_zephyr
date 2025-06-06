/*
 * Copyright (c) 2025 Telink
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "connections.h"

#include <zephyr/init.h>
#include <zephyr/net/net_config.h>
#include <zephyr/net/wifi_mgmt.h>
#include <zephyr/net/openthread.h>
#include <openthread/dataset_ftd.h>
#include <openthread/border_routing.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(connections, LOG_LEVEL_INF);

/* Since currently there is no official support of net route */
#define NET_IPV6_ND_INFINITE_LIFETIME 0xFFFFFFFF
#define NET_ROUTE_PREFERENCE_MEDIUM   0x00

enum net_ipv6_nbr_state {
	NET_IPV6_NBR_STATE_INCOMPLETE,
	NET_IPV6_NBR_STATE_REACHABLE,
	NET_IPV6_NBR_STATE_STALE,
	NET_IPV6_NBR_STATE_DELAY,
	NET_IPV6_NBR_STATE_PROBE,
	NET_IPV6_NBR_STATE_STATIC,
};
extern struct net_nbr *net_ipv6_nbr_add(struct net_if *iface, const struct in6_addr *addr,
					const struct net_linkaddr *lladdr, bool is_router,
					enum net_ipv6_nbr_state state);
extern struct net_route_entry *net_route_add(struct net_if *iface, struct in6_addr *addr,
					     uint8_t prefix_len, struct in6_addr *nexthop,
					     uint32_t lifetime, uint8_t preference);
/* End of routing compatibility */

#if !CONFIG_OPENTHREAD_MANUAL_START
#error openthread should be started manually
#endif /* !CONFIG_OPENTHREAD_MANUAL_START */
#if !CONFIG_OPENTHREAD_FTD
#error openthread should be FTD
#endif /* CONFIG_OPENTHREAD_FTD */
#if !CONFIG_NET_DEFAULT_IF_ETHERNET
#error ethernet should be default interface
#endif /* !CONFIG_NET_DEFAULT_IF_ETHERNET */

static connections_wifi_changed connections_wifi_changed_cb;
static connections_thread_changed connections_thread_changed_cb;

static void wifi_connect(void)
{
	LOG_INF("wifi connecting to '%s'...", CONFIG_BR_WIFI_SSID);

	for (bool con_requested = false; !con_requested;) {
		struct wifi_connect_req_params connect_req_params = {
			.ssid = CONFIG_BR_WIFI_SSID,
			.ssid_length = strlen(CONFIG_BR_WIFI_SSID),
			.psk = CONFIG_BR_WIFI_PASSWORD,
			.psk_length = strlen(CONFIG_BR_WIFI_PASSWORD),
			.security = WIFI_SECURITY_TYPE_PSK};

		if (net_mgmt(NET_REQUEST_WIFI_CONNECT, net_if_get_default(), &connect_req_params,
			     sizeof(connect_req_params))) {
			con_requested = true;
		} else {
			k_msleep(100);
		}
	}
}

static void wifi_connection_changed(struct net_mgmt_event_callback *cb, uint32_t mgmt_event,
				    struct net_if *iface)
{
	switch (mgmt_event) {
	case NET_EVENT_WIFI_CONNECT_RESULT: {
		const struct wifi_status *status = (const struct wifi_status *)cb->info;

		if (!status->status) {
			if (connections_wifi_changed_cb) {
				connections_wifi_changed_cb(iface, true);
			}
		} else {
			LOG_ERR("wifi connection error %d", status->status);
		}
	} break;
	case NET_EVENT_WIFI_DISCONNECT_RESULT:
		if (connections_wifi_changed_cb) {
			connections_wifi_changed_cb(iface, false);
		}
		LOG_INF("wifi reconnect");
		wifi_connect();
		break;
	}
}

static void ot_connection_changed(otChangedFlags flags, struct openthread_context *ot_context,
				  void *user_data)
{
	ARG_UNUSED(user_data);

	if (flags & OT_CHANGED_THREAD_ROLE) {
		if (connections_thread_changed_cb) {
			connections_thread_changed_cb(ot_context->instance,
						      otThreadGetDeviceRole(ot_context->instance));
		}
	}
	if (flags & OT_CHANGED_THREAD_NETDATA) {
		otIp6Prefix omr_prefix;

		if (otBorderRoutingGetOmrPrefix(ot_context->instance, &omr_prefix) ==
		    OT_ERROR_NONE) {
			char omr_prefix_str[OT_IP6_PREFIX_STRING_SIZE];
			struct net_if *lan_if = (struct net_if *)user_data;

			otIp6PrefixToString(&omr_prefix, omr_prefix_str, sizeof(omr_prefix_str));
			LOG_INF("OMR prefix: %s, LAN if: %s", omr_prefix_str, lan_if->config.name);

			struct in6_addr *lan_ip = net_if_ipv6_get_ll(lan_if, NET_ADDR_ANY_STATE);
			const struct net_linkaddr *lan_mac = net_if_get_link_addr(lan_if);
			struct in6_addr pr;

			memcpy(&pr, &omr_prefix.mPrefix, sizeof(pr));

			if (!net_ipv6_nbr_add(ot_context->iface, lan_ip, lan_mac, true,
					      NET_IPV6_NBR_STATE_REACHABLE)) {
				LOG_ERR("ot can't add lan neighbor");
			}
			if (!net_route_add(ot_context->iface, &pr, omr_prefix.mLength, lan_ip,
					   NET_IPV6_ND_INFINITE_LIFETIME,
					   NET_ROUTE_PREFERENCE_MEDIUM)) {
				LOG_ERR("ot can't add lan route");
			}

			struct in6_addr *ot_ip =
				net_if_ipv6_get_ll(ot_context->iface, NET_ADDR_ANY_STATE);
			const struct net_linkaddr *ot_mac = net_if_get_link_addr(ot_context->iface);

			if (!net_ipv6_nbr_add(lan_if, ot_ip, ot_mac, true,
					      NET_IPV6_NBR_STATE_REACHABLE)) {
				LOG_ERR("lan can't add ot neighbor");
			}

			otOperationalDatasetTlvs active_dataset;

			if (otDatasetGetActiveTlvs(ot_context->instance, &active_dataset) ==
			    OT_ERROR_NONE) {
				char active_dataset_str[active_dataset.mLength * 2 + 1];

				bin2hex(active_dataset.mTlvs, active_dataset.mLength,
					active_dataset_str, sizeof(active_dataset_str));
				LOG_INF("active dataset: %s", active_dataset_str);
			} else {
				LOG_ERR("no dataset");
			}
		}
	}
}

void connections_init(connections_wifi_changed on_wifi, connections_thread_changed on_thread)
{
	static const struct in6_addr icmp_rs_addr = {.s6_addr = {0xff, 0x02, 0x00, 0x00, 0x00, 0x00,
								 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
								 0x00, 0x00, 0x00, 0x02}};

	net_if_ipv6_maddr_join(net_if_get_default(),
			       net_if_ipv6_maddr_add(net_if_get_default(), &icmp_rs_addr));

	connections_wifi_changed_cb = on_wifi;
	connections_thread_changed_cb = on_thread;

	static struct net_mgmt_event_callback wifi_callback;

	net_mgmt_init_event_callback(&wifi_callback, wifi_connection_changed,
				     NET_EVENT_WIFI_CONNECT_RESULT |
					     NET_EVENT_WIFI_DISCONNECT_RESULT);
	net_mgmt_add_event_callback(&wifi_callback);

	static struct openthread_state_changed_cb ot_callback = {.state_changed_cb =
									 ot_connection_changed};

	ot_callback.user_data = net_if_get_default();
	openthread_state_changed_cb_register(openthread_get_default_context(), &ot_callback);

	struct openthread_context *ot_context = openthread_get_default_context();

	openthread_api_mutex_lock(ot_context);
	if (!otDatasetIsCommissioned(ot_context->instance)) {
		otOperationalDataset dataset;

		if (otDatasetCreateNewNetwork(ot_context->instance, &dataset) == OT_ERROR_NONE) {
#ifdef CONFIG_OPENTHREAD_CHANNEL
			dataset.mChannel = CONFIG_OPENTHREAD_CHANNEL;
			dataset.mComponents.mIsChannelPresent = true;
#endif /* CONFIG_OPENTHREAD_CHANNEL */
#ifdef CONFIG_OPENTHREAD_PANID
			dataset.mPanId = CONFIG_OPENTHREAD_PANID;
			dataset.mComponents.mIsPanIdPresent = true;
#endif /* CONFIG_OPENTHREAD_PANID */
#ifdef CONFIG_OPENTHREAD_XPANID
			net_bytes_from_str(dataset.mExtendedPanId.m8,
					   sizeof(dataset.mExtendedPanId.m8),
					   (char *)CONFIG_OPENTHREAD_XPANID);
			dataset.mComponents.mIsExtendedPanIdPresent = true;
#endif /* CONFIG_OPENTHREAD_XPANID */
#ifdef CONFIG_OPENTHREAD_NETWORKKEY
			if (strlen(CONFIG_OPENTHREAD_NETWORKKEY)) {
				net_bytes_from_str(dataset.mNetworkKey.m8,
						   sizeof(dataset.mNetworkKey.m8),
						   (char *)CONFIG_OPENTHREAD_NETWORKKEY);
				dataset.mComponents.mIsNetworkKeyPresent = true;
			}
#endif /* CONFIG_OPENTHREAD_NETWORKKEY */
#ifdef CONFIG_OPENTHREAD_NETWORK_NAME
			strncpy(dataset.mNetworkName.m8, CONFIG_OPENTHREAD_NETWORK_NAME,
				OT_NETWORK_NAME_MAX_SIZE);
			dataset.mComponents.mIsNetworkNamePresent = true;
#endif /* CONFIG_OPENTHREAD_NETWORK_NAME */
			if (otDatasetSetActive(ot_context->instance, &dataset) != OT_ERROR_NONE) {
				LOG_ERR("set openthread dataset failed");
			}
		} else {
			LOG_ERR("init openthread dataset failed");
		}
	}
	if (otIp6SetEnabled(ot_context->instance, true) != OT_ERROR_NONE) {
		LOG_ERR("set openthread ipv6 failed");
	}
	if (otThreadSetEnabled(ot_context->instance, true) != OT_ERROR_NONE) {
		LOG_ERR("set openthread starting failed");
	}
	openthread_api_mutex_unlock(ot_context);

	if (net_if_is_wifi(net_if_get_default())) {
		wifi_connect();
	} else {
		LOG_ERR("wifi is not default interface");
	}
}

#if !CONFIG_NET_CONFIG_AUTO_INIT

static int init_application(void)
{
	const char *app_info = "Initializing network";

	STRUCT_SECTION_FOREACH(net_if, iface) {
		if (net_if_is_up(iface)) {
			(void)net_config_init_app(net_if_get_device(iface), app_info);
		} else {
			LOG_INF("interface: %s is down\n", iface->config.name);
		}
	}
	return 0;
}

SYS_INIT(init_application, APPLICATION, CONFIG_NET_CONFIG_INIT_PRIO);

#endif /* !CONFIG_NET_CONFIG_AUTO_INIT */
