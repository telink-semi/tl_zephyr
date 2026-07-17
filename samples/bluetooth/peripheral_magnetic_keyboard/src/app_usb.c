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
#include <zephyr/drivers/gpio.h>
#include <zephyr/settings/settings.h>

#include <zephyr/usb/usb_device.h>
#include <zephyr/usb/class/hid.h>
#include <zephyr/usb/class/usb_hid.h>


#include "app_public.h"

#define LOG_LEVEL LOG_LEVEL_DBG
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(app_usb);


volatile unsigned int  vbus_status = 0;
volatile unsigned int usb_connected_ok = 0;

 static const uint8_t hid_report_kb_desc[] = HID_KEYBOARD_REPORT_DESC();


//static const uint8_t hid_report_ms_desc[] = HID_MOUSE_REPORT_DESC(5);
static const uint8_t hid_report_n_keys_desc[] = HID_N_KEY_REPORT_DESC();

static const uint8_t hid_report_vendor_defined[] = HID_MOUSE_REPORT_DESC(5);


enum usb_dc_status_code usb_status;

// static K_SEM_DEFINE(usb_sem, 1, 1); /* starts off "available" */
static void in_ready_cb(const struct device *dev)
{
	ARG_UNUSED(dev);
	// k_sem_give(&usb_sem);
}

static void out_ready_cb(const struct device *dev)
{
	uint8_t buf[CONFIG_HID_INTERRUPT_EP_MPS];
	uint32_t len = 0;
	int ret;

	ret = hid_int_ep_read(dev, buf, sizeof(buf), &len);

	if (ret == 0 && len > 0) {
		LOG_HEXDUMP_INF(buf, len, "OUT ep received");
        if (len == 1) {
            extern struct gpio_dt_spec cap_led_pin;
            extern struct gpio_dt_spec num_led_pin;

            gpio_pin_set_dt(&cap_led_pin, (*buf & HID_KBD_LED_CAPS_LOCK));
            gpio_pin_set_dt(&num_led_pin, (*buf & HID_KBD_LED_NUM_LOCK));
        }
	}
}

/* LED control handler implementation */
int kbd_set_report(const struct device *dev, struct usb_setup_packet *setup, int32_t *len,
			uint8_t **data)
{
	LOG_INF("kb_out: %x, len %d", **data, *len);

    extern struct gpio_dt_spec cap_led_pin;
    extern struct gpio_dt_spec num_led_pin;

    gpio_pin_set_dt(&cap_led_pin, (**data & HID_KBD_LED_CAPS_LOCK));
    gpio_pin_set_dt(&num_led_pin, (**data & HID_KBD_LED_NUM_LOCK));

	return 0;
}

struct hid_ops kbd_ops = {
	.set_report = kbd_set_report,
	.int_in_ready = in_ready_cb,
    #ifdef CONFIG_ENABLE_HID_INT_OUT_EP
	.int_out_ready = out_ready_cb,
    #endif
};

static void status_cb(enum usb_dc_status_code status, const uint8_t *param)
{
	usb_status = status;
    LOG_INF("usb_status: %d", usb_status);
}

const struct device *hid_dev_kb;
const struct device *hid_dev_n_key;
const struct device *hid_vendor;

const struct device *hid_vendor_4;
const struct device *hid_vendor_5;
const struct device *hid_vendor_6;
int usb_hw_init(void)
{
	int ret;

	hid_dev_kb = device_get_binding("HID_0");
	if (hid_dev_kb == NULL) {
		LOG_ERR("Cannot get USB HID Device");
        return -ENODEV;
	}

	hid_dev_n_key = device_get_binding("HID_1");
	if (hid_dev_n_key == NULL) {
		LOG_ERR("Cannot get USB HID 1 Device");
        return -ENODEV;
	}

    hid_vendor = device_get_binding("HID_2");
	if (hid_vendor == NULL) {
		LOG_ERR("Cannot get USB HID 2 Device");
        return -ENODEV;
	}

	usb_hid_register_device(hid_dev_kb, hid_report_kb_desc, sizeof(hid_report_kb_desc), &kbd_ops);
	usb_hid_register_device(hid_dev_n_key, hid_report_n_keys_desc, sizeof(hid_report_n_keys_desc), &kbd_ops);
	usb_hid_register_device(hid_vendor, hid_report_vendor_defined, sizeof(hid_report_vendor_defined), &kbd_ops);

    ret = usb_hid_set_proto_code(hid_dev_kb, HID_BOOT_IFACE_CODE_KEYBOARD);
    if (ret) {
        LOG_WRN("Failed to set HID proto code for HID_0 (%d)", ret);
    }

    ret = usb_hid_set_proto_code(hid_dev_n_key, HID_BOOT_IFACE_CODE_NONE);
    if (ret) {
        LOG_WRN("Failed to set HID proto code for HID_1 (%d)", ret);
    }

    ret = usb_hid_set_proto_code(hid_vendor, HID_BOOT_IFACE_CODE_NONE);
    if (ret) {
        LOG_WRN("Failed to set HID proto code for HID_2 (%d)", ret);
    }

	usb_hid_init(hid_dev_kb);
	usb_hid_init(hid_dev_n_key);
	usb_hid_init(hid_vendor);

	ret = usb_enable(status_cb);
	if (ret != 0) {
		LOG_ERR("Failed to enable USB");
		return ret;
	}

	LOG_INF("Enable USB, usb hw init");
    return 0;
}

void app_usb_mode_exit(void)
{
    usb_disable();
    usb_connected_ok = 0;
    LOG_ERR("usb mode exit\r\n");
}

_attribute_ram_code_sec_ int app_normal_key_report_to_usb(unsigned char *buf)
{
    static unsigned char kb[8] = {0, 0, 1, 0, 0, 0, 0, 0};
	#if 0
    unsigned char  status = 0;
    status=app_usb_ep_is_idle(HID_KEYBOARD_IN_ENDPOINT_NUM);
    if(status!=0)
    {
        return status;
    }
	#endif
    tmemcpy(&kb[0], &buf[0], 8);
    //return app_usb_epin_send(HID_KEYBOARD_IN_ENDPOINT_ADDRESS, tmp, 8);
	return hid_int_ep_write(hid_dev_kb, kb, sizeof(kb), NULL);
}


_attribute_ram_code_sec_ int app_all_key_report_to_usb(unsigned char *buf)
{
    static unsigned char kb[17] ={8,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
	#if 0
    unsigned char  status = 0;
    status=app_usb_ep_is_idle(HID_KEYBOARD_IN_ENDPOINT_NUM);
    if(status!=0)
    {
        return status;
    }
	#endif
    tmemcpy(&kb[1], &buf[0], 16);
    //return app_usb_epin_send(HID_KEYBOARD_IN_ENDPOINT_ADDRESS, tmp, 8);
	return hid_int_ep_write(hid_dev_n_key, kb, sizeof(kb), NULL);
}

_attribute_ram_code_sec_ int app_consume_key_report_to_usb(unsigned char *buf)
{
    static unsigned char kb[3]={2,0,0}; // first is report id
	#if 0
    unsigned char  status = 0;
    status=app_usb_ep_is_idle(HID_KEYBOARD_IN_ENDPOINT_NUM);
    if(status!=0)
    {
        return status;
    }
	#endif
    tmemcpy(&kb[1],&buf[0],2);
    //return app_usb_epin_send(HID_KEYBOARD_IN_ENDPOINT_ADDRESS, tmp, 8);
	return hid_int_ep_write(hid_dev_n_key, kb, sizeof(kb), NULL);
}

_attribute_ram_code_sec_ unsigned char app_system_key_report_to_usb(unsigned char *buf)
{
    static unsigned char kb[2]={3,0}; // first is report id
	#if 0
    unsigned char  status = 0;
    status=app_usb_ep_is_idle(HID_KEYBOARD_IN_ENDPOINT_NUM);
    if(status!=0)
    {
        return status;
    }
	#endif
    tmemcpy(&kb[1],&buf[0],1);
    //return app_usb_epin_send(HID_KEYBOARD_IN_ENDPOINT_ADDRESS, tmp, 8);
	return hid_int_ep_write(hid_dev_n_key, kb, sizeof(kb), NULL);
}

_attribute_ram_code_sec_ void app_usb_report_to_pc(void)
{
    unsigned char  *p= pp_fifo_get_ptr(&d25fKbTxFifo);
    if(p!=0)
    {
        unsigned char cmd = p[1];
        int ret = 0; 

        if(cmd==MOUSE_DATA)
        {
            // ret=app_mouse_report_to_usb(&p[3]);
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
            ret = app_system_key_report_to_usb(&p[2]);
        }
        else if(cmd == ALL_KB_DATA_CMD)
        {
            ret = app_all_key_report_to_usb(&p[2]);
        }
        if(ret == 0)
        {
            pp_fifo_pop(&d25fKbTxFifo);
        }
    }
}

_attribute_ram_code_sec_ void app_usb_main_loop(void)
{
    if (usb_connected_ok == 1)
    {
	    app_usb_report_to_pc();
    }
}


_attribute_ram_code_sec_ void app_usb_status_check(void)
{
    static unsigned int last_usb_status = 0xff;

    check_vbus();

    if(last_usb_status != usb_status)
    {
        if(usb_status == USB_DC_CONFIGURED)
        {
            LOG_INF("mode is usb mode\r\n");
            // TODO:gpio_set_level(MODE_LED_PIN, LED_IS_ON);
            // TODO: gpio_set_level(PAIR_LED_PIN,LED_IS_OFF);
            usb_connected_ok = 1;
            if(fun_mode == KB_MODE_2P4G)
            {
                // TODO:p24g_send_sm_msg(P24G_SM_CMD_SET_KB_MODE, KB_MODE_USB, 0, 0);
            }
            else
            {
                LOG_INF("ble enter idle mode\r\n");
                // TODO:ble_mode_enter_idle();
            }
        }
        else if(usb_status == USB_DC_DISCONNECTED)
        {
            // TODO:gpio_set_level(MODE_LED_PIN, LED_IS_OFF);
            usb_connected_ok = 0;

            if(fun_mode == KB_MODE_2P4G)
            {
                LOG_INF("mode is 2.4g\r\n");
                // TODO:p24g_send_sm_msg(P24G_SM_CMD_SET_KB_MODE, KB_MODE_2P4G, 0, 0);
            }
        }
        last_usb_status = usb_status;
    }
}