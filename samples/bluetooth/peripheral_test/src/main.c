/* main.c - Application main entry point */

/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "zephyr/bluetooth/gap.h"
#include <zephyr/types.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/kernel.h>

#include <zephyr/settings/settings.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/gatt.h>

#define DEVICE_NAME     CONFIG_BT_DEVICE_NAME
#define DEVICE_NAME_LEN (sizeof(DEVICE_NAME) - 1)

static const struct bt_data ad[] = {
	BT_DATA(BT_DATA_NAME_COMPLETE, CONFIG_BT_DEVICE_NAME, DEVICE_NAME_LEN),
	BT_DATA_BYTES(BT_DATA_GAP_APPEARANCE, BT_BYTES_LIST_LE16(BT_APPEARANCE_GENERIC_REMOTE)),
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
};

static const struct bt_data sd[] = {
	BT_DATA(BT_DATA_NAME_COMPLETE, CONFIG_BT_DEVICE_NAME, DEVICE_NAME_LEN),
};

static struct bt_conn *default_conn;

K_THREAD_STACK_DEFINE(conn_wq_stack, 1024);
static struct k_work_q conn_wq;
static struct k_work_delayable conn_param_work;

static void conn_param_work_handler(struct k_work *work)
{
	if (!default_conn) {
		return;
	}

	const struct bt_le_conn_param param = {
		.interval_min = 6, /* 30 ms */
		.interval_max = 6, /* 50 ms */
		.latency = 0,
		.timeout = 400,
	};
	int ret = bt_conn_le_param_update(default_conn, &param);

	printk("conn param update ret = %d\n", ret);
}

static void connected(struct bt_conn *conn, uint8_t err)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	if (err) {
		printk("Failed to connect to %s (%u)\n", addr, err);
		return;
	}

	printk("Connected %s\n", addr);

	/* int res = bt_conn_set_security(conn, BT_SECURITY_L2); */
	/* printk("set security err: %d\n", res); */

	if (!default_conn) {
		default_conn = bt_conn_ref(conn);
	}
	k_work_schedule_for_queue(&conn_wq, &conn_param_work, K_SECONDS(2));
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	printk("Disconnected from %s (reason 0x%02x)\n", addr, reason);

	k_work_cancel_delayable(&conn_param_work);
	if (default_conn) {
		bt_conn_unref(default_conn);
		default_conn = NULL;
	}
}

static void security_changed(struct bt_conn *conn, bt_security_t level, enum bt_security_err err)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	if (!err) {
		printk("Security changed: %s level %u\n", addr, level);
	} else {
		printk("Security failed: %s level %u err %d\n", addr, level, err);
	}
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
	.connected = connected,
	.disconnected = disconnected,
	.security_changed = security_changed,
};

static void bt_ready(int err)
{
	if (err) {
		printk("Bluetooth init failed (err %d)\n", err);
		return;
	}

	printk("Bluetooth initialized\n");

	if (IS_ENABLED(CONFIG_SETTINGS)) {
		settings_load();
	}

	err = bt_le_adv_start(
		((struct bt_le_adv_param[]){{
			.id = 0,
			.sid = 0,
			.secondary_max_skip = 0,
			.options = (BT_LE_ADV_OPT_CONNECTABLE | BT_LE_ADV_OPT_FORCE_NAME_IN_AD),
			.interval_min = (0x00a0),
			.interval_max = (0x00f0),
			.peer = (((void *)0)),
		}}),
		ad, ARRAY_SIZE(ad), sd, ARRAY_SIZE(sd));
	if (err) {
		printk("Advertising failed to start (err %d)\n", err);
		return;
	}

	printk("Advertising successfully started\n");
}

static void auth_cancel(struct bt_conn *conn)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	printk("Pairing cancelled: %s\n", addr);
}

static struct bt_conn_auth_cb auth_cb_display = {
	.passkey_display = NULL,
	.passkey_entry = NULL,
	.cancel = auth_cancel,
};

int main(void)
{
	int err;

	k_work_queue_start(&conn_wq, conn_wq_stack, K_THREAD_STACK_SIZEOF(conn_wq_stack),
			   K_PRIO_PREEMPT(8), NULL);
	k_work_init_delayable(&conn_param_work, conn_param_work_handler);

	err = bt_enable(bt_ready);
	if (err) {
		printk("Bluetooth init failed (err %d)\n", err);
		return 0;
	}

	if (IS_ENABLED(CONFIG_SAMPLE_BT_USE_AUTHENTICATION)) {
		bt_conn_auth_cb_register(&auth_cb_display);
		printk("Bluetooth authentication callbacks registered.\n");
	}

	while (1) {
		k_sleep(K_MSEC(10));
	}

	return 0;
}
