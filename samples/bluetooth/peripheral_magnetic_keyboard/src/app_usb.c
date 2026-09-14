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

#include <zephyr/usb/usbd.h>
#include <zephyr/usb/class/hid.h>
#include <zephyr/usb/class/usbd_hid.h>


#include "app_public.h"

#define LOG_LEVEL LOG_LEVEL_DBG
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(app_usb);

/* USB device context with Telink VID/PID */
USBD_DEVICE_DEFINE(app_usbd,
		   DEVICE_DT_GET(DT_NODELABEL(zephyr_udc0)),
		   0x248A, 0x3228);

USBD_DESC_LANG_DEFINE(app_lang);
USBD_DESC_MANUFACTURER_DEFINE(app_mfr, "Telink");
USBD_DESC_PRODUCT_DEFINE(app_product, "Telink HID Keyboard");
USBD_DESC_SERIAL_NUMBER_DEFINE(app_sn);
USBD_DESC_CONFIG_DEFINE(fs_cfg_desc, "FS Configuration");
USBD_DESC_CONFIG_DEFINE(hs_cfg_desc, "HS Configuration");

static const uint8_t attributes = USB_SCD_REMOTE_WAKEUP;

USBD_CONFIGURATION_DEFINE(app_fs_config, attributes, 100, &fs_cfg_desc);
USBD_CONFIGURATION_DEFINE(app_hs_config, attributes, 100, &hs_cfg_desc);

volatile unsigned int vbus_status = 0;
volatile unsigned int usb_connected_ok = 0;
volatile unsigned int usb_suspended = 0;

 static const uint8_t hid_report_kb_desc[] = HID_KEYBOARD_REPORT_DESC();


//static const uint8_t hid_report_ms_desc[] = HID_MOUSE_REPORT_DESC(5);
static const uint8_t hid_report_n_keys_desc[] = HID_N_KEY_REPORT_DESC();

static const uint8_t hid_report_vendor_defined[] = HID_MOUSE_REPORT_DESC(5);


enum usb_conn_status usb_status;

// static K_SEM_DEFINE(usb_sem, 1, 1); /* starts off "available" */
static void kbd_led_set(uint8_t report)
{
	extern struct gpio_dt_spec cap_led_pin;
	extern struct gpio_dt_spec num_led_pin;

	gpio_pin_set_dt(&cap_led_pin, (report & HID_KBD_LED_CAPS_LOCK));
	gpio_pin_set_dt(&num_led_pin, (report & HID_KBD_LED_NUM_LOCK));
}

/* LED control handler implementation (control pipe SET_REPORT) */
static int kb_set_report(const struct device *dev,
			 const uint8_t type, const uint8_t id,
			 const uint16_t len, const uint8_t *const buf)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(type);
	ARG_UNUSED(id);

	LOG_INF("set report len %u", len);

	if (len > 0) {
		kbd_led_set(buf[0]);
	}

	return 0;
}

/* Output report received through the interrupt OUT pipe */
static void kb_output_report(const struct device *dev,
			     const uint16_t len, const uint8_t *const buf)
{
	ARG_UNUSED(dev);

	LOG_HEXDUMP_INF(buf, len, "OUT ep received");

	if (len > 0) {
		kbd_led_set(buf[0]);
	}
}

static int kb_get_report(const struct device *dev,
			 const uint8_t type, const uint8_t id,
			 const uint16_t len, uint8_t *const buf)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(type);
	ARG_UNUSED(id);
	ARG_UNUSED(len);
	ARG_UNUSED(buf);

	return 0;
}

static void kb_set_protocol(const struct device *dev, const uint8_t proto)
{
	ARG_UNUSED(dev);

	LOG_INF("protocol changed to %s", proto == 0U ? "Boot" : "Report");
}

#define KB_REPORT_BUF_CNT 2

static unsigned char kb_buf[KB_REPORT_BUF_CNT][8] = {
	{0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0},
};
static volatile bool kb_inflight[KB_REPORT_BUF_CNT];

static unsigned char ak_buf[KB_REPORT_BUF_CNT][17] = {
	{8,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
	{8,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
};
static volatile bool ak_inflight[KB_REPORT_BUF_CNT];

static unsigned char ck_buf[KB_REPORT_BUF_CNT][3] = {{2,0,0}, {2,0,0}};
static volatile bool ck_inflight[KB_REPORT_BUF_CNT];

static unsigned char sk_buf[KB_REPORT_BUF_CNT][2] = {{3,0}, {3,0}};
static volatile bool sk_inflight[KB_REPORT_BUF_CNT];

static int kb_get_free_slot(const volatile bool *inflight)
{
	for (int i = 0; i < KB_REPORT_BUF_CNT; i++) {
		if (!inflight[i]) {
			return i;
		}
	}
	return -1;
}

static void kb_input_report_done(const struct device *dev,
				 const uint8_t *const report)
{
	ARG_UNUSED(dev);

	for (int i = 0; i < KB_REPORT_BUF_CNT; i++) {
		if (report == kb_buf[i])
		{ 
			kb_inflight[i] = false;
			return;
		}
		if (report == ak_buf[i])
		{
			ak_inflight[i] = false;
			return;
		}
		if (report == ck_buf[i])
		{
			ck_inflight[i] = false;
			return;
		}
		if (report == sk_buf[i])
		{
			sk_inflight[i] = false;
			return;
		}
	}
}

static struct hid_device_ops kbd_ops = {
	.get_report = kb_get_report,
	.set_report = kb_set_report,
	.set_protocol = kb_set_protocol,
	.output_report = kb_output_report,
	.input_report_done = kb_input_report_done,
};

static void app_usb_msg_cb(struct usbd_context *const uds_ctx,
			   const struct usbd_msg *const msg)
{
	ARG_UNUSED(uds_ctx);

	if (msg->type == USBD_MSG_CONFIGURATION) {
		usb_status = USB_CONFIGURED;
		LOG_INF("USB configured");
	} else if (msg->type == USBD_MSG_VBUS_REMOVED) {
		usb_status = USB_DISCONNECTED;
		LOG_INF("USB disconnected");
	} else if (msg->type == USBD_MSG_RESET) {
		usb_suspended = 0;
		LOG_INF("USB reset");
	} else if (msg->type == USBD_MSG_SUSPEND) {
		usb_suspended = 1;
		LOG_INF("USB suspended by host");
	} else if (msg->type == USBD_MSG_RESUME) {
		usb_suspended = 0;
		LOG_INF("USB resumed by host");
	}
}

const struct device *hid_dev_kb;
const struct device *hid_dev_n_key;
const struct device *hid_vendor;

int usb_hw_init(void)
{
	int ret;

	hid_dev_kb = DEVICE_DT_GET(DT_NODELABEL(hid_dev_0));
	if (!device_is_ready(hid_dev_kb)) {
		LOG_ERR("Cannot get USB HID Device");
		return -ENODEV;
	}

	hid_dev_n_key = DEVICE_DT_GET(DT_NODELABEL(hid_dev_1));
	if (!device_is_ready(hid_dev_n_key)) {
		LOG_ERR("Cannot get USB HID 1 Device");
		return -ENODEV;
	}

	hid_vendor = DEVICE_DT_GET(DT_NODELABEL(hid_dev_2));
	if (!device_is_ready(hid_vendor)) {
		LOG_ERR("Cannot get USB HID 2 Device");
		return -ENODEV;
	}

	ret = hid_device_register(hid_dev_kb, hid_report_kb_desc,
				  sizeof(hid_report_kb_desc), &kbd_ops);
	if (ret) {
		LOG_ERR("Failed to register HID_0 (%d)", ret);
		return ret;
	}

	ret = hid_device_register(hid_dev_n_key, hid_report_n_keys_desc,
				  sizeof(hid_report_n_keys_desc), &kbd_ops);
	if (ret) {
		LOG_ERR("Failed to register HID_1 (%d)", ret);
		return ret;
	}

	ret = hid_device_register(hid_vendor, hid_report_vendor_defined,
				  sizeof(hid_report_vendor_defined), &kbd_ops);
	if (ret) {
		LOG_ERR("Failed to register HID_2 (%d)", ret);
		return ret;
	}

	/* String descriptors */
	usbd_add_descriptor(&app_usbd, &app_lang);
	usbd_add_descriptor(&app_usbd, &app_mfr);
	usbd_add_descriptor(&app_usbd, &app_product);
	usbd_add_descriptor(&app_usbd, &app_sn);

	/* Configurations */
	usbd_add_configuration(&app_usbd, USBD_SPEED_FS, &app_fs_config);
	if (usbd_caps_speed(&app_usbd) == USBD_SPEED_HS) {
		usbd_add_configuration(&app_usbd, USBD_SPEED_HS, &app_hs_config);
	}

	/* Register class instances */
	usbd_register_all_classes(&app_usbd, USBD_SPEED_FS, 1, NULL);
	if (usbd_caps_speed(&app_usbd) == USBD_SPEED_HS) {
		usbd_register_all_classes(&app_usbd, USBD_SPEED_HS, 1, NULL);
	}

	usbd_msg_register_cb(&app_usbd, app_usb_msg_cb);

	ret = usbd_init(&app_usbd);
	if (ret) {
		LOG_ERR("Failed to init USB (%d)", ret);
		return ret;
	}

	if (!usbd_can_detect_vbus(&app_usbd)) {
		ret = usbd_enable(&app_usbd);
		if (ret) {
			LOG_ERR("Failed to enable USB (%d)", ret);
			return ret;
		}
	}

	LOG_INF("Enable USB, usb hw init");
	return 0;
}

void app_usb_mode_exit(void)
{
    usbd_disable(&app_usbd);
    usb_connected_ok = 0;
    LOG_ERR("usb mode exit\r\n");
}

_attribute_ram_code_sec_ int app_normal_key_report_to_usb(unsigned char *buf)
{
	#if 0
    unsigned char  status = 0;
    status=app_usb_ep_is_idle(HID_KEYBOARD_IN_ENDPOINT_NUM);
    if(status!=0)
    {
        return status;
    }
	#endif
    int idx = kb_get_free_slot(kb_inflight);
    if (idx < 0)
    {
        return -EBUSY;
    }

    tmemcpy(&kb_buf[idx][0], &buf[0], 8);
    int ret = hid_device_submit_report(hid_dev_kb, sizeof(kb_buf[idx]), kb_buf[idx]);
    if (ret == 0)
    {
        kb_inflight[idx] = true;
    }
    return ret;
}


_attribute_ram_code_sec_ int app_all_key_report_to_usb(unsigned char *buf)
{
	#if 0
    unsigned char  status = 0;
    status=app_usb_ep_is_idle(HID_KEYBOARD_IN_ENDPOINT_NUM);
    if(status!=0)
    {
        return status;
    }
	#endif
    int idx = kb_get_free_slot(ak_inflight);
    if (idx < 0)
    {
        return -EBUSY;
    }

    tmemcpy(&ak_buf[idx][1], &buf[0], 16);
    int ret = hid_device_submit_report(hid_dev_n_key, sizeof(ak_buf[idx]), ak_buf[idx]);
    if (ret == 0)
    {
        ak_inflight[idx] = true;
    }
    return ret;
}

_attribute_ram_code_sec_ int app_consume_key_report_to_usb(unsigned char *buf)
{
	#if 0
    unsigned char  status = 0;
    status=app_usb_ep_is_idle(HID_KEYBOARD_IN_ENDPOINT_NUM);
    if(status!=0)
    {
        return status;
    }
	#endif
    int idx = kb_get_free_slot(ck_inflight);
    if (idx < 0)
    {
        return -EBUSY;
    }

    tmemcpy(&ck_buf[idx][1], &buf[0], 2);
    int ret = hid_device_submit_report(hid_dev_n_key, sizeof(ck_buf[idx]), ck_buf[idx]);
    if (ret == 0)
    {
        ck_inflight[idx] = true;
    }
    return ret;
}

_attribute_ram_code_sec_ unsigned char app_system_key_report_to_usb(unsigned char *buf)
{
	#if 0
    unsigned char  status = 0;
    status=app_usb_ep_is_idle(HID_KEYBOARD_IN_ENDPOINT_NUM);
    if(status!=0)
    {
        return status;
    }
	#endif
    int idx = kb_get_free_slot(sk_inflight);
    if (idx < 0)
    {
        return -EBUSY;
    }

    tmemcpy(&sk_buf[idx][1], &buf[0], 1);
    int ret = hid_device_submit_report(hid_dev_n_key, sizeof(sk_buf[idx]), sk_buf[idx]);
    if (ret == 0)
    {
        sk_inflight[idx] = true;
    }
    return ret;
}

_attribute_ram_code_sec_ int app_usb_try_wakeup(void)
{
    if (usb_suspended == 1) {
        usbd_wakeup_request(&app_usbd);
        LOG_INF("Request remote wakeup");
        return 1;
    }

    return 0;
}

_attribute_ram_code_sec_ void app_usb_report_to_pc(void)
{
    unsigned char  *p= pp_fifo_get_ptr(&d25fKbTxFifo);
    if(p!=0)
    {
        unsigned char cmd = p[1];
        int ret = 0; 

        if (app_usb_try_wakeup()) {
            return;
        }
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
        if(usb_status == USB_CONFIGURED)
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
        else if(usb_status == USB_DISCONNECTED)
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