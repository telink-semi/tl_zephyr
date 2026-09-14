/** @file
 *  @brief HoG Service sample
 */

/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifdef __cplusplus
extern "C" {
#endif

#define HID_USAGE_GEN_DESKTOP_CONSUMER        0x0C
#define HID_USAGE_GEN_DESKTOP_SYSTEM_CONTROL  0x80

/**
 * @brief Simple HID N-keys keyboard report descriptor.
 */
#define HID_N_KEY_REPORT_DESC() {                                \
        HID_USAGE_PAGE(HID_USAGE_GEN_DESKTOP),                \
        HID_USAGE(HID_USAGE_GEN_DESKTOP_KEYBOARD),        \
        HID_COLLECTION(HID_COLLECTION_APPLICATION),        \
                HID_REPORT_ID(0x08),                                                \
                HID_USAGE_PAGE(HID_USAGE_GEN_DESKTOP_KEYPAD),\
                HID_USAGE_MIN8(0x00),                                \
                HID_USAGE_MAX8(0x80),                                \
                HID_LOGICAL_MIN8(0),                                \
                HID_LOGICAL_MAX8(1),                                \
                HID_REPORT_SIZE(1),                                        \
                HID_REPORT_COUNT(0x80),                                \
                HID_INPUT(0x02),                                        \
        HID_END_COLLECTION,                                                \
                                                                                        \
                                /*Consumer */                                \
        HID_USAGE_PAGE(HID_USAGE_GEN_DESKTOP_CONSUMER),                \
        HID_USAGE(HID_USAGE_GEN_DESKTOP),        \
        HID_COLLECTION(HID_COLLECTION_APPLICATION),        \
                HID_REPORT_ID(0x02),                                        \
                HID_REPORT_SIZE(0x10),                                        \
                HID_REPORT_COUNT(0x01),                                \
                HID_LOGICAL_MIN8(1),                                \
                HID_LOGICAL_MAX16(0x8c, 0x02),                                \
                HID_USAGE_MIN8(0x00),                                \
                HID_USAGE_MAX16(0x8c, 0x02),                                \
                HID_INPUT(0x00),                                        \
        HID_END_COLLECTION,                                                \
                                                                        \
                                /* System Key: Sleep/Power/WakeUp */      \
        HID_USAGE_PAGE(HID_USAGE_GEN_DESKTOP),                \
        HID_USAGE(HID_USAGE_GEN_DESKTOP_SYSTEM_CONTROL),      \
        HID_COLLECTION(HID_COLLECTION_APPLICATION),        \
                HID_REPORT_ID(0x03),                                        \
                HID_USAGE(0x82),  /* System Sleep */       \
                HID_USAGE(0x81),  /* System Power Down */  \
                HID_USAGE(0x83),  /* System Wake Up */     \
                HID_LOGICAL_MIN8(0),                                \
                HID_LOGICAL_MAX8(1),                                \
                HID_USAGE_MIN8(0x81),                                \
                HID_USAGE_MAX8(0x83),                                \
                HID_REPORT_SIZE(1),                                        \
                HID_REPORT_COUNT(3),                                \
                HID_INPUT(0x02),                                        \
                HID_REPORT_SIZE(5),                                        \
                HID_REPORT_COUNT(1),                                \
                HID_INPUT(0x01),                                        \
        HID_END_COLLECTION,                                                \
}

enum usb_conn_status {
	USB_DISCONNECTED = 0,
	USB_CONFIGURED,
};

extern volatile unsigned int vbus_status;
extern volatile  unsigned int usb_connected_ok;
extern enum usb_conn_status usb_status;
extern volatile unsigned int usb_suspended;

int usb_hw_init(void);
void app_usb_main_loop(void);
int app_usb_try_wakeup(void);
void app_usb_status_check(void);
#ifdef __cplusplus
}
#endif