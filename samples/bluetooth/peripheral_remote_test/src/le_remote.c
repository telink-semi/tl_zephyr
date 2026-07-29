/* le_remote.c - HID Service implementation for peripheral_remote_test
 *
 * Copyright (c) 2024 Telink Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Equivalent to Telink SDK ble_remote/app_att.c HID Service (L595-648).
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "zephyr/sys/byteorder.h"
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/device.h>

#include "zephyr_key_matrix.h"

/* HID report IDs (aligned with Telink hids.h) */
#define HID_REPORT_ID_KEYBOARD_INPUT        1
#define HID_REPORT_ID_CONSUME_CONTROL_INPUT 2

/* HID report types */
#define HID_REPORT_TYPE_INPUT  1
#define HID_REPORT_TYPE_OUTPUT 2

/* HID protocol modes */
#define HID_PROTOCOL_MODE_BOOT   0
#define HID_PROTOCOL_MODE_REPORT 1

/* HID information flags: RemoteWake capable */
#define HID_INFO_FLAGS_REMOTE_WAKE 0x01

/* Attribute indexes inside hids_svc.attrs[] for notification helpers.
 * Order matches the BT_GATT_SERVICE_DEFINE expansion below:
 *   [0]  Primary Service
 *   [1]  Protocol Mode Declaration
 *   [2]  Protocol Mode Value
 *   [3]  Boot KB In Declaration
 *   [4]  Boot KB In Value
 *   [5]  Boot KB In CCC
 *   [6]  Boot KB Out Declaration
 *   [7]  Boot KB Out Value
 *   [8]  Consumer Control In Declaration
 *   [9]  Consumer Control In Value
 *   [10] Consumer Control In CCC
 *   [11] Consumer Control In Report Ref
 *   [12] Keyboard In Declaration
 *   [13] Keyboard In Value
 *   [14] Keyboard In CCC
 *   [15] Keyboard In Report Ref
 *   [16] Keyboard Out Declaration
 *   [17] Keyboard Out Value
 *   [18] Keyboard Out Report Ref
 *   [19] Report Map Declaration
 *   [20] Report Map Value
 *   [21] Report Map Ext Report Ref
 *   [22] HID Info Declaration
 *   [23] HID Info Value
 *   [24] Ctrl Point Declaration
 *   [25] Ctrl Point Value
 */
#define HIDS_BOOT_KB_IN_VAL_IDX 4
#define HIDS_CC_IN_VAL_IDX      9
#define HIDS_KB_IN_VAL_IDX      13

/* HID state */
static uint8_t hids_protocol_mode = HID_PROTOCOL_MODE_REPORT;

/* Boot keyboard input report: minimal 1 byte like Telink bootKeyInReport. */
static uint8_t hids_boot_kb_in;
static bool hids_boot_kb_in_notify;

/* Boot keyboard output report: 1 byte LED state */
static uint8_t hids_boot_kb_out;

/* Consumer control input report: 2 bytes (16-bit usage code) */
static uint8_t hids_cc_in[2];
static bool hids_cc_in_notify;

/* Keyboard input report: 8 bytes (1 modifier + 1 reserved + 6 keys) */
static uint8_t hids_kb_in[8];
static bool hids_kb_in_notify;

/* Keyboard output report: 1 byte LED state */
static uint8_t hids_kb_out;

/* Report reference descriptors {report_id, report_type} */
static const uint8_t hids_cc_in_ref[2] = {HID_REPORT_ID_CONSUME_CONTROL_INPUT,
					  HID_REPORT_TYPE_INPUT};
static const uint8_t hids_kb_in_ref[2] = {HID_REPORT_ID_KEYBOARD_INPUT, HID_REPORT_TYPE_INPUT};
static const uint8_t hids_kb_out_ref[2] = {HID_REPORT_ID_KEYBOARD_INPUT, HID_REPORT_TYPE_OUTPUT};

/* HID information: bcdHID=0x0111, bCountryCode=0, Flags=RemoteWake */
static const uint8_t hids_info[4] = {0x11, 0x01, 0x00, HID_INFO_FLAGS_REMOTE_WAKE};

/* HID control point (suspend/resume): 0=Resume, 1=Suspend */
static uint8_t hids_ctrl_point;

/* External Report Reference: points to Battery Service UUID (0x180F),
 * required by HID 1.0 spec when Report Map references an external service.
 */
static const uint16_t hids_ext_report_ref = BT_UUID_BAS_VAL;

/* HID Report Map: keyboard (report ID 1) + consumer control (report ID 2).
 * Matches Telink ble_remote/app_att.c reportMap[] (non-audio portion).
 */
static const uint8_t hids_report_map[] = {
	/* Keyboard input report (ID 1) */
	0x05,
	0x01, /* Usage Pg (Generic Desktop) */
	0x09,
	0x06, /* Usage (Keyboard) */
	0xA1,
	0x01, /* Collection: Application */
	0x85,
	HID_REPORT_ID_KEYBOARD_INPUT, /* Report Id (1) */
	0x05,
	0x07, /* Usage Pg (Key Codes) */
	0x19,
	0xE0, /* Usage Min (224) VK_CTRL */
	0x29,
	0xE7, /* Usage Max (231) VK_RWIN */
	0x15,
	0x00, /* Log Min (0) */
	0x25,
	0x01, /* Log Max (1) */
	/* Modifier byte */
	0x75,
	0x01, /* Report Size (1) */
	0x95,
	0x08, /* Report Count (8) */
	0x81,
	0x02, /* Input: Data, Variable, Absolute */
	/* Reserved byte */
	0x95,
	0x01, /* Report Count (1) */
	0x75,
	0x08, /* Report Size (8) */
	0x81,
	0x01, /* Input: Constant */
	/* LED output report: 5 bits */
	0x95,
	0x05, /* Report Count (5) */
	0x75,
	0x01, /* Report Size (1) */
	0x05,
	0x08, /* Usage Pg (LEDs) */
	0x19,
	0x01, /* Usage Min */
	0x29,
	0x05, /* Usage Max */
	0x91,
	0x02, /* Output: Data, Variable, Absolute */
	/* 3 reserved bits */
	0x95,
	0x01, /* Report Count (1) */
	0x75,
	0x03, /* Report Size (3) */
	0x91,
	0x01, /* Output: Constant */
	/* Key arrays (6 bytes) */
	0x95,
	0x06, /* Report Count (6) */
	0x75,
	0x08, /* Report Size (8) */
	0x15,
	0x00, /* Log Min (0) */
	0x25,
	0xF1, /* Log Max (241) */
	0x05,
	0x07, /* Usage Pg (Key Codes) */
	0x19,
	0x00, /* Usage Min (0) */
	0x29,
	0xF1, /* Usage Max (241) */
	0x81,
	0x00, /* Input: Data, Array */
	0xC0, /* End Collection */

	/* Consumer Control input report (ID 2) */
	0x05,
	0x0C, /* Usage Page (Consumer) */
	0x09,
	0x01, /* Usage (Consumer Control) */
	0xA1,
	0x01, /* Collection (Application) */
	0x85,
	HID_REPORT_ID_CONSUME_CONTROL_INPUT, /* Report Id (2) */
	0x75,
	0x10, /* Report Size (16 bits) */
	0x95,
	0x01, /* Report Count (1) */
	0x15,
	0x01, /* Log Min (1) */
	0x26,
	0x8C,
	0x02, /* Log Max (0x028C) */
	0x19,
	0x01, /* Usage Min (1) */
	0x2A,
	0x8C,
	0x02, /* Usage Max (0x028C) */
	0x81,
	0x00, /* Input: Data, Variable, Absolute */
	0xC0, /* End Collection */
};

/* --- Read callbacks (each knows its own buffer length) --- */

static ssize_t read_protocol_mode(struct bt_conn *conn, const struct bt_gatt_attr *attr, void *buf,
				  uint16_t len, uint16_t offset)
{
	return bt_gatt_attr_read(conn, attr, buf, len, offset, &hids_protocol_mode,
				 sizeof(hids_protocol_mode));
}

static ssize_t read_boot_kb_in(struct bt_conn *conn, const struct bt_gatt_attr *attr, void *buf,
			       uint16_t len, uint16_t offset)
{
	return bt_gatt_attr_read(conn, attr, buf, len, offset, &hids_boot_kb_in,
				 sizeof(hids_boot_kb_in));
}

static ssize_t read_boot_kb_out(struct bt_conn *conn, const struct bt_gatt_attr *attr, void *buf,
				uint16_t len, uint16_t offset)
{
	return bt_gatt_attr_read(conn, attr, buf, len, offset, &hids_boot_kb_out,
				 sizeof(hids_boot_kb_out));
}

static ssize_t read_cc_in(struct bt_conn *conn, const struct bt_gatt_attr *attr, void *buf,
			  uint16_t len, uint16_t offset)
{
	return bt_gatt_attr_read(conn, attr, buf, len, offset, hids_cc_in, sizeof(hids_cc_in));
}

static ssize_t read_kb_in(struct bt_conn *conn, const struct bt_gatt_attr *attr, void *buf,
			  uint16_t len, uint16_t offset)
{
	return bt_gatt_attr_read(conn, attr, buf, len, offset, hids_kb_in, sizeof(hids_kb_in));
}

static ssize_t read_kb_out(struct bt_conn *conn, const struct bt_gatt_attr *attr, void *buf,
			   uint16_t len, uint16_t offset)
{
	return bt_gatt_attr_read(conn, attr, buf, len, offset, &hids_kb_out, sizeof(hids_kb_out));
}

static ssize_t read_report_map(struct bt_conn *conn, const struct bt_gatt_attr *attr, void *buf,
			       uint16_t len, uint16_t offset)
{
	return bt_gatt_attr_read(conn, attr, buf, len, offset, hids_report_map,
				 sizeof(hids_report_map));
}

static ssize_t read_hids_info(struct bt_conn *conn, const struct bt_gatt_attr *attr, void *buf,
			      uint16_t len, uint16_t offset)
{
	return bt_gatt_attr_read(conn, attr, buf, len, offset, hids_info, sizeof(hids_info));
}

static ssize_t read_cc_in_ref(struct bt_conn *conn, const struct bt_gatt_attr *attr, void *buf,
			      uint16_t len, uint16_t offset)
{
	return bt_gatt_attr_read(conn, attr, buf, len, offset, hids_cc_in_ref,
				 sizeof(hids_cc_in_ref));
}

static ssize_t read_kb_in_ref(struct bt_conn *conn, const struct bt_gatt_attr *attr, void *buf,
			      uint16_t len, uint16_t offset)
{
	return bt_gatt_attr_read(conn, attr, buf, len, offset, hids_kb_in_ref,
				 sizeof(hids_kb_in_ref));
}

static ssize_t read_kb_out_ref(struct bt_conn *conn, const struct bt_gatt_attr *attr, void *buf,
			       uint16_t len, uint16_t offset)
{
	return bt_gatt_attr_read(conn, attr, buf, len, offset, hids_kb_out_ref,
				 sizeof(hids_kb_out_ref));
}

static ssize_t read_ext_report_ref(struct bt_conn *conn, const struct bt_gatt_attr *attr, void *buf,
				   uint16_t len, uint16_t offset)
{
	return bt_gatt_attr_read(conn, attr, buf, len, offset, &hids_ext_report_ref,
				 sizeof(hids_ext_report_ref));
}

/* --- Write callbacks --- */

static ssize_t write_protocol_mode(struct bt_conn *conn, const struct bt_gatt_attr *attr,
				   const void *buf, uint16_t len, uint16_t offset, uint8_t flags)
{
	const uint8_t *value = buf;

	if (offset != 0 || len != 1) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	}

	if (value[0] != HID_PROTOCOL_MODE_BOOT && value[0] != HID_PROTOCOL_MODE_REPORT) {
		return BT_GATT_ERR(BT_ATT_ERR_VALUE_NOT_ALLOWED);
	}

	hids_protocol_mode = value[0];
	printk("HID protocol mode set to %u\n", hids_protocol_mode);

	return len;
}

static ssize_t write_boot_kb_out(struct bt_conn *conn, const struct bt_gatt_attr *attr,
				 const void *buf, uint16_t len, uint16_t offset, uint8_t flags)
{
	if (offset + len > sizeof(hids_boot_kb_out)) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}

	memcpy(&hids_boot_kb_out + offset, buf, len);
	printk("Boot KB output report: 0x%02x\n", hids_boot_kb_out);

	return len;
}

static ssize_t write_kb_out(struct bt_conn *conn, const struct bt_gatt_attr *attr, const void *buf,
			    uint16_t len, uint16_t offset, uint8_t flags)
{
	if (offset + len > sizeof(hids_kb_out)) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}

	memcpy(&hids_kb_out + offset, buf, len);
	printk("KB output report: 0x%02x\n", hids_kb_out);

	return len;
}

static ssize_t write_ctrl_point(struct bt_conn *conn, const struct bt_gatt_attr *attr,
				const void *buf, uint16_t len, uint16_t offset, uint8_t flags)
{
	const uint8_t *value = buf;

	if (offset != 0 || len != 1) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	}

	if (value[0] != 0 && value[0] != 1) {
		return BT_GATT_ERR(BT_ATT_ERR_VALUE_NOT_ALLOWED);
	}

	hids_ctrl_point = value[0];
	printk("HID control point: %s\n", hids_ctrl_point ? "Suspend" : "Resume");

	return len;
}

/* --- CCC callbacks --- */

static void ccc_boot_kb_in_cfg_changed(const struct bt_gatt_attr *attr, uint16_t value)
{
	hids_boot_kb_in_notify = (value == BT_GATT_CCC_NOTIFY);
	printk("Boot KB input notify: %s\n", hids_boot_kb_in_notify ? "enabled" : "disabled");
}

static void ccc_cc_in_cfg_changed(const struct bt_gatt_attr *attr, uint16_t value)
{
	hids_cc_in_notify = (value == BT_GATT_CCC_NOTIFY);
	printk("Consumer Control input notify: %s\n", hids_cc_in_notify ? "enabled" : "disabled");
}

static void ccc_kb_in_cfg_changed(const struct bt_gatt_attr *attr, uint16_t value)
{
	hids_kb_in_notify = (value == BT_GATT_CCC_NOTIFY);
	printk("KB input notify: %s\n", hids_kb_in_notify ? "enabled" : "disabled");
}

/* --- HID Service definition ---
 * Attribute order matches Telink app_att.c L595-648 (excluding audio reports
 * and the include declaration of the Battery Service, which Zephyr handles
 * automatically when both services are registered).
 */
BT_GATT_SERVICE_DEFINE(
	hids_svc, BT_GATT_PRIMARY_SERVICE(BT_UUID_HIDS),

	/* Protocol Mode: READ | WRITE_WITHOUT_RESP */
	BT_GATT_CHARACTERISTIC(BT_UUID_HIDS_PROTOCOL_MODE,
			       BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE_WITHOUT_RESP,
			       BT_GATT_PERM_READ | BT_GATT_PERM_WRITE, read_protocol_mode,
			       write_protocol_mode, &hids_protocol_mode),

	/* Boot Keyboard Input Report: READ | NOTIFY */
	BT_GATT_CHARACTERISTIC(BT_UUID_HIDS_BOOT_KB_IN_REPORT,
			       BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY, BT_GATT_PERM_READ,
			       read_boot_kb_in, NULL, &hids_boot_kb_in),
	BT_GATT_CCC(ccc_boot_kb_in_cfg_changed, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),

	/* Boot Keyboard Output Report: READ | WRITE | WRITE_WITHOUT_RESP */
	BT_GATT_CHARACTERISTIC(BT_UUID_HIDS_BOOT_KB_OUT_REPORT,
			       BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE |
				       BT_GATT_CHRC_WRITE_WITHOUT_RESP,
			       BT_GATT_PERM_READ | BT_GATT_PERM_WRITE, read_boot_kb_out,
			       write_boot_kb_out, &hids_boot_kb_out),

	/* Consumer Control Input Report: READ | NOTIFY + Report Ref */
	BT_GATT_CHARACTERISTIC(BT_UUID_HIDS_REPORT, BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY,
			       BT_GATT_PERM_READ, read_cc_in, NULL, hids_cc_in),
	BT_GATT_CCC(ccc_cc_in_cfg_changed, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),
	BT_GATT_DESCRIPTOR(BT_UUID_HIDS_REPORT_REF, BT_GATT_PERM_READ, read_cc_in_ref, NULL, NULL),

	/* Keyboard Input Report: READ | NOTIFY + Report Ref */
	BT_GATT_CHARACTERISTIC(BT_UUID_HIDS_REPORT, BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY,
			       BT_GATT_PERM_READ, read_kb_in, NULL, hids_kb_in),
	BT_GATT_CCC(ccc_kb_in_cfg_changed, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),
	BT_GATT_DESCRIPTOR(BT_UUID_HIDS_REPORT_REF, BT_GATT_PERM_READ, read_kb_in_ref, NULL, NULL),

	/* Keyboard Output Report: READ | WRITE | WRITE_WITHOUT_RESP + Report Ref */
	BT_GATT_CHARACTERISTIC(BT_UUID_HIDS_REPORT,
			       BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE |
				       BT_GATT_CHRC_WRITE_WITHOUT_RESP,
			       BT_GATT_PERM_READ | BT_GATT_PERM_WRITE, read_kb_out, write_kb_out,
			       &hids_kb_out),
	BT_GATT_DESCRIPTOR(BT_UUID_HIDS_REPORT_REF, BT_GATT_PERM_READ, read_kb_out_ref, NULL, NULL),

	/* Report Map: READ + External Report Ref */
	BT_GATT_CHARACTERISTIC(BT_UUID_HIDS_REPORT_MAP, BT_GATT_CHRC_READ, BT_GATT_PERM_READ,
			       read_report_map, NULL, NULL),
	BT_GATT_DESCRIPTOR(BT_UUID_HIDS_EXT_REPORT, BT_GATT_PERM_READ, read_ext_report_ref, NULL,
			   NULL),

	/* HID Information: READ */
	BT_GATT_CHARACTERISTIC(BT_UUID_HIDS_INFO, BT_GATT_CHRC_READ, BT_GATT_PERM_READ,
			       read_hids_info, NULL, NULL),

	/* HID Control Point: WRITE_WITHOUT_RESP */
	BT_GATT_CHARACTERISTIC(BT_UUID_HIDS_CTRL_POINT, BT_GATT_CHRC_WRITE_WITHOUT_RESP,
			       BT_GATT_PERM_WRITE, NULL, write_ctrl_point, &hids_ctrl_point), );

/* --- Public notification helpers --- */

int hids_notify_boot_kb_in(uint8_t report)
{
	if (!hids_boot_kb_in_notify) {
		return -EACCES;
	}

	hids_boot_kb_in = report;
	return bt_gatt_notify(NULL, &hids_svc.attrs[HIDS_BOOT_KB_IN_VAL_IDX], &hids_boot_kb_in,
			      sizeof(hids_boot_kb_in));
}

int hids_notify_kb_in(const uint8_t *report, uint8_t len)
{
	if (!hids_kb_in_notify || len > sizeof(hids_kb_in)) {
		return -EACCES;
	}

	memcpy(hids_kb_in, report, len);
	return bt_gatt_notify(NULL, &hids_svc.attrs[HIDS_KB_IN_VAL_IDX], hids_kb_in, len);
}

int hids_notify_cc_in(uint16_t usage)
{
	if (!hids_cc_in_notify) {
		return -EACCES;
	}

	sys_put_le16(usage, hids_cc_in);
	return bt_gatt_notify(NULL, &hids_svc.attrs[HIDS_CC_IN_VAL_IDX], hids_cc_in,
			      sizeof(hids_cc_in));
}

/* USB HID Consumer Control usage codes */
#define CC_VOLUME_INCREMENT 0x00E9
#define CC_VOLUME_DECREMENT 0x00EA

static void on_button_change(size_t button, bool pressed, void *context)
{
	/* const char *context_name = "isr"; */

	/* if (!k_is_in_isr()) { */
	/* context_name = k_thread_name_get(k_current_get()); */
	/* } */
	/* printk("[%s] button %u %s '%s'\n", */
	/* context_name, button, pressed ? "pressed" : "released", (const char *)context); */

	/* HID Consumer Control report semantics:
	 *   Press   -> send usage code (OS holds the key, repeats the action)
	 *   Release -> send 0          (OS releases the key, stops repeating)
	 *
	 * Missing the release (sending 0) makes the OS think the key is still
	 * held down, causing it to keep applying the action (e.g. volume
	 * keeps going up). This is why a single press appeared to "upload
	 * continuously".
	 */
	uint16_t usage = 0;

	switch (button) {
	case 2:
		usage = pressed ? CC_VOLUME_INCREMENT : 0;
		break;
	case 3:
		usage = pressed ? CC_VOLUME_DECREMENT : 0;
		break;
	default:
		return; /* unmapped key */
	}

	hids_notify_cc_in(usage);
}

static KEY_MATRIX_DEFINE(key_matrix);

int le_remote_button_init(void)
{
	if (!key_matrix_init(&key_matrix)) {
		printk("key_matrix_init failed\n");
		return 1;
	}

	static const char test_context[] = "test context";

	key_matrix_set_callback(&key_matrix, on_button_change, (void *)test_context);

	return 0;
}
