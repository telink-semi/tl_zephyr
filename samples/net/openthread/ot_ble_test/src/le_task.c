/*
 * Copyright (c) 2023-2026 Telink
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/settings/settings.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(ot_ble_le_task, LOG_LEVEL_INF);

#include "thd_task.h"

/* Channel Sounding RAS Reflector integration */
#define BT_UUID_RAS_SERVICE_VAL 0x185B

#if CONFIG_BT_TLX_CHANNEL_SOUNDING
extern void cs_reflector_init(void);
extern void cs_reflector_on_connected(struct bt_conn *conn, uint8_t err);
extern void cs_reflector_on_disconnected(struct bt_conn *conn);
#endif

/* ============================================================
 * BLE Peripheral section
 * ============================================================
 */

static const struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
	BT_DATA_BYTES(BT_DATA_UUID16_SOME, BT_UUID_RAS_SERVICE_VAL & 0xFF,
		      (BT_UUID_RAS_SERVICE_VAL >> 8) & 0xFF),
};

static const struct bt_data sd[] = {
	BT_DATA(BT_DATA_NAME_COMPLETE, CONFIG_BT_DEVICE_NAME, sizeof(CONFIG_BT_DEVICE_NAME) - 1),
};

static void connected(struct bt_conn *conn, uint8_t err)
{
	if (err) {
		printk("Connection failed, err 0x%02x %s\n", err, bt_hci_err_to_str(err));
	} else {
		printk("Connected\n");
	}
#if CONFIG_BT_TLX_CHANNEL_SOUNDING
	cs_reflector_on_connected(conn, err);
#endif
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	printk("Disconnected, reason 0x%02x %s\n", reason, bt_hci_err_to_str(reason));
#if CONFIG_BT_TLX_CHANNEL_SOUNDING
	cs_reflector_on_disconnected(conn);
#endif
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
	.connected = connected,
	.disconnected = disconnected,
};

static void bt_ready(void)
{
	int err;

	printk("Bluetooth initialized\n");

	err = bt_le_adv_start(BT_LE_ADV_CONN_FAST_1, ad, ARRAY_SIZE(ad), sd, ARRAY_SIZE(sd));
	if (err) {
		printk("Advertising failed to start (err %d)\n", err);
		return;
	}

	printk("Advertising successfully started\n");
}

/*
 * static void auth_cancel(struct bt_conn *conn)
 * {
 *	char addr[BT_ADDR_LE_STR_LEN];
 *
 *	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));
 *
 *	printk("Pairing cancelled: %s\n", addr);
 * }
 *
 * static struct bt_conn_auth_cb auth_cb_display = {
 *	.cancel = auth_cancel,
 * }
 */

void bt_le_task_init(void)
{
	int err;

	err = bt_enable(NULL);
	if (err) {
		printk("Bluetooth init failed (err %d)\n", err);
		return;
	}

	/* Load bonded keys from flash so reconnects skip re-pairing. */
	if (IS_ENABLED(CONFIG_SETTINGS)) {
		settings_load();
	}
#if CONFIG_BT_TLX_CHANNEL_SOUNDING
	/* Initialize Channel Sounding RAS Reflector */
	cs_reflector_init();
#endif
	bt_ready();

	/* bt_conn_auth_cb_register(&auth_cb_display); */

	while (1) {
		k_sleep(K_SECONDS(1));
		static int i;

		if (i == 0) {
			i = 1;
			/* tlksdk_thd_enableFlexibleTask(THD_TASK_ENABLE); */
			tlksdk_thd_enableInsertTask1(0x01);
		}
	}
}
