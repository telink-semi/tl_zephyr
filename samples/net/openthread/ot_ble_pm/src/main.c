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
#include <zephyr/settings/settings.h>

#include "tlx_bt_init.h"
#include "thd_task.h"

#ifdef CONFIG_IEEE802154_TLX_BLE_COEXIST

static struct k_work_delayable post_join_dwork;
static struct k_work_delayable ble_enable_dwork;

extern volatile bool tlx_rf_802154_mode;

volatile bool isThreadCommissioned = false;

/* BLE control state */
enum ble_ctrl_state {
	BLE_STATE_COEXIST, /* default after boot: Thread + BLE time-share RF */
	BLE_STATE_OFF,     /* BLE off: 802.15.4 owns RF exclusively */
	BLE_STATE_BUSY,    /* switching, prevent re-entry */
};

static enum ble_ctrl_state ble_state = BLE_STATE_COEXIST;

/* Disable BLE: hand RF over to 802.15.4 (the original dual_mode_suspend logic) */
static void ble_disable(void)
{
	tlx_bt_802154_dual_mode_disable();

	/* Wait for RF to park back to BLE mode (timeout 1000 ms) */
	for (uint32_t waited_ms = 0; tlx_rf_802154_mode; waited_ms += 10) {
		if (waited_ms >= 1000) {
			LOG_ERR("RF failed to park in BLE mode, skip BLE teardown");
			ble_state = BLE_STATE_COEXIST;
			return;
		}
		k_sleep(K_MSEC(10));
	}

	bt_le_adv_stop();

	if (bt_disable()) {
		LOG_ERR("bt_disable failed, continue with 802.15.4 revival");
	}

	tlx_bt_802154_dual_mode_suspend();

	ble_state = BLE_STATE_OFF;
	LOG_INF("BLE off: RF owned by 802.15.4");

	/* Test: auto re-enable BLE after 5s to verify coexistence restore */
	k_work_schedule(&ble_enable_dwork, K_SECONDS(5));
}

extern void bt_le_adv_restart(void);

/* Enable BLE: restore Thread + BLE coexistence */
static void ble_enable(void)
{
	/* 1. Revert 802.15.4 ownership and park back to BLE mode */
	tlx_bt_802154_dual_mode_resume();

	/* 2. Re-init BLE host+controller:
	 *    hci_tlx_open -> tlx_bt_controller_init()
	 *      -> rf_radio_reset() + rf_drv_ble_init()   (RF back to BLE)
	 *      -> tlx_bt_802154_dual_mode_enable()       (re-register mode-change/ISR callbacks)
	 *      -> tlx_bt_irq_init()                      (reconnect IRQ_ZB_RT to rf_irq_handler)
	 */
	int err = bt_enable(NULL);
	if (err) {
		LOG_ERR("bt_enable failed (%d), rollback to 802.15.4", err);
		tlx_bt_802154_dual_mode_suspend();
		ble_state = BLE_STATE_OFF;
		return;
	}

	/* Reload bonded keys (bt_disable() clears them via bt_keys_reset) */
	if (IS_ENABLED(CONFIG_SETTINGS)) {
		settings_load();
	}

	/* 3. Restart advertising */
	bt_le_adv_restart();

	/* 4. Restore insert-task time-sharing: 802.15.4 runs in the BLE scheduler again.
	 *    The switch_to_802154_mode() callback then resumes OT threads + gives ready_sem */
	tlksdk_thd_enableInsertTask1(0x01);

	ble_state = BLE_STATE_COEXIST;
	LOG_INF("BLE on: Thread + BLE coexist");
}

/* Public interface: enable/disable BLE by parameter */
void ble_ctrl_set(bool enable)
{
	bool is_on = (ble_state == BLE_STATE_COEXIST);

	if (enable == is_on || ble_state == BLE_STATE_BUSY) {
		return;
	}

	ble_state = BLE_STATE_BUSY;

	if (enable) {
		ble_enable();
	} else {
		ble_disable();
	}
}

/* Test: auto call ble_enable 5s after ble_disable to verify coexistence restore */
static void ble_enable_dwork_handler(struct k_work *work)
{
	LOG_INF("Test: re-enable BLE to verify coexistence restore");
	ble_ctrl_set(true);
}

static void post_join_dwork_handler(struct k_work *work)
{
	otDeviceRole role = otThreadGetDeviceRole(openthread_get_default_context()->instance);

	/* Re-check the device is still attached (child) after the delay, avoid stopping BLE while detached */
	if (role != OT_DEVICE_ROLE_CHILD) {
		LOG_INF("Not in Thread network anymore, skip post-join");
		return;
	}

	LOG_INF("Thread joined: stop BLE entirely, handover RF to 802.15.4");

	ble_ctrl_set(false);
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
			k_work_schedule(&post_join_dwork, K_SECONDS(5));
			isThreadCommissioned = true;
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
	k_work_init_delayable(&ble_enable_dwork, ble_enable_dwork_handler);

	extern void bt_le_task_init(void);
	bt_le_task_init();
#endif

	return 0;
}
