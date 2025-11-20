/** @file hog.h
 *  @brief
 */

/*
 * Copyright (c) 2025 Telink Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifdef __cplusplus
extern "C" {
#endif

#include "stack/ble/service/hids.h"

#define HIDS_MOUSE_INPUT_REPORT_ID              0x01
#define HIDS_KEYBOARD_INPUT_REPORT_ID           0x02
#define HIDS_CONSUMER_INPUT_REPORT_ID           0x03
#define HIDS_SYSTEM_INPUT_REPORT_ID             0x04
#define HIDS_ALL_KEY_INPUT_REPORT_ID            0x05
#define HIDS_VENDOR_INPUT_REPORT_ID             0x06


/* HID Boot Report ID */
#define HID_KEYBOARD_BOOT_ID                    0xFF
#define HID_MOUSE_BOOT_ID                       0xFE

/* HID Report Type/ID to Attribute handle map item */
struct hid_report_id_map_t {
	uint8_t type;
	uint8_t id;
	uint16_t handle;
};


struct hids_ccc {
	uint16_t handle; /* ccc handle */
	uint8_t value;    /* ccc value */
} __packed;

/* HID Spec Version: 1.11 */
#define HID_VERSION                   0x0111

/* HID Control Point Values */
#define HID_CONTROL_POINT_SUSPEND     0x00
#define HID_CONTROL_POINT_RESUME      0x01

/* Max length of an output report value */
#define HID_MAX_REPORT_LEN            32

/* Proprietary Service */
#define HID_START_HDL                 0x0
#define HID_END_HDL                   (HID_MAX_HDL - 1)

/* Proprietary Service Handles Common to HID Devices */
enum {
	HID_SVC_HDL = HID_START_HDL,        /* Proprietary Service Declaration */
	HID_INFO_CH_HDL,                    /* HID Information Characteristic Declaration */
	HID_INFO_HDL,                       /* HID Information Value */
	HID_REPORT_MAP_CH_HDL,              /* HID Report Map Characteristic Declaration */
	HID_REPORT_MAP_HDL,                 /* HID Report Map Value */
	HID_EXTERNAL_REPORT_HDL,            /* HID External Report Descriptor */
	HID_CONTROL_POINT_CH_HDL,           /* HID Control Point Characteristic Declaration */
	HID_CONTROL_POINT_HDL,              /* HID Control Point Value */
	HID_KEYBOARD_BOOT_IN_CH_HDL,        /* HID Keyboard Boot Input Characteristic Declaration */
	HID_KEYBOARD_BOOT_IN_HDL,           /* HID Keyboard Boot Input Value */
	HID_KEYBOARD_BOOT_IN_CH_CCC_HDL,    /* HID Keyboard Boot Input CCC Descriptor */
	HID_KEYBOARD_BOOT_OUT_CH_HDL,       /* HID Keyboard Boot Output Characteristic Declaration */
	HID_KEYBOARD_BOOT_OUT_HDL,          /* HID Keyboard Boot Output Value */
	HID_MOUSE_BOOT_IN_CH_HDL,           /* HID Mouse Boot Input Characteristic Declaration */
	HID_MOUSE_BOOT_IN_HDL,              /* HID Mouse Boot Input Value */
	HID_MOUSE_BOOT_IN_CH_CCC_HDL,       /* HID Mouse Boot Input CCC Descriptor */
	HID_INPUT_REPORT_1_CH_HDL,          /* HID Input Report Characteristic Declaration */
	HID_INPUT_REPORT_1_HDL,             /* HID Input Report Value */
	HID_INPUT_REPORT_1_CH_CCC_HDL,      /* HID Input Report CCC Descriptor */
	HID_INPUT_REPORT_1_REFERENCE_HDL,   /* HID Input Report Reference Descriptor */
	HID_INPUT_REPORT_2_CH_HDL,          /* HID Input Report Characteristic Declaration */
	HID_INPUT_REPORT_2_HDL,             /* HID Input Report Value */
	HID_INPUT_REPORT_2_CH_CCC_HDL,      /* HID Input Report CCC Descriptor */
	HID_INPUT_REPORT_2_REFERENCE_HDL,   /* HID Input Report Reference Descriptor */
	HID_INPUT_REPORT_3_CH_HDL,          /* HID Input Report Characteristic Declaration */
	HID_INPUT_REPORT_3_HDL,             /* HID Input Report Value */
	HID_INPUT_REPORT_3_CH_CCC_HDL,      /* HID Input Report CCC Descriptor */
	HID_INPUT_REPORT_3_REFERENCE_HDL,   /* HID Input Report Reference Descriptor */
	HID_INPUT_REPORT_4_CH_HDL,          /* HID Input Report Characteristic Declaration */
	HID_INPUT_REPORT_4_HDL,             /* HID Input Report Value */
	HID_INPUT_REPORT_4_CH_CCC_HDL,      /* HID Input Report CCC Descriptor */
	HID_INPUT_REPORT_4_REFERENCE_HDL,   /* HID Input Report Reference Descriptor */

	HID_INPUT_REPORT_5_CH_HDL,          /* HID Input Report Characteristic Declaration */
	HID_INPUT_REPORT_5_HDL,             /* HID Input Report Value */
	HID_INPUT_REPORT_5_CH_CCC_HDL,      /* HID Input Report CCC Descriptor */
	HID_INPUT_REPORT_5_REFERENCE_HDL,   /* HID Input Report Reference Descriptor */

	HID_INPUT_REPORT_6_CH_HDL,          /* HID Input Report Characteristic Declaration */
	HID_INPUT_REPORT_6_HDL,             /* HID Input Report Value */
	HID_INPUT_REPORT_6_CH_CCC_HDL,      /* HID Input Report CCC Descriptor */
	HID_INPUT_REPORT_6_REFERENCE_HDL,   /* HID Input Report Reference Descriptor */

	HID_OUTPUT_REPORT_CH_HDL,           /* HID Output Report Characteristic Declaration */
	HID_OUTPUT_REPORT_HDL,              /* HID Output Report Value */
	HID_OUTPUT_REPORT_REFERENCE_HDL,    /* HID Output Report Reference Descriptor */
	HID_FEATURE_REPORT_CH_HDL,          /* HID Feature Report Characteristic Declaration */
	HID_FEATURE_REPORT_HDL,             /* HID Feature Report Value */
	HID_FEATURE_REPORT_REFERENCE_HDL,   /* HID Feature Report Reference Descriptor */
	HID_PROTOCOL_MODE_CH_HDL,           /* HID Protocol Mode Characteristic Declaration */
	HID_PROTOCOL_MODE_HDL,              /* HID Protocol Mode Value */
	HID_MAX_HDL
};

void hog_init(void);

void hog_button_loop(void);

void hid_set_ccc_table_value(uint16_t handle, uint8_t value);

uint8_t hid_get_ccc_table_value(uint16_t handle);

int ble_nortify_all_key_data(const void *data, uint16_t len);

int ble_nortify_keyboard_data(const void *data, uint16_t len);

int ble_nortify_consumer_data(const void *data, uint16_t len);

#ifdef __cplusplus
}
#endif
