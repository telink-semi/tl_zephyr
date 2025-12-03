/* main.c - OpenThread */

/*
 * Copyright (c) 2023-2024 Telink
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(ot_main, LOG_LEVEL_DBG);

#include <zephyr/net/openthread.h>
#include <openthread/thread.h>

static void ot_show_ip6_addr(otInstance *inst)
{
	const otNetifAddress *ip6_addr = otIp6GetUnicastAddresses(inst);

	for (const otNetifAddress *addr = ip6_addr; addr; addr = addr->mNext) {
		if (addr->mValid) {
			char ip6_str[OT_IP6_ADDRESS_STRING_SIZE];

			otIp6AddressToString(&addr->mAddress, ip6_str, sizeof(ip6_str));
			LOG_INF("ip: %s", ip6_str);
		}
	}
}

static void ot_satate_changed(otChangedFlags flags,
	struct openthread_context *ot_context, void *user_data)
{
	if (flags & OT_CHANGED_THREAD_ROLE) {
		switch (otThreadGetDeviceRole(ot_context->instance)) {
		case OT_DEVICE_ROLE_CHILD:
			LOG_INF("OT child");
			LOG_INF("OT Short address: %04x",
				otLinkGetShortAddress(ot_context->instance));
			LOG_HEXDUMP_INF(otLinkGetExtendedAddress(ot_context->instance),
				OT_EXT_ADDRESS_SIZE, "OT Extended address:");
			ot_show_ip6_addr(ot_context->instance);
			break;
		case OT_DEVICE_ROLE_ROUTER:
			LOG_INF("OT router");
			LOG_INF("OT Short address: %04x",
				otLinkGetShortAddress(ot_context->instance));
			LOG_HEXDUMP_INF(otLinkGetExtendedAddress(ot_context->instance),
				OT_EXT_ADDRESS_SIZE, "OT Extended address:");
			ot_show_ip6_addr(ot_context->instance);
			break;
		case OT_DEVICE_ROLE_LEADER:
			LOG_INF("OT leader");
			LOG_INF("OT Short address: %04x",
				otLinkGetShortAddress(ot_context->instance));
			LOG_HEXDUMP_INF(otLinkGetExtendedAddress(ot_context->instance),
				OT_EXT_ADDRESS_SIZE, "OT Extended address:");
			ot_show_ip6_addr(ot_context->instance);
			break;
		case OT_DEVICE_ROLE_DISABLED:
			LOG_INF("OT disabled");
			break;
		case OT_DEVICE_ROLE_DETACHED:
			LOG_INF("OT detached");
			break;
		default:
			LOG_INF("OT unknown");
			break;
		}
	}
}

int main(void)
{
	LOG_INF("***** OpenThread CLI on Zephyr *****");
	LOG_INF("OT channel     %u",     CONFIG_OPENTHREAD_CHANNEL);
	LOG_INF("OT pan id      %04x",   CONFIG_OPENTHREAD_PANID);
	LOG_INF("OT pan ext id  %s",     CONFIG_OPENTHREAD_XPANID);
	LOG_INF("OT network key %s",     CONFIG_OPENTHREAD_NETWORKKEY);

	static struct openthread_state_changed_cb ot_state_cahnge = {
		.state_changed_cb = ot_satate_changed
	};

	openthread_state_changed_cb_register(openthread_get_default_context(), &ot_state_cahnge);

	return 0;
}
