/*
 * Copyright (c) 2026 Telink Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <stdio.h>
#include <stddef.h>
#include <errno.h>
#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/types.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/services/bas.h>

static int scan_start(void);

static uint8_t indicating;
static uint8_t simulate_htm;
static struct bt_gatt_indicate_params ind_params;
static double temperature;

static struct bt_conn *central_conn;
static struct bt_conn *peripheral_conn;
static struct bt_uuid_16 discover_uuid = BT_UUID_INIT_16(0);
static struct bt_gatt_discover_params discover_params;
static struct bt_gatt_subscribe_params subscribe_params;

static const struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
	BT_DATA_BYTES(BT_DATA_UUID16_ALL,
		      BT_UUID_16_ENCODE(BT_UUID_HTS_VAL),
		      BT_UUID_16_ENCODE(BT_UUID_DIS_VAL),
		      BT_UUID_16_ENCODE(BT_UUID_BAS_VAL)),
};

static const struct bt_data sd[] = {
	BT_DATA(BT_DATA_NAME_COMPLETE, CONFIG_BT_DEVICE_NAME, sizeof(CONFIG_BT_DEVICE_NAME) - 1),
};

static double pow_val(double x, double y)
{
	double result = 1;

	if (y < 0) {
		y = -y;
		while (y--) {
			result /= x;
		}
	} else {
		while (y--) {
			result *= x;
		}
	}

	return result;
}

static uint8_t notify_func(struct bt_conn *conn, struct bt_gatt_subscribe_params *params,
			   const void *data, uint16_t length)
{
	double get_temperature;
	uint32_t mantissa;
	int8_t exponent;

	if (!data) {
		printk("[UNSUBSCRIBED]\n");
		params->value_handle = 0U;
		return BT_GATT_ITER_STOP;
	}

	/* temperature value display */
	mantissa = sys_get_le24(&((uint8_t *)data)[1]);
	exponent = ((uint8_t *)data)[4];
	get_temperature = (double)mantissa * pow_val(10, exponent);

	printf("Get temperature %gC.\n", get_temperature);

	temperature = get_temperature;

	return BT_GATT_ITER_CONTINUE;
}

static uint8_t discover_func(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			     struct bt_gatt_discover_params *params)
{
	int err;

	if (!attr) {
		printk("Discover complete\n");
		(void)memset(params, 0, sizeof(*params));
		return BT_GATT_ITER_STOP;
	}

	printk("[ATTRIBUTE] handle %u\n", attr->handle);

	if (!bt_uuid_cmp(discover_params.uuid, BT_UUID_HTS)) {
		memcpy(&discover_uuid, BT_UUID_HTS_MEASUREMENT, sizeof(discover_uuid));
		discover_params.uuid = &discover_uuid.uuid;
		discover_params.start_handle = attr->handle + 1;
		discover_params.type = BT_GATT_DISCOVER_CHARACTERISTIC;

		err = bt_gatt_discover(conn, &discover_params);
		if (err) {
			printk("Discover failed (err %d)\n", err);
		}
	} else if (!bt_uuid_cmp(discover_params.uuid, BT_UUID_HTS_MEASUREMENT)) {
		memcpy(&discover_uuid, BT_UUID_GATT_CCC, sizeof(discover_uuid));
		discover_params.uuid = &discover_uuid.uuid;
		discover_params.start_handle = attr->handle + 2;
		discover_params.type = BT_GATT_DISCOVER_DESCRIPTOR;
		subscribe_params.value_handle = bt_gatt_attr_value_handle(attr);

		err = bt_gatt_discover(conn, &discover_params);
		if (err) {
			printk("Discover failed (err %d)\n", err);
		}
	} else {
		subscribe_params.notify = notify_func;
		subscribe_params.value = BT_GATT_CCC_INDICATE;
		subscribe_params.ccc_handle = attr->handle;

		err = bt_gatt_subscribe(conn, &subscribe_params);
		if (err && err != -EALREADY) {
			printk("Subscribe failed (err %d)\n", err);
		} else {
			printk("[SUBSCRIBED]\n");
		}

		return BT_GATT_ITER_STOP;
	}

	return BT_GATT_ITER_STOP;
}

static void connected(struct bt_conn *conn, uint8_t conn_err)
{
	int err = 0;
	char addr[BT_ADDR_LE_STR_LEN];
	struct bt_conn_info info;

	bt_conn_get_info(conn, &info);

	if (info.role == BT_CONN_ROLE_CENTRAL) {
		printk("Connection type: Master %d\r\n", info.role);

		bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

		if (conn_err) {
			printk("Failed to connect to %s (%u)\n", addr, conn_err);

			bt_conn_unref(central_conn);
			central_conn = NULL;

			scan_start();
			return;
		}

		printk("Connected: %s\n", addr);

		if (conn == central_conn) {
			memcpy(&discover_uuid, BT_UUID_HTS, sizeof(discover_uuid));
			discover_params.uuid = &discover_uuid.uuid;
			discover_params.func = discover_func;
			discover_params.start_handle = BT_ATT_FIRST_ATTRIBUTE_HANDLE;
			discover_params.end_handle = BT_ATT_LAST_ATTRIBUTE_HANDLE;
			discover_params.type = BT_GATT_DISCOVER_PRIMARY;

			err = bt_gatt_discover(central_conn, &discover_params);
			if (err) {
				printk("Discover failed(err %d)\n", err);
				return;
			}
		}
	} else {
		printk("connection type slave\r\n");

		if (conn_err) {
			printk("Connection failed (err %u)\n", conn_err);
		} else {
			peripheral_conn = bt_conn_ref(conn);
			printk("Connected\n");
		}
	}
}

static void device_found(const bt_addr_le_t *addr, int8_t rssi, uint8_t type,
			 struct net_buf_simple *ad)
{
	char addr_str[BT_ADDR_LE_STR_LEN];
	int err;

	if (central_conn) {
		return;
	}

	/* We're only interested in connectable events */
	if (type != BT_GAP_ADV_TYPE_ADV_IND &&
	    type != BT_GAP_ADV_TYPE_ADV_DIRECT_IND) {
		return;
	}

	bt_addr_le_to_str(addr, addr_str, sizeof(addr_str));

	/* connect only to devices in close proximity */
	if (rssi < -50) {
		return;
	}

	uint8_t addr_filter[BT_ADDR_SIZE] = {0xAC, 0x61, 0x6B, 0xF5, 0xBF, 0xC4};
	if(memcmp(addr->a.val, addr_filter, BT_ADDR_SIZE) != 0) {
		return;
	}

	printk("Device found: %s (RSSI %d)\n", addr_str, rssi);

	if (bt_le_scan_stop()) {
		return;
	}

	err = bt_conn_le_create(addr, BT_CONN_LE_CREATE_CONN,
				BT_LE_CONN_PARAM_DEFAULT, &central_conn);
	if (err) {
		printk("Create conn to %s failed (%d)\n", addr_str, err);
		scan_start();
	}
}

static int scan_start(void)
{
	/* Use active scanning and disable duplicate filtering to handle any
	 * devices that might update their advertising data at runtime.
	 */
	struct bt_le_scan_param scan_param = {
		.type       = BT_LE_SCAN_TYPE_ACTIVE,
		.options    = BT_LE_SCAN_OPT_NONE,
		.interval   = BT_GAP_SCAN_FAST_INTERVAL,
		.window     = BT_GAP_SCAN_FAST_WINDOW,
	};

	return bt_le_scan_start(&scan_param, device_found);
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	char addr[BT_ADDR_LE_STR_LEN];
	int err;

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	printk("Disconnected: %s, reason 0x%02x %s\n", addr, reason, bt_hci_err_to_str(reason));

	if (conn == central_conn) {
		bt_conn_unref(central_conn);
		central_conn = NULL;

		err = scan_start();
		if (err) {
			printk("Scanning failed to start (err %d)\n", err);
		}
	} else if (conn == peripheral_conn) {
		bt_conn_unref(peripheral_conn);
		peripheral_conn = NULL;
	}
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

	err = scan_start();
	if (err) {
		printk("Scanning failed to start (err %d)\n", err);
		return;
	}
	printk("Scanning successfully started\n");
}

static void auth_cancel(struct bt_conn *conn)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	printk("Pairing cancelled: %s\n", addr);
}

static struct bt_conn_auth_cb auth_cb_display = {
	.cancel = auth_cancel,
};

static void htmc_ccc_cfg_changed(const struct bt_gatt_attr *attr, uint16_t value)
{
	simulate_htm = (value == BT_GATT_CCC_INDICATE) ? 1 : 0;
}

static void indicate_cb(struct bt_conn *conn,
			struct bt_gatt_indicate_params *params, uint8_t err)
{
	printk("Indication %s\n", err != 0U ? "fail" : "success");
}

static void indicate_destroy(struct bt_gatt_indicate_params *params)
{
	indicating = 0U;
}

/* Health Thermometer Service Declaration */
BT_GATT_SERVICE_DEFINE(hts_svc, BT_GATT_PRIMARY_SERVICE(BT_UUID_HTS),
	BT_GATT_CHARACTERISTIC(BT_UUID_HTS_MEASUREMENT, BT_GATT_CHRC_INDICATE,
	BT_GATT_PERM_NONE, NULL, NULL, NULL),
	BT_GATT_CCC(htmc_ccc_cfg_changed, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),
	/* more optional Characteristics */
);

static void hts_indicate(void)
{
	static uint8_t htm[5];
	uint32_t mantissa;
	uint8_t exponent;

	if (!simulate_htm || indicating) {
		return;
	}

	printf("Send temperature is %gC\n", temperature);

	mantissa = (uint32_t)(temperature * 100);
	exponent = (uint8_t)-2;

	htm[0] = 0; /* temperature in celsius */
	sys_put_le24(mantissa, (uint8_t *)&htm[1]);
	htm[4] = exponent;

	ind_params.attr = &hts_svc.attrs[2];
	ind_params.func = indicate_cb;
	ind_params.destroy = indicate_destroy;
	ind_params.data = &htm;
	ind_params.len = sizeof(htm);

	if (bt_gatt_indicate(NULL, &ind_params) == 0) {
		indicating = 1U;
	}
}

static void bas_notify(void)
{
	uint8_t battery_level = bt_bas_get_battery_level();

	battery_level--;

	if (!battery_level) {
		battery_level = 100U;
	}

	bt_bas_set_battery_level(battery_level);
}


int main(void)
{
	int err;

	err = bt_enable(NULL);
	if (err) {
		printk("Bluetooth init failed (err %d)\n", err);
		return 0;
	}

	printk("Bluetooth initialized\n");

	bt_ready();

	bt_conn_auth_cb_register(&auth_cb_display);

	/* Implement indicate. At the moment there is no suitable way
	 * of starting delayed work so we do it here
	 */
	while (1) {
		k_sleep(K_SECONDS(1));

		/* Temperature measurements simulation */
		hts_indicate();

		/* Battery level simulation */
		bas_notify();
	}
	return 0;
}
