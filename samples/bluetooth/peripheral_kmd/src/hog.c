/** @file
 *  @brief HoG Service sample
 */

/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/types.h>
#include <zephyr/drivers/gpio.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/kernel.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/gatt.h>

#include "app_public.h"

enum {
	HIDS_REMOTE_WAKE = BIT(0),
	HIDS_NORMALLY_CONNECTABLE = BIT(1),
};

static uint8_t ctrl_point;

/*! HID Report Type/ID and attribute handle map */
// static const struct hid_report_id_map_t hid_app_report_id_set[] = {
// 	/* type                       ID                            handle */
// 	{HOG_REPORT_TYPE_INPUT,       HIDS_MOUSE_INPUT_REPORT_ID,    HID_INPUT_REPORT_1_HDL},  /* Mouse Input Report */
// 	{HOG_REPORT_TYPE_INPUT,       HIDS_KEYBOARD_INPUT_REPORT_ID, HID_INPUT_REPORT_2_HDL},     /* Keyboard Input Report */
// 	{HOG_REPORT_TYPE_OUTPUT,      HIDS_KEYBOARD_INPUT_REPORT_ID, HID_OUTPUT_REPORT_HDL},      /* Keyboard Output Report */
// 	{HOG_REPORT_TYPE_FEATURE,     HIDS_KEYBOARD_INPUT_REPORT_ID, HID_FEATURE_REPORT_HDL},     /* Keyboard Feature Report */
// 	{HOG_REPORT_TYPE_INPUT,       HIDS_CONSUMER_INPUT_REPORT_ID, HID_INPUT_REPORT_3_HDL},     /* consumer Input Report */
// 	{HOG_REPORT_TYPE_INPUT,       HIDS_SYSTEM_INPUT_REPORT_ID ,  HID_INPUT_REPORT_4_HDL},     /* system Input Report */
// 	{HOG_REPORT_TYPE_INPUT,       HIDS_ALL_KEY_INPUT_REPORT_ID , HID_INPUT_REPORT_5_HDL},    /* kb all key Input Report */
// 	{HOG_REPORT_TYPE_INPUT,       HIDS_VENDOR_INPUT_REPORT_ID ,  HID_INPUT_REPORT_6_HDL},    /* kb all key Input Report */
// 	{HOG_REPORT_TYPE_INPUT,       HID_KEYBOARD_BOOT_ID,          HID_KEYBOARD_BOOT_IN_HDL},   /* Boot Keyboard Input Report */
// 	{HOG_REPORT_TYPE_OUTPUT,      HID_KEYBOARD_BOOT_ID,          HID_KEYBOARD_BOOT_OUT_HDL},  /* Boot Keyboard Output Report */
// 	{HOG_REPORT_TYPE_INPUT,       HID_MOUSE_BOOT_ID,             HID_MOUSE_BOOT_IN_HDL},      /* Boot Mouse Input Report */
// };

static uint8_t hid_report_map[] = {
 //keyboard report in
	 0x05, 0x01,	 // Usage Pg (Generic Desktop)
	 0x09, 0x06,	 // Usage (Keyboard)
	 0xA1, 0x01,	 // Collection: (Application)
     0x85, HIDS_KEYBOARD_INPUT_REPORT_ID,	 // Report Id (keyboard)
				   //
	 0x05, 0x07,	 // Usage Pg (Key Codes)
	 0x19, 0xE0,	 // Usage Min (224)  VK_CTRL:0xe0
	 0x29, 0xE7,	 // Usage Max (231)  VK_RWIN:0xe7
	 0x15, 0x00,	 // Log Min (0)
	 0x25, 0x01,	 // Log Max (1)
				   //
				   // Modifier byte
	 0x75, 0x01,	 // Report Size (1)   1 bit * 8
	 0x95, 0x08,	 // Report Count (8)
	 0x81, 0x02,	 // Input: (Data, Variable, Absolute)
				   //
				   // Reserved byte
	 0x95, 0x01,	 // Report Count (1)
	 0x75, 0x08,	 // Report Size (8)
	 0x81, 0x01,	 // Input: (Constant)
 
	 //keyboard output
	 //5 bit led ctrl: NumLock CapsLock ScrollLock Compose kana
	 0x95, 0x05,	//Report Count (5)
	 0x75, 0x01,	//Report Size (1)
	 0x05, 0x08,	//Usage Pg (LEDs )
	 0x19, 0x01,	//Usage Min
	 0x29, 0x05,	//Usage Max
	 0x91, 0x02,	//Output (Data, Variable, Absolute)
	 //3 bit reserved
	 0x95, 0x01,	//Report Count (1)
	 0x75, 0x03,	//Report Size (3)
	 0x91, 0x01,	//Output (Constant)
 
	 // Key arrays (6 bytes)
	 0x95, 0x06,	 // Report Count (6)
	 0x75, 0x08,	 // Report Size (8)
	 0x15, 0x00,	 // Log Min (0)
	 0x26, 0xF1, 0x00,	 // Log Max (241)
	 0x05, 0x07,	 // Usage Pg (Key Codes)
	 0x19, 0x00,	 // Usage Min (0)
	 0x2a, 0xf1, 0x00, 	 // Usage Max (241)
	 0x81, 0x00,	 // Input: (Data, Array)
 
	 0xC0,			  // End Collection

/////////////////////////Consumer///////////////////////////////////
	0x05, 0x0C,   // Usage Page (Consumer)
	0x09, 0x01,   // Usage (Consumer Control)
	0xA1, 0x01,   // Collection (Application)
	0x85, HIDS_CONSUMER_INPUT_REPORT_ID,   //	 Report Id
	0x75,0x10, 	//global, report size 16 bits
	0x95,0x01, 	//global, report count 1
	0x15,0x01, 	//global, min  0x01
	0x26,0x8c,0x02,  //global, max  0x28c
	0x19,0x01, 	//local, min   0x01
	0x2a,0x8c,0x02,  //local, max	  0x28c
	0x81,0x00, 	//main,  input data varible, absolute
	0xc0,		  //main, end collection
	 
	/////////////////////////System report///////////////////////////////////

	0x05,0x01, 	//Usage Page (Generic Desktop Control)
	0x09,0x80,	//Usage (SYSTEM CONTROL)
	0xA1,0x01,	//Collection (Application)

	0x85,HIDS_SYSTEM_INPUT_REPORT_ID,		//Report ID (ACPI)

	0x25,0x01,	//	Logical Maximum (1)
	0x15,0x00,	//	Logical Minimum (0)
	0x75,0x01,	//	Report Size

	0x09,0x82, 	//	USAGE SYSTEM SLEEP
	0x09,0x81,	//	USAGE SYSTEM POWER DOWN 
	0x09,0x83, 	//	USAGE SYSTEM WAKE UP

	0x95,0x03,	//	REPORT_COUNT (03H)
	0x81,0x02,	//	INPUT (DATA, VAR)								 
	0x95,0x05,	//	Report Count (05)

	0x81,0x01,	//	Input (CONSTANT)
	0xC0,		//	END COLLECTION
/////////////////////////ALL key report///////////////////////////////////

	0x05, 0x01,  // Usage Pg (Generic Desktop)
	0x09, 0x06,  // Usage (Keyboard)
	0xA1, 0x01,  // Collection: (Application)
	0x85, HIDS_ALL_KEY_INPUT_REPORT_ID,  // Report Id (keyboard)
	0x05, 0x07,  // Usage Pg (Key Codes)
	0x19, 0x00,  // Usage Min 
	0x29, 0x80,  // Usage Max 
	0x15, 0x00,  // Log Min (0)
	0x25, 0x01,  // Log Max (1)

	0x75, 0x01,  // Report Size (1)   1 bit * 8
	0x95, 0x80,  // Report Count (8)
	0x81, 0x02,  // Input: (Data, Variable, Absolute
	
	0xC0,             // End Collection

	/////////////////////////Mouse///////////////////////////////////
	0x05, 0x01,  // Usage Page (Generic Desktop)
	0x09, 0x02,  // Usage (Mouse)
	0xA1, 0x01,  // Collection (Application)
	0x85, HIDS_MOUSE_INPUT_REPORT_ID,  // Report Id
	0x09, 0x01,  //   Usage (Pointer)
	0xA1, 0x00,  //   Collection (Physical)
	0x05, 0x09,  //	 Usage Page (Buttons)
	0x19, 0x01,  //	 Usage Minimum (01) - Button 1
	0x29, 0x03,  //	 Usage Maximum (03) - Button 3
	0x15, 0x00,  //	 Logical Minimum (0)
	0x25, 0x01,  //	 Logical Maximum (1)
	0x75, 0x01,  //	 Report Size (1)
	0x95, 0x05,  //	 Report Count (3)
	0x81, 0x02,  //	 Input (Data, Variable, Absolute) - Button states
	0x75, 0x03,  //	 Report Size (5)
	0x95, 0x01,  //	 Report Count (1)
	0x81, 0x01,  //	 Input (Constant) - Padding or Reserved bits
 
	0x05,0x01,   //  Usage Page (Generic Desktop Control)	 
	0x09,0x30,   // Usage (X)
	0x09,0x31,   // Usage (Y)
	 
	0x16,0x01,0x80, //  LOGICAL_MINIMUM(0)
	0x26,0xff,0x7f,  
	0x75,0x10,  //  Report Size (16)
	0x95,0x02,  //  Report Count (2)
	0x81,0x06,  //  Input (Data, Variable, Relative)

	//0x05,0x01,			 //  Usage Page (Generic Desktop Control)
	0x09,0x38, 		 //  Usage (Wheel)
	0x15,0x81, 		 //  Logical Minimum (-4)
	0x25,0x7F, 		 //  Logical Maximum (3)
	0x75,0x08, 		 //  Report Size (3)
	0x95,0x01, 		 //  Report Count (1)
	0x81,0x06, 		 //  Input (Data, Variable, Relative)
	 
	0xC0,		  //   End Collection
	0xC0,		  // End Collection
};


struct hids_info {
	uint16_t version; /* version number of base USB HID Specification */
	uint8_t code; /* country HID Device hardware is localized for. */
	uint8_t flags;
} __packed;

struct hids_report {
	uint8_t id; /* report id */
	uint8_t type; /* report type */
} __packed;

/* HID Info Value: HID Spec version, country code, flags */
const struct hids_info hid_info_val = {
	.version = HID_VERSION,
	.code = 0x00,
	.flags = HIDS_NORMALLY_CONNECTABLE,
};

/* HID Control Point Value */
uint8_t hid_cp_val;

struct _bt_gatt_ccc hid_ccc[5];

/* HID Input Report Reference - ID, Type */
const struct hids_report hid_val_irep1_id_map = {
	.id = HIDS_MOUSE_INPUT_REPORT_ID,
	.type = HOG_REPORT_TYPE_INPUT,
};

/* HID Input Report Reference - ID, Type */
const struct hids_report hid_val_irep2_id_map = {
	.id = HIDS_KEYBOARD_INPUT_REPORT_ID,
	.type = HOG_REPORT_TYPE_INPUT,
};

/* HID Input Report Reference - ID, Type */
const struct hids_report hid_val_irep3_id_map = {
	.id = HIDS_CONSUMER_INPUT_REPORT_ID,
	.type = HOG_REPORT_TYPE_INPUT,
};

/* HID Input Report Reference - ID, Type */
const struct hids_report hid_val_irep4_id_map = {
	.id = HIDS_SYSTEM_INPUT_REPORT_ID,
	.type = HOG_REPORT_TYPE_INPUT,
};

/* HID Input Report Reference - ID, Type */
const struct hids_report hid_val_irep5_id_map = {
	.id = HIDS_ALL_KEY_INPUT_REPORT_ID,
	.type = HOG_REPORT_TYPE_INPUT,
};

/* HID Input Report Reference - ID, Type */
const struct hids_report hid_val_irep6_id_map = {
	.id = HIDS_VENDOR_INPUT_REPORT_ID,
	.type = HOG_REPORT_TYPE_INPUT,
};

/* HID Output Report Reference - ID, Type */
const struct hids_report hid_val_orep_id_map = {
	.id = 0x0a,
	.type = HOG_REPORT_TYPE_OUTPUT,
};

/* HID Feature Report Reference - ID, Type */
const struct hids_report hid_val_frep_id_map = {
	.id = 0x00,
	.type = HOG_REPORT_TYPE_FEATURE,
};

/* HID Protocol Mode Value */
static uint8_t hid_pm_val = HOG_PROTOCOL_MODE_REPORT;
static const uint16_t hid_len_pm_val = sizeof(hid_pm_val);

static uint8_t bt_attr_get_id(const struct bt_gatt_attr *attr);
//static struct hid_report_id_map_t *hid_get_report_id_map(uint16_t handle);

static ssize_t read_info(struct bt_conn *conn,
			  const struct bt_gatt_attr *attr, void *buf,
			  uint16_t len, uint16_t offset)
{
	return bt_gatt_attr_read(conn, attr, buf, len, offset, attr->user_data,
				 sizeof(struct hids_info));
}

static ssize_t read_report_map(struct bt_conn *conn,
			       const struct bt_gatt_attr *attr, void *buf,
			       uint16_t len, uint16_t offset)
{
	return bt_gatt_attr_read(conn, attr, buf, len, offset, hid_report_map,
				 sizeof(hid_report_map));
}

static ssize_t read_ext_report(struct bt_conn *conn,
			       const struct bt_gatt_attr *attr, void *buf,
			       uint16_t len, uint16_t offset)
{
	uint8_t hid_ext_report[] = {((uint8_t) (BT_UUID_BAS_BATTERY_LEVEL_VAL)), ((uint8_t)((BT_UUID_BAS_BATTERY_LEVEL_VAL) >> 8))};
	uint16_t hid_len_ext_report = sizeof(hid_ext_report);

	return bt_gatt_attr_read(conn, attr, buf, len, offset, hid_ext_report,
				 hid_len_ext_report);
}

static ssize_t write_ctrl_point(struct bt_conn *conn,
				const struct bt_gatt_attr *attr,
				const void *buf, uint16_t len, uint16_t offset,
				uint8_t flags)
{
	uint8_t *value = attr->user_data;

	if (offset + len > sizeof(ctrl_point)) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}

	memcpy(value + offset, buf, len);

	return len;
}

static ssize_t read_report_reference(struct bt_conn *conn,
			   const struct bt_gatt_attr *attr, void *buf,
			   uint16_t len, uint16_t offset)
{
	return bt_gatt_attr_read(conn, attr, buf, len, offset, attr->user_data,
				 sizeof(struct hids_report));
}

static ssize_t read_report_value(struct bt_conn *conn,
			   const struct bt_gatt_attr *attr, void *buf,
			   uint16_t len, uint16_t offset)
{
	//struct hid_report_id_map_t *p_id_map;

	if (offset > HID_MAX_REPORT_LEN)
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	#if 0
	/* notify the application */
	if (hid_cb.p_config->input_cback != NULL) {
		p_id_map = hid_get_report_id_map(bt_attr_get_id(attr));

		if (p_id_map != NULL)
			hid_cb.p_config->input_cback(conn, p_id_map->id, len, (uint8_t *)buf);
	}
	#endif
	return len;
}

static ssize_t write_report_value(struct bt_conn *conn,
				const struct bt_gatt_attr *attr,
				const void *buf, uint16_t len, uint16_t offset,
				uint8_t flags)
{
	// struct hid_report_id_map_t *p_id_map;

	printk("write_report_value: len %d\r\n", len);

	if (offset + len > HID_MAX_REPORT_LEN)
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);

	#if 0
	/* notify the application */
	if (hid_cb.p_config->output_cback != NULL) {
		p_id_map = hid_get_report_id_map(bt_attr_get_id(attr));

		if (p_id_map != NULL)
			hid_cb.p_config->output_cback(conn, p_id_map->id, len, (uint8_t *)buf);
	}
	#endif
	return len;
}

static void hid_ccc_changed(const struct bt_gatt_attr *attr, uint16_t value)
{
	hid_set_ccc_table_value(bt_attr_get_id(attr), value);
    connect_complete = 1;
	printk("handle: %d, ccc_value : %d", bt_attr_get_id(attr), value);
}

static ssize_t read_protocol_mode(struct bt_conn *conn,
			   const struct bt_gatt_attr *attr, void *buf,
			   uint16_t len, uint16_t offset)
{
	return bt_gatt_attr_read(conn, attr, buf, len, offset, attr->user_data,
				 hid_len_pm_val);
}


static ssize_t write_protocol_mode(struct bt_conn *conn,
				const struct bt_gatt_attr *attr,
				const void *buf, uint16_t len, uint16_t offset,
				uint8_t flags)
{
	if (offset + len > 1)
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	#if 0
	if (hid_cb.p_config->info_cback != NULL)
		hid_cb.p_config->info_cback(conn, HID_INFO_PROTOCOL_MODE, *((uint8_t *)buf));
	#endif
	return len;
}

/* HID Service Declaration */
BT_GATT_SERVICE_DEFINE(hog_svc,

	BT_GATT_PRIMARY_SERVICE(BT_UUID_HIDS),

	BT_GATT_CHARACTERISTIC(BT_UUID_HIDS_INFO, BT_GATT_CHRC_READ,
						   BT_GATT_PERM_READ_ENCRYPT,
			   			  read_info, NULL, (void *)&hid_info_val),

	BT_GATT_CHARACTERISTIC(BT_UUID_HIDS_REPORT_MAP, BT_GATT_CHRC_READ,
						 BT_GATT_PERM_READ_ENCRYPT,
			             read_report_map, NULL, NULL),
	BT_GATT_DESCRIPTOR(BT_UUID_HIDS_EXT_REPORT, BT_GATT_PERM_READ_ENCRYPT,
			   read_ext_report, NULL, NULL),

	BT_GATT_CHARACTERISTIC(BT_UUID_HIDS_CTRL_POINT,
			       BT_GATT_CHRC_WRITE_WITHOUT_RESP,
				   BT_GATT_PERM_WRITE_ENCRYPT,
			       NULL, write_ctrl_point, &hid_cp_val),

	BT_GATT_CHARACTERISTIC(BT_UUID_HIDS_BOOT_KB_IN_REPORT,
			       BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY,
				   BT_GATT_PERM_READ_ENCRYPT,
			       read_report_value, NULL, NULL),
	BT_GATT_CCC(hid_ccc_changed,
		    BT_GATT_PERM_READ_ENCRYPT | BT_GATT_PERM_WRITE_ENCRYPT),

	BT_GATT_CHARACTERISTIC(BT_UUID_HIDS_BOOT_KB_OUT_REPORT,
			       BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE_WITHOUT_RESP | BT_GATT_CHRC_WRITE,
				   BT_GATT_PERM_READ_ENCRYPT | BT_GATT_PERM_WRITE_ENCRYPT,
			   read_report_value, write_report_value, NULL),

	BT_GATT_CHARACTERISTIC(BT_UUID_HIDS_BOOT_MOUSE_IN_REPORT,
			       BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY,
				   BT_GATT_PERM_READ_ENCRYPT,
			       read_report_value, NULL, (void *)NULL),
	BT_GATT_CCC(hid_ccc_changed,
		    BT_GATT_PERM_READ_ENCRYPT | BT_GATT_PERM_WRITE_ENCRYPT),
	/*hid input report 1*/
	BT_GATT_CHARACTERISTIC(BT_UUID_HIDS_REPORT,
			       BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY,
				   BT_GATT_PERM_READ_ENCRYPT,
			       read_report_value, NULL, NULL),
	BT_GATT_CCC(hid_ccc_changed,
		    BT_GATT_PERM_READ_ENCRYPT | BT_GATT_PERM_WRITE_ENCRYPT),
	BT_GATT_DESCRIPTOR(BT_UUID_HIDS_REPORT_REF, BT_GATT_PERM_READ_ENCRYPT,
			   read_report_reference, NULL, (void *)&hid_val_irep1_id_map),
	/*hid input report 2*/
	BT_GATT_CHARACTERISTIC(BT_UUID_HIDS_REPORT,
			       BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY,
				   BT_GATT_PERM_READ_ENCRYPT,
			       read_report_value, NULL, NULL),
	BT_GATT_CCC(hid_ccc_changed,
		    BT_GATT_PERM_READ_ENCRYPT | BT_GATT_PERM_WRITE_ENCRYPT),
	BT_GATT_DESCRIPTOR(BT_UUID_HIDS_REPORT_REF, BT_GATT_PERM_READ_ENCRYPT,
			   read_report_reference, NULL, (void *)&hid_val_irep2_id_map),
	/*hid input report 3*/
	BT_GATT_CHARACTERISTIC(BT_UUID_HIDS_REPORT,
			       BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY,
				   BT_GATT_PERM_READ_ENCRYPT,
			       read_report_value, NULL, NULL),
	BT_GATT_CCC(hid_ccc_changed,
		    BT_GATT_PERM_READ_ENCRYPT | BT_GATT_PERM_WRITE_ENCRYPT),
	BT_GATT_DESCRIPTOR(BT_UUID_HIDS_REPORT_REF, BT_GATT_PERM_READ_ENCRYPT,
			   read_report_reference, NULL, (void *)&hid_val_irep3_id_map),

 	/*hid input report 4*/
	BT_GATT_CHARACTERISTIC(BT_UUID_HIDS_REPORT,
			       BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY,
				   BT_GATT_PERM_READ_ENCRYPT,
			       read_report_value, NULL, NULL),
	BT_GATT_CCC(hid_ccc_changed,
		    BT_GATT_PERM_READ_ENCRYPT | BT_GATT_PERM_WRITE_ENCRYPT),
	BT_GATT_DESCRIPTOR(BT_UUID_HIDS_REPORT_REF, BT_GATT_PERM_READ_ENCRYPT,
			   read_report_reference, NULL, (void *)&hid_val_irep4_id_map),

 	/*hid input report 5*/
	BT_GATT_CHARACTERISTIC(BT_UUID_HIDS_REPORT,
			       BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY,
				   BT_GATT_PERM_READ_ENCRYPT,
			       read_report_value, NULL, NULL),
	BT_GATT_CCC(hid_ccc_changed,
		    BT_GATT_PERM_READ_ENCRYPT | BT_GATT_PERM_WRITE_ENCRYPT),
	BT_GATT_DESCRIPTOR(BT_UUID_HIDS_REPORT_REF, BT_GATT_PERM_READ_ENCRYPT,
			   read_report_reference, NULL, (void *)&hid_val_irep5_id_map),

 	/*hid input report 6*/
	BT_GATT_CHARACTERISTIC(BT_UUID_HIDS_REPORT,
			       BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY,
				   BT_GATT_PERM_READ_ENCRYPT,
			       read_report_value, NULL, NULL),
	BT_GATT_CCC(hid_ccc_changed,
		    BT_GATT_PERM_READ_ENCRYPT | BT_GATT_PERM_WRITE_ENCRYPT),
	BT_GATT_DESCRIPTOR(BT_UUID_HIDS_REPORT_REF, BT_GATT_PERM_READ_ENCRYPT,
			   read_report_reference, NULL, (void *)&hid_val_irep6_id_map),

    /*hid output report*/
	BT_GATT_CHARACTERISTIC(BT_UUID_HIDS_REPORT,
			       BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE_WITHOUT_RESP | BT_GATT_CHRC_WRITE,
				   BT_GATT_PERM_READ_ENCRYPT | BT_GATT_PERM_WRITE_ENCRYPT,
			       read_report_value, write_report_value, NULL),
	BT_GATT_DESCRIPTOR(BT_UUID_HIDS_REPORT_REF, BT_GATT_PERM_READ_ENCRYPT,
			   read_report_reference, NULL, (void *)&hid_val_orep_id_map),
    /*hid feature report*/
	BT_GATT_CHARACTERISTIC(BT_UUID_HIDS_REPORT,
			       BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE,
				   BT_GATT_PERM_READ_ENCRYPT | BT_GATT_PERM_WRITE_ENCRYPT,
			       read_report_value, write_report_value, NULL),
	BT_GATT_DESCRIPTOR(BT_UUID_HIDS_REPORT_REF, BT_GATT_PERM_READ_ENCRYPT,
			   read_report_reference, NULL, (void *)&hid_val_frep_id_map),
    /*hid protocol mode*/
	BT_GATT_CHARACTERISTIC(BT_UUID_HIDS_PROTOCOL_MODE,
			       BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE_WITHOUT_RESP,
				   BT_GATT_PERM_READ_ENCRYPT | BT_GATT_PERM_WRITE_ENCRYPT,
				  read_protocol_mode, write_protocol_mode, &hid_pm_val),
);

void hog_init(void)
{

}

static uint8_t bt_attr_get_id(const struct bt_gatt_attr *attr)
{
	return attr -  attr_hog_svc;
}

struct hids_ccc hids_ccc_table[] = {
	/* handle                           value*/
{HID_KEYBOARD_BOOT_IN_CH_CCC_HDL, 0},
{HID_MOUSE_BOOT_IN_CH_CCC_HDL, 0},
{HID_INPUT_REPORT_1_CH_CCC_HDL, 0},
{HID_INPUT_REPORT_2_CH_CCC_HDL, 0},
{HID_INPUT_REPORT_3_CH_CCC_HDL, 0},
{HID_INPUT_REPORT_4_CH_CCC_HDL, 0},
{HID_INPUT_REPORT_5_CH_CCC_HDL, 0},
{HID_INPUT_REPORT_6_CH_CCC_HDL, 0},
};

void hid_set_ccc_table_value(uint16_t handle, uint8_t value)
{
	uint8_t i = 0;

	for (i = 0; i < ARRAY_SIZE(hids_ccc_table); i++) {
		if (hids_ccc_table[i].handle == handle) {
			hids_ccc_table[i].value = value;
			break;
		}
	}
}

uint8_t hid_get_ccc_table_value(uint16_t handle)
{
	uint8_t i = 0;

	for (i = 0; i < ARRAY_SIZE(hids_ccc_table); i++) {
		if (hids_ccc_table[i].handle == handle)
			return hids_ccc_table[i].value;
	}
	return 0;
}

int hid_send_input_report(struct bt_conn *conn, uint8_t report_id, uint8_t *p_value, uint16_t len)
{
	#if 0
	uint16_t handle = hid_get_report_handle(HOG_REPORT_TYPE_INPUT, report_id);

	if ((handle != 0) && hid_ccc_is_enabled(handle+1))
		return bt_gatt_notify(conn, &attrs[handle], p_value, len);
	else
		return -EIO;
	#endif

	return bt_gatt_notify(conn,  &hog_svc.attrs[HID_INPUT_REPORT_3_HDL], p_value, len);
}

int ble_nortify_keyboard_data(const void *data, uint16_t len)
{
	if (hid_get_ccc_table_value(HID_INPUT_REPORT_2_CH_CCC_HDL)) {
		return  bt_gatt_notify(NULL,  &hog_svc.attrs[HID_INPUT_REPORT_2_HDL], data, len);
	} else {
		//printk("k");
		return -1;
	}
}

int ble_nortify_consumer_data(const void *data, uint16_t len)
{
	if (hid_get_ccc_table_value(HID_INPUT_REPORT_3_CH_CCC_HDL)) {
		return  bt_gatt_notify(NULL,  &hog_svc.attrs[HID_INPUT_REPORT_3_HDL], data, len);
	} else {
		//printk("c");
		return -1;
	}
}

int ble_nortify_all_key_data(const void *data, uint16_t len)
{
	if (hid_get_ccc_table_value(HID_INPUT_REPORT_5_CH_CCC_HDL)) {
		return  bt_gatt_notify(NULL,  &hog_svc.attrs[HID_INPUT_REPORT_5_HDL], data, len);
	} else {
		//printk("a");
		return -1;
	}
}
