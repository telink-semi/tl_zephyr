/*
 * Copyright (c) 2025 Telink Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "otbr_srp.h"
#include "otbr_mdns_fmt.h"
#include <openthread/srp_server.h>
#include <openthread/border_routing.h>

#define LOG_LEVEL CONFIG_TELINK_OTBR_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(otbr_services);

const otIp6Address *otbr_srp_get_host_addrss(struct openthread_context *aContext,
					     const otSrpServerHost *aHost)
{
	const otIp6Address *addr = NULL;
	otIp6Prefix omr_prefix;

	if (otBorderRoutingGetOmrPrefix(aContext->instance, &omr_prefix) == OT_ERROR_NONE) {
		uint8_t addr_num;
		const otIp6Address *addr_host = otSrpServerHostGetAddresses(aHost, &addr_num);

		for (uint8_t i = 0; i < addr_num; i++) {
			if (!memcmp(&omr_prefix.mPrefix, &addr_host[i],
				    (omr_prefix.mLength + 7) / 8)) {
				addr = &addr_host[i];
				break;
			}
		}
	}
	return addr;
}

bool otbr_srp_search_service(struct openthread_context *aContext, const char *service_name,
			     const otSrpServerService **aService)
{
	bool found = false;

	aService ? *aService = NULL : 0;
	if (!aContext || !service_name) {
		return found;
	}

	const otSrpServerHost *host = otSrpServerGetNextHost(aContext->instance, NULL);

	while (!found && host) {
		if (!otSrpServerHostIsDeleted(host)) {
			const otSrpServerService *service =
				otSrpServerHostGetNextService(host, NULL);

			while (!found && service) {
				if (!otSrpServerServiceIsDeleted(service)) {
					const char *instance_name = otSrpServerServiceGetInstanceName(service);

					if (instance_name && !otbr_mdns_fmt_strcasecmp(instance_name, service_name)) {
						aService ? *aService = service : 0;
						found = true;
					}
				}
				service = otSrpServerHostGetNextService(host, service);
			}
		}
		host = otSrpServerGetNextHost(aContext->instance, host);
	}
	if (found) {
		LOG_DBG("srp service %s found", service_name);
	}
	return found;
}

bool otbr_srp_search_host(struct openthread_context *aContext, const char *host_name,
			  const otSrpServerHost **aHost)
{
	bool found = false;

	aHost ? *aHost = NULL : 0;
	if (!aContext || !host_name) {
		return found;
	}

	const otSrpServerHost *host = otSrpServerGetNextHost(aContext->instance, NULL);

	while (!found && host) {
		if (!otSrpServerHostIsDeleted(host)) {
			const char *server_name = otSrpServerHostGetFullName(host);

			if (server_name && !otbr_mdns_fmt_strcasecmp(server_name, host_name)) {
				aHost ? *aHost = host : 0;
				found = true;
			}
		}
		host = otSrpServerGetNextHost(aContext->instance, host);
	}
	if (found) {
		LOG_DBG("srp host %s found", host_name);
	}
	return found;
}

static void otbr_srp_on_srp_update(otSrpServerServiceUpdateId aId, const otSrpServerHost *aHost,
				   uint32_t aTimeout, void *aContext)
{
	struct openthread_context *ctx = (struct openthread_context *)aContext;

	otSrpServerHandleServiceUpdateResult(ctx->instance, aId, OT_ERROR_NONE);

	const otIp6Address *host_addr = otbr_srp_get_host_addrss(ctx, aHost);
	char host_addr_str[OT_IP6_ADDRESS_STRING_SIZE];

	if (host_addr) {
		otIp6AddressToString(host_addr, host_addr_str, sizeof(host_addr_str));
	} else {
		host_addr_str[0] = '\0';
	}

	const otSrpServerService *service = otSrpServerHostGetNextService(aHost, NULL);

	while (service) {
		LOG_DBG("srp service %s: '%s' %s",
			otSrpServerServiceIsDeleted(service) ? "delete" : "publish",
			otSrpServerServiceGetInstanceName(service), host_addr_str);
		service = otSrpServerHostGetNextService(aHost, service);
	}
}

void otbr_srp_init(struct openthread_context *aContext)
{
	otSrpServerSetServiceUpdateHandler(aContext->instance, otbr_srp_on_srp_update, aContext);
}
