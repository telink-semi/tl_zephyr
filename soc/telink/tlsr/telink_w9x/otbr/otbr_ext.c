/*
 * Copyright (c) 2025 Telink Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "otbr_ext.h"
#include "otbr_icmpv6.h"

#include <openthread/thread.h>
#include <openthread/platform/infra_if.h>
#include <openthread/border_routing.h>
#include <openthread/dataset_ftd.h>

#include <zephyr/net/net_if.h>

/* warning: currently zephyr has no external routing interface */
#include <../subsys/net/ip/ipv6.h>
#include <../subsys/net/ip/route.h>

#define LOG_LEVEL CONFIG_TELINK_W91_OTBR_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(otbr_ext);

/************************************************************************
 * OTBR Check configuration
 ************************************************************************/

#if !CONFIG_OPENTHREAD_MANUAL_START
#error openthread should be started manually
#endif /* !CONFIG_OPENTHREAD_MANUAL_START */

/************************************************************************
 * OTBR internal functions
 ************************************************************************/

static void otbr_infra_icmpv6_inp(int if_idx, const struct in6_addr *src, const void *icmp_data,
				  size_t icmp_data_length, void *ctx)
{
	struct otInstance *ot_instance = (struct otInstance *)ctx;

	otPlatInfraIfRecvIcmp6Nd(ot_instance, if_idx, (otIp6Address *)src, icmp_data,
				 icmp_data_length);
}

/************************************************************************
 * OTBR thread functionality
 ************************************************************************/

otError otPlatInfraIfSendIcmp6Nd(uint32_t aInfraIfIndex, const otIp6Address *aDestAddress,
				 const uint8_t *aBuffer, uint16_t aBufferLength)
{
	otError result = OT_ERROR_FAILED;
	struct in6_addr dest;

	memcpy(&dest, aDestAddress, sizeof(dest));
	if (!otbr_icmpv6_send(aInfraIfIndex, &dest, aBuffer, aBufferLength)) {
		result = OT_ERROR_NONE;
	} else {
		LOG_ERR("otbr send icmpv6 failed");
	}
	return result;
}

bool otPlatInfraIfHasAddress(uint32_t aInfraIfIndex, const otIp6Address *aAddress)
{
	bool result = false;
	struct net_if *iface = net_if_get_by_index(aInfraIfIndex);

	if (iface) {
		struct in6_addr addr;

		memcpy(&addr, aAddress, sizeof(addr));
		if (net_if_ipv6_addr_lookup_by_iface(iface, &addr)) {
			result = true;
		}
	}
	return result;
}

/************************************************************************
 * OTBR external functions
 ************************************************************************/

void otbr_ext_infra_up(struct openthread_context *aContext, uint32_t aInfraIfIndex)
{
	otError err = otBorderRoutingInit(aContext->instance, aInfraIfIndex, true);

	if (err == OT_ERROR_NONE) {
		otbr_icmpv6_start_listen(aInfraIfIndex, otbr_infra_icmpv6_inp, aContext->instance);
		err = otBorderRoutingSetEnabled(aContext->instance, true);
		if (err == OT_ERROR_NONE) {
			LOG_INF("otbr enabled");
		} else {
			LOG_ERR("otbr enabling failed %d", err);
		}
	} else {
		LOG_ERR("otbr init failed %d", err);
	}
}

void otbr_ext_infra_down(struct openthread_context *aContext, uint32_t aInfraIfIndex)
{
	otError err = otBorderRoutingSetEnabled(aContext->instance, false);

	otbr_icmpv6_stop_listen(aInfraIfIndex);

	if (err == OT_ERROR_NONE) {
		LOG_INF("obr disabled");
	} else {
		LOG_ERR("otbr disabling failed %d", err);
	}
}

void otbr_ext_thread_start(struct openthread_context *aContext)
{
	openthread_api_mutex_lock(aContext);
	if (!otDatasetIsCommissioned(aContext->instance)) {
		otOperationalDataset dataset;

		if (otDatasetCreateNewNetwork(aContext->instance, &dataset) == OT_ERROR_NONE) {
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
			if (otDatasetSetActive(aContext->instance, &dataset) != OT_ERROR_NONE) {
				LOG_ERR("ot dataset set failed");
			}
		} else {
			LOG_ERR("ot dataset failed");
		}
	}
	if (otIp6SetEnabled(aContext->instance, true) != OT_ERROR_NONE) {
		LOG_ERR("ot ipv6 failed");
	}
	if (otThreadSetEnabled(aContext->instance, true) != OT_ERROR_NONE) {
		LOG_ERR("ot start failed");
	}
	openthread_api_mutex_unlock(aContext);
}

bool otbr_ext_omr_ipaddr_show(struct openthread_context *aContext)
{
	bool result = false;
	otIp6Prefix omr_prefix;

	if (otBorderRoutingGetOmrPrefix(aContext->instance, &omr_prefix) == OT_ERROR_NONE) {
		const otNetifAddress *ipaddr = otIp6GetUnicastAddresses(aContext->instance);

		while (ipaddr) {
			if (ipaddr->mValid) {
				otIp6Prefix addr_prefix = {.mLength = ipaddr->mPrefixLength};

				memcpy(&addr_prefix.mPrefix, &ipaddr->mAddress,
				       sizeof(addr_prefix.mPrefix));
				if (otIp6ArePrefixesEqual(&omr_prefix, &addr_prefix)) {
					char ipaddr_str[OT_IP6_ADDRESS_STRING_SIZE];

					otIp6AddressToString(&ipaddr->mAddress, ipaddr_str,
							     sizeof(ipaddr_str));
					LOG_PRINTK("ot omr addr: %s\n", ipaddr_str);
					result = true;
				}
			}
			ipaddr = ipaddr->mNext;
		}
		if (result) {
			char omr_prefix_str[OT_IP6_PREFIX_STRING_SIZE];

			otIp6PrefixToString(&omr_prefix, omr_prefix_str, sizeof(omr_prefix_str));
			LOG_PRINTK("ot omr net : %s\n", omr_prefix_str);
		}
	}
	return result;
}

void otbr_ext_thread_dataset_show(struct openthread_context *aContext)
{
	otOperationalDatasetTlvs act_ds;

	if (otDatasetGetActiveTlvs(aContext->instance, &act_ds) == OT_ERROR_NONE) {
		char act_ds_str[act_ds.mLength * 2 + 1];

		bin2hex(act_ds.mTlvs, act_ds.mLength, act_ds_str, sizeof(act_ds_str));
		LOG_PRINTK("ot active dataset: %s\n", act_ds_str);
	} else {
		LOG_ERR("ot no dataset");
	}
}

void otbr_ext_apply_omr_route(struct openthread_context *aContext, uint32_t aInfraIfIndex)
{
	struct net_if *infra_if = net_if_get_by_index(aInfraIfIndex);

	if (aContext && infra_if) {
		otIp6Prefix omr_prefix;

		if (otBorderRoutingGetOmrPrefix(aContext->instance, &omr_prefix) == OT_ERROR_NONE) {
			struct in6_addr prefix;

			memcpy(&prefix, &omr_prefix.mPrefix, sizeof(prefix));

			struct in6_addr *infra_ip =
				net_if_ipv6_get_ll(infra_if, NET_ADDR_ANY_STATE);
			const struct net_linkaddr *infra_mac = net_if_get_link_addr(infra_if);
			struct in6_addr *ot_ip =
				net_if_ipv6_get_ll(aContext->iface, NET_ADDR_ANY_STATE);
			const struct net_linkaddr *ot_mac = net_if_get_link_addr(aContext->iface);

			if (!net_ipv6_nbr_add(aContext->iface, infra_ip, infra_mac, true,
					      NET_IPV6_NBR_STATE_REACHABLE)) {
				LOG_ERR("otbr ot neighbor failed");
			}
			if (!net_route_add(aContext->iface, &prefix, omr_prefix.mLength, infra_ip,
					   NET_IPV6_ND_INFINITE_LIFETIME,
					   NET_ROUTE_PREFERENCE_MEDIUM)) {
				LOG_ERR("otbr ot route failed");
			}
			if (!net_ipv6_nbr_add(infra_if, ot_ip, ot_mac, true,
					      NET_IPV6_NBR_STATE_REACHABLE)) {
				LOG_ERR("otbr infra neighbor failed");
			}
		} else {
			LOG_ERR("otbr no omr prefix");
		}
	} else {
		LOG_ERR("otbr omr route failed: ot_ctx=%p, infra_if=%p", aContext, infra_if);
	}
}
