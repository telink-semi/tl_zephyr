/* main.c - Application main entry point */

/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/types.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/kernel.h>

#include <zephyr/settings/settings.h>

#include <zephyr/usb/usb_device.h>
#include <zephyr/usb/class/usb_hid.h>


#include "app_public.h"

#define LOG_LEVEL LOG_LEVEL_DBG
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(app_usb);

#define APP_VBUS_CHECK_DE_JT_CNT   2

//static struct gpio_dt_spec led_caps = GPIO_DT_SPEC_GET_OR(DT_ALIAS(led-caps), gpios, {0});
//static struct gpio_dt_spec led_num = GPIO_DT_SPEC_GET_OR(DT_ALIAS(led-num), gpios, {0});
//static struct gpio_dt_spec led_scrol = GPIO_DT_SPEC_GET_OR(DT_ALIAS(led-scrol), gpios, {0});
//static struct gpio_dt_spec led_pair = GPIO_DT_SPEC_GET_OR(DT_ALIAS(led-pair), gpios, {0});

volatile unsigned int  vbus_status = 0;
static unsigned char last_vbus_status = 0;
volatile unsigned int usb_connected_ok = 0;

#if 1
 static const uint8_t hid_report_kb_desc[] = HID_KEYBOARD_REPORT_DESC();
#else
__aligned(4) static const unsigned char  hid_report_kb_desc[] = {
 //keyboard report in
     0x05, 0x01,     // Usage Pg (Generic Desktop)
     0x09, 0x06,     // Usage (Keyboard)
     0xA1, 0x01,     // Collection: (Application)
     //0x85, REPORT_ID_KEYBOARD_INPUT_AAA,   // Report Id (keyboard)
                   //
     0x05, 0x07,     // Usage Pg (Key Codes)
     0x19, 0xE0,     // Usage Min (224)  VK_CTRL:0xe0
     0x29, 0xE7,     // Usage Max (231)  VK_RWIN:0xe7
     0x15, 0x00,     // Log Min (0)
     0x25, 0x01,     // Log Max (1)
                   //
                   // Modifier byte
     0x75, 0x01,     // Report Size (1)   1 bit * 8
     0x95, 0x08,     // Report Count (8)
     0x81, 0x02,     // Input: (Data, Variable, Absolute)
                   //
                   // Reserved byte
     0x95, 0x01,     // Report Count (1)
     0x75, 0x08,     // Report Size (8)
     0x81, 0x01,     // Input: (Constant)

     //keyboard output
     //5 bit led ctrl: NumLock CapsLock ScrollLock Compose kana
     0x95, 0x05,    //Report Count (5)
     0x75, 0x01,    //Report Size (1)
     0x05, 0x08,    //Usage Pg (LEDs )
     0x19, 0x01,    //Usage Min
     0x29, 0x05,    //Usage Max
     0x91, 0x02,    //Output (Data, Variable, Absolute)
     //3 bit reserved
     0x95, 0x01,    //Report Count (1)
     0x75, 0x03,    //Report Size (3)
     0x91, 0x01,    //Output (Constant)

     // Key arrays (6 bytes)
     0x95, 0x06,     // Report Count (6)
     0x75, 0x08,     // Report Size (8)
     0x15, 0x00,     // Log Min (0)
     0x26, 0xF1,0x00,    // Log Max (241)
     0x05, 0x07,     // Usage Pg (Key Codes)
     0x19, 0x00,     // Usage Min (0)
     0x2a, 0xf1,0x00,    // Usage Max (241)
     0x81, 0x00,     // Input: (Data, Array)

     0xC0,            // End Collection
};
#endif

//static const uint8_t hid_report_ms_desc[] = HID_MOUSE_REPORT_DESC(5);
static const uint8_t hid_report_n_keys_desc[] = HID_N_KEY_REPORT_DESC();
#if 0
static const uint8_t hid_report_ms_desc[] = {
	//mouse report in
	0x05, 0x01,  // Usage Page (Generic Desktop)
	0x09, 0x02,  // Usage (Mouse)
	0xA1, 0x01,  // Collection (Application)
	0x85, 0x01,  // Report Id
	0x09, 0x01,  //   Usage (Pointer)
	0xA1, 0x00,  //   Collection (Physical)
	0x05, 0x09,  //  Usage Page (Buttons)
	0x19, 0x01,  //  Usage Minimum (01) - Button 1
	0x29, 0x05,  //  Usage Maximum (03) - Button 3
	0x15, 0x00,  //  Logical Minimum (0)
	0x25, 0x01,  //  Logical Maximum (1)
	0x75, 0x01,  //  Report Size (1)
	0x95, 0x05,  //  Report Count (3)
	0x81, 0x02,  //  Input (Data, Variable, Absolute) - Button states
	0x75, 0x03,  //  Report Size (5)
	0x95, 0x01,  //  Report Count (1)
	0x81, 0x01,  //  Input (Constant) - Padding or Reserved bits

	0x05, 0x01,	 //  Usage Page (Generic Desktop Control)
	0x09, 0x30,  // Usage (X)
	0x09, 0x31,  // Usage (Y)

	0x16, 0x01, 0x80, //  LOGICAL_MINIMUM(0)
	0x26, 0xff, 0x7f,
	0x75, 0x10, //	Report Size (16)
	0x95, 0x02, //	Report Count (2)
	0x81, 0x06, //	Input (Data, Variable, Relative)

	//0x05,0x01,			 //  Usage Page (Generic Desktop Control)
	0x09, 0x38,		 //  Usage (Wheel)
	0x15, 0x81,		 //  Logical Minimum (-4)
	0x25, 0x7F,		 //  Logical Maximum (3)
	0x75, 0x08,		 //  Report Size (3)
	0x95, 0x01,		 //  Report Count (1)
	0x81, 0x06,		 //  Input (Data, Variable, Relative)

	0x05,0x0c,			// Usage Page(Consumer)
	0x0a,0x38,0x02,	// Usage (AC Pan) tilt
	0x15,0x81,			//	Logical Minimum (-128)
	0x25,0x7F,			//	Logical Maximum (127)
	0x75,0x08,			//	Report Size (8)
	0x95,0x01,			//	Report Count (3)
	0x81,0x06,			// Input (Data, Variable, Relative)

	0xC0,		  //   End Collection
	0xC0,		  // End Collection
};
#endif
static const uint8_t hid_report_vendor_defined[] = HID_MOUSE_REPORT_DESC(5);
#if 0
static const uint8_t hid_report_vendor_defined[] = {
    0x06, 0xEF, 0xff,  //global usage page
    0x09, 0x00,  //usage undefined
    0xa1, 0x01,  //main collection
    0x85, 0x06, //global report ID 0x6
    0x15, 0x00,  //LOGICAL_MINIMUM (0)
    0x26, 0xff, 0x00, //LOGICAL_MAXIMUM (255)
    0x95, 63,   //Report Count (32) //MTU_SIZE=23
    0x75, 0x08,  //Report Size (8)
    0x09, 0x01,
    0x81, 0x02,  //INPUT (Data,Var,Abs)
    0x09, 0x02,
    0x91, 0x02,  //Output (Data, Variable, Absolute)
    0x09, 0x03,
    0xB1, 0x02,  //feature (Data, Variable, Absolute)
    0xc0,    //main, end collection

};
#endif

static enum usb_dc_status_code usb_status;

static K_SEM_DEFINE(usb_sem, 1, 1); /* starts off "available" */
static void in_ready_cb(const struct device *dev)
{
	ARG_UNUSED(dev);
	k_sem_give(&usb_sem);
}

/* LED control handler implementation */
int kbd_set_report(const struct device *dev, struct usb_setup_packet *setup, int32_t *len,
			uint8_t **data)
{
	printk("kb_out: %x, len %d", **data, *len);

	//gpio_pin_set(led1.port, led1.pin, (**data & HID_KBD_LED_NUM_LOCK));
	//gpio_pin_set(led2.port, led2.pin, (**data & HID_KBD_LED_CAPS_LOCK));

	return 0;
}

struct hid_ops kbd_ops = {
	.set_report = kbd_set_report,
	.int_in_ready = in_ready_cb,
};

static void status_cb(enum usb_dc_status_code status, const uint8_t *param)
{
	usb_status = status;
    LOG_INF("usb_status: %d", usb_status);
}

const struct device *hid_dev_kb;
const struct device *hid_dev_n_key;
const struct device *hid_vendor;

int usb_hw_init(void)
{
	int ret;

	hid_dev_kb = device_get_binding("HID_0");
	if (hid_dev_kb == NULL) {
		LOG_ERR("Cannot get USB HID Device");
		return 0;
	}

	// hid_dev_n_key = device_get_binding("HID_1");
	// if (hid_dev_n_key == NULL) {
	//	LOG_ERR("Cannot get USB HID 1 Device");
	//	return 0;
	// }

    // hid_vendor = device_get_binding("HID_2");
	// if (hid_vendor == NULL) {
	//	LOG_ERR("Cannot get USB HID 2 Device");
	//	return 0;
	// }

	usb_hid_register_device(hid_dev_kb, hid_report_kb_desc, sizeof(hid_report_kb_desc), &kbd_ops);
	// usb_hid_register_device(hid_dev_n_key, hid_report_n_keys_desc, sizeof(hid_report_n_keys_desc), &kbd_ops);
	//usb_hid_register_device(hid_vendor, hid_report_vendor_defined, sizeof(hid_report_vendor_defined), &kbd_ops);

	usb_hid_init(hid_dev_kb);
	// usb_hid_init(hid_dev_n_key);
	//usb_hid_init(hid_vendor);

	ret = usb_enable(status_cb);
	if (ret != 0) {
		LOG_ERR("Failed to enable USB");
		return 0;
	}

	LOG_INF("Enable USB, usb hw init");
}

void app_usb_mode_exit(void)
{
    usb_disable();
    usb_connected_ok = 0;
    printk("usb mode exit\r\n");
}

_attribute_ram_code_sec_ int app_normal_key_report_to_usb(unsigned char *buf)
{
    static unsigned char kb[8] = {0, 0, 1, 0, 0, 0, 0, 0};
    unsigned char  status = 0;
	#if 0
    status=app_usb_ep_is_idle(HID_KEYBOARD_IN_ENDPOINT_NUM);
    if(status!=0)
    {
        return status;
    }
	#endif
    memcpy(&kb[0], &buf[0], 8);
    //return app_usb_epin_send(HID_KEYBOARD_IN_ENDPOINT_ADDRESS, tmp, 8);
	return hid_int_ep_write(hid_dev_kb, kb, sizeof(kb), NULL);
}


_attribute_ram_code_sec_ int app_all_key_report_to_usb(unsigned char *buf)
{
    static unsigned char kb[17] ={8,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
    unsigned char  status = 0;
	#if 0
    status=app_usb_ep_is_idle(HID_KEYBOARD_IN_ENDPOINT_NUM);
    if(status!=0)
    {
        return status;
    }
	#endif
    memcpy(&kb[1], &buf[0], 16);
    //return app_usb_epin_send(HID_KEYBOARD_IN_ENDPOINT_ADDRESS, tmp, 8);
	return hid_int_ep_write(hid_dev_n_key, kb, sizeof(kb), NULL);
}

_attribute_ram_code_sec_ int app_consume_key_report_to_usb(unsigned char *buf)
{
    static unsigned char kb[3]={2,0,0}; // first is report id
    unsigned char  status = 0;
	#if 0
    status=app_usb_ep_is_idle(HID_KEYBOARD_IN_ENDPOINT_NUM);
    if(status!=0)
    {
        return status;
    }
	#endif
    memcpy(&kb[1],&buf[0],2);
    //return app_usb_epin_send(HID_KEYBOARD_IN_ENDPOINT_ADDRESS, tmp, 8);
	return hid_int_ep_write(hid_dev_n_key, kb, sizeof(kb), NULL);
}

_attribute_ram_code_sec_ void app_usb_report_to_pc(void)
{
    unsigned char  *p= pp_fifo_get_ptr(&tx_fifo);
    if(p!=0)
    {
        unsigned char cmd = p[1];
        int ret = 0;

        if(cmd==MOUSE_DATA)
        {
            //ret=app_mouse_report_to_usb(&p[3]);
        }
        else if(cmd == NORMAL_KB_DATA_CMD)
        {
            ret = app_normal_key_report_to_usb(&p[2]);
        }
        else if(cmd == CONSUME_KB_DATA_CMD)
        {
            ret = app_consume_key_report_to_usb(&p[2]);
        }
        else if(cmd == SYSTEM_KB_DATA_CMD)
        {
            //ret = app_system_key_report_to_usb(&p[2]);
        }
        else if(cmd == ALL_KB_DATA_CMD)
        {
            ret = app_all_key_report_to_usb(&p[2]);
        }
        if(ret == 0)
        {
            pp_fifo_pop(&tx_fifo);
        }
    }
}

_attribute_ram_code_sec_ void app_usb_main_loop(void)
{
    if (usb_connected_ok == 1) {
	    app_usb_report_to_pc();
    }
}

_attribute_ram_code_sec_ void check_vbus(void)
{
    static int8_t vbus_cnt = 0;

    if(gpio_pin_get_dt(&vbus_check_pin) == VBUS_5V_CHECK_PIN_USB_IN_LEVEL) {
        if (vbus_cnt < APP_VBUS_CHECK_DE_JT_CNT) {
            vbus_cnt ++;
        } else {
            vbus_status=1;
        }
    } else {
        if (vbus_cnt > -APP_VBUS_CHECK_DE_JT_CNT) {
            vbus_cnt --;
        } else {
            vbus_status=0;
        }
    }

}

// _attribute_ram_code_sec_ void check_vbus(void)
// {
// #if (HW_BOARD_TYPE == HW_PRJ_KEYBOARD || HW_BOARD_TYPE == HW_DIGIT_KEYBOARD)
//     vbus_status = 1;
//     if(gpio_pin_get_dt(&vbus_check_pin) == 0)
//     {
//         vbus_status = 0;
//     }
// #endif
// }

_attribute_ram_code_sec_ uint8_t app_is_usb_det_in(void)
{
    return (vbus_status == 1);
}

_attribute_ram_code_sec_ void app_usb_status_check(void)
{
    static unsigned int last_usb_status = 0xff;

        check_vbus();

        if(last_vbus_status != vbus_status)
        {
            printk("vbus_status = %d\r\n",vbus_status);
            if(last_vbus_status == 0)
            {
                // TODO:app_usb_bus_reset_init();
            }
            else
            {
                 usb_status = 0;
            }
            last_vbus_status = vbus_status;
        }

        if(last_usb_status != usb_status)
        {
            if(usb_status == USB_DC_CONFIGURED)
            {
                printk("mode is usb mode\r\n");
                // TODO:gpio_set_level(MODE_LED_PIN, LED_IS_ON);
                // TODO: gpio_set_level(PAIR_LED_PIN,LED_IS_OFF);
                usb_connected_ok = 1;
                if(fun_mode == KB_MODE_2P4G)
                {
                    // TODO:p24g_send_sm_msg(P24G_SM_CMD_SET_KB_MODE, P24G_KB_MODE_USB, 0, 0);
                }
                else
                {
                    printk("ble enter idle mode\r\n");
                    // TODO:ble_mode_enter_idle();
                }
            }
            else if(usb_status == USB_DC_DISCONNECTED)
            {
                // TODO:gpio_set_level(MODE_LED_PIN, LED_IS_OFF);
                usb_connected_ok = 0;

                if(fun_mode == KB_MODE_2P4G)
                {
                    printk("mode is 2.4g\r\n");
                    // TODO:p24g_send_sm_msg(P24G_SM_CMD_SET_KB_MODE, P24G_KB_MODE_2P4G, 0, 0);
                }
            }
            last_usb_status = usb_status;
        }
}
