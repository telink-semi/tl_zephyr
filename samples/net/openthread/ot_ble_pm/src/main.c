/* main.c - OpenThread */

/*
 * Copyright (c) 2023-2026 Telink
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/devicetree.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(ot_main, LOG_LEVEL_DBG);

#include <zephyr/net/openthread.h>
#include <openthread/thread.h>
#include <zephyr/bluetooth/bluetooth.h>

#include "tlx_bt_init.h"

#ifdef CONFIG_IEEE802154_TLX_BLE_COEXIST
/* Stop insert task + BLE adv after a delay following Thread join; 802.15.4 takes over RF */
static struct k_work_delayable post_join_dwork;

static void post_join_dwork_handler(struct k_work *work)
{
	otDeviceRole role = otThreadGetDeviceRole(openthread_get_default_context()->instance);

	/* Re-check the device is still attached (child/router) after the delay, avoid stopping BLE while detached */
	if (role != OT_DEVICE_ROLE_CHILD) {
		LOG_INF("Not in Thread network anymore, skip post-join");
		return;
	}

	LOG_INF("Thread joined 30s: stop insert task & BLE, handover RF to 802.15.4");
	tlx_bt_802154_post_join();
	bt_le_adv_stop();
}
#endif /* CONFIG_IEEE802154_TLX_BLE_COEXIST */

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
		otDeviceRole role = otThreadGetDeviceRole(ot_context->instance);

		/* Joined as child/router, run once: schedule stopping insert task + BLE adv after a delay */
		if (role == OT_DEVICE_ROLE_CHILD) {
#ifdef CONFIG_IEEE802154_TLX_BLE_COEXIST
			LOG_INF("Thread joined: schedule post-join & stop adv in 30s");
			k_work_schedule(&post_join_dwork, K_SECONDS(10));
#endif
		}

		switch (role) {
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
	// LOG_INF("***** OpenThread SED joiner @ F_CPU = %u *****",
	// 	(DT_PROP(DT_PATH(cpus, cpu_0), clock_frequency)));
	// LOG_INF("OT channel     %u",     CONFIG_OPENTHREAD_CHANNEL);
	// LOG_INF("OT pan id      %04x",   CONFIG_OPENTHREAD_PANID);
	// LOG_INF("OT pan ext id  %s",     CONFIG_OPENTHREAD_XPANID);
	// LOG_INF("OT network key %s",     CONFIG_OPENTHREAD_NETWORKKEY);
	LOG_INF("***** OpenThread + BLE Peripheral *****");

	static struct openthread_state_changed_cb ot_state_cahnge = {
		.state_changed_cb = ot_satate_changed
	};

	openthread_state_changed_cb_register(openthread_get_default_context(), &ot_state_cahnge);

#ifdef CONFIG_IEEE802154_TLX_BLE_COEXIST
	k_work_init_delayable(&post_join_dwork, post_join_dwork_handler);

	extern void bt_le_task_init(void);
	bt_le_task_init();
#endif

	return 0;
}
