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

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/gatt.h>

#include "app_public.h"

#define LOG_LEVEL LOG_LEVEL_DBG
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(app_ble);

#define DEVICE_NAME			CONFIG_BT_DEVICE_NAME
#define DEVICE_NAME_LEN		(sizeof(DEVICE_NAME) - 1)

static struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
	BT_DATA_BYTES(BT_DATA_GAP_APPEARANCE, BT_BYTES_LIST_LE16(BT_APPEARANCE_HID_KEYBOARD)),
	BT_DATA_BYTES(BT_DATA_UUID16_ALL, BT_UUID_16_ENCODE(BT_UUID_HIDS_VAL)),
	BT_DATA_BYTES(BT_DATA_MANUFACTURER_DATA, 0x06, 0x00, 0x03, 0x00, 0x80),
	// BT_DATA(BT_DATA_NAME_COMPLETE, DEVICE_NAME, DEVICE_NAME_LEN),
};

static struct bt_data sd[] = {
	// BT_DATA(BT_DATA_NAME_COMPLETE, DEVICE_NAME, DEVICE_NAME_LEN),
};

struct bt_conn *connected_handle;
static struct k_work_delayable ble_change_pipe_delayed_work;
static struct k_work_delayable ble_start_pairing_delayed_work;
uint8_t pair_flag;
volatile uint8_t ble_status = 0;
uint8_t connect_complete = 0;
uint32_t tick_connected = 0;
uint8_t need_enter_sleep = 0;
uint8_t need_save_info = 0;

bt_addr_le_t ble_addr = {.type = BT_ADDR_LE_RANDOM, .a = {0xC0, 0x79, 0xEE, 0x00, 0x01, 0xD1}};

struct bt_le_adv_param adv_param = {
	.id = BT_ID_DEFAULT,
	.sid = 0,
	.secondary_max_skip = 0,
	.options = (BT_LE_ADV_OPT_CONNECTABLE |BT_LE_ADV_OPT_USE_NAME |
				BT_LE_ADV_OPT_ONE_TIME |BT_LE_ADV_OPT_FORCE_NAME_IN_AD),
	.interval_min = 0x0020, /* 20 ms */
	.interval_max = 0x0020, /* 20 ms */
	.peer = NULL,
};

static const struct bt_le_conn_param conn_update_param = {
		.interval_min = 6, // 7.5ms
		.interval_max = 6, // 7.5ms
		.latency = 0x2c,
		.timeout = 400,         // 4s
};

static void connected(struct bt_conn *conn, uint8_t err)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	if (err) {
		printk("Failed to connect to %s (%u)\n", addr, err);
		return;
	}

	printk("Connected %s\n", addr);
	connected_handle = conn;

	if (bt_conn_set_security(conn, BT_SECURITY_L2)) {
		printk("Failed to set security\n");
	}

    ble_status = CON_BEGIN_BLE_STATUS;

	int ret = bt_conn_le_param_update(conn, &conn_update_param);
	printk("bt_conn_le_param_update %d\n", ret);
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));
	connected_handle = NULL;
	printk("Disconnected from %s (reason 0x%02x)\n", addr, reason);

	connect_complete = 0;

    if (user_active_disconnect == MULTI_DEVICE_CHANGE_PIPE_1 ||
		user_active_disconnect == MULTI_DEVICE_CHANGE_PIPE_2 ||
		user_active_disconnect == MULTI_DEVICE_CHANGE_PIPE_3 ||
		user_active_disconnect == MULTI_DEVICE_CHANGE_PIPE_4) {
		ble_status = BLE_STATUS_INIT;
		start_change_ble_pipe_by_delay_work();
	} else if(user_active_disconnect == MULTI_DEVICE_PAIR_PIPE_1 ||
			  user_active_disconnect == MULTI_DEVICE_PAIR_PIPE_2 ||
			  user_active_disconnect == MULTI_DEVICE_PAIR_PIPE_3 ||
			  user_active_disconnect == MULTI_DEVICE_PAIR_PIPE_4) {
		user_active_disconnect = 0;
		ble_status = BLE_STATUS_INIT;
		start_pairing_by_delay_work();
	} else {
		ble_status = IDLE_BLE_STATUS;
	}
}

static void security_changed(struct bt_conn *conn, bt_security_t level,
			     enum bt_security_err err)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	if (!err) {
		printk("Security changed: %s level %u\n", addr, level);
		if (level >= BT_SECURITY_L2) {
			printk("Encryption is complete and sufficient.\\n");
			ble_status = CON_FIRST_SMP_BLE_STATUS;
			connect_complete = 1;
			need_save_info = 1;
	}
	} else {
		printk("Security failed: %s level %u err %d\n", addr, level,
		       err);
	}
}

static void le_param_updated(struct bt_conn *conn, uint16_t interval,
			     uint16_t latency, uint16_t timeout)
{
	printk("LE conn param updated: int 0x%04x lat %d to %d", interval, latency, timeout);

}

BT_CONN_CB_DEFINE(conn_callbacks) = {
	.connected = connected,
	.disconnected = disconnected,
	.security_changed = security_changed,
	.le_param_updated = le_param_updated,
};

static void bt_ready(int err)
{
	if (err) {
		printk("Bluetooth init failed (err %d)\n", err);
		return;
	}

	hog_init();

	if (IS_ENABLED(CONFIG_SETTINGS)) {
		settings_load();
	}

	ble_status = IDLE_BLE_STATUS;
	#if 1
	adv_param.id = flash_dev_info.mast_id;
	printk("Bluetooth initialized id %d\n", adv_param.id);

	ble_addr.a.val[5] = 0xD1 + flash_dev_info.mast_id;
	LOG_HEXDUMP_INF(ble_addr.a.val, 6, "ble_addr:");
	err = bt_le_adv_start(&adv_param, ad, ARRAY_SIZE(ad), sd, ARRAY_SIZE(sd));
	if (err) {
		printk("Advertising failed to start (err %d)\n", err);
		return;
	}
	printk("Advertising successfully started\n");
	#else

	#endif
}

static void auth_passkey_display(struct bt_conn *conn, unsigned int passkey)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	printk("Passkey for %s: %06u\n", addr, passkey);
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

void start_change_ble_pipe_by_delay_work(void)
{
	int ret = k_work_schedule(&ble_change_pipe_delayed_work, K_MSEC(50));
	if (ret != 1) {
		printk("Failed to schedule work. Error code: %d\n", ret);
		return;
	}
}

_attribute_ram_code_sec_noinline_ void ble_change_pipe_delayed_work_handler(struct k_work *work) {

	uint8_t pipe = (user_active_disconnect - 1);

    if(pipe != flash_dev_info.mast_id)
    {
        flash_dev_info.mast_id = pipe;
		// save info when smp complete
		//save_dev_info();
		bt_le_adv_stop();
		printk("switch to mast_id: %d\r\n", flash_dev_info.mast_id);
		adv_param.id = flash_dev_info.ble_id[flash_dev_info.mast_id];
		printk("adv_param id %d\n", adv_param.id);

		ble_status = IDLE_BLE_STATUS;
	}
}


void start_pairing_by_delay_work(void)
{
	int ret = k_work_schedule(&ble_start_pairing_delayed_work, K_MSEC(50));
	if (ret != 1) {
		printk("Failed to schedule pariring work. Error code: %d\n", ret);
		return;
	}
}

void ble_start_pairing_delayed_work_handler(struct k_work *work)
{
	int err;

    printk("ble_start_pairing_delayed_work_handler mast id %d\r\n", flash_dev_info.mast_id);

	err = bt_le_adv_stop();
	if (err) {
		printk("Advertising failed to stop (err %d)\n", err);
		return;
	}

	flash_dev_info.slave_mac_addr[flash_dev_info.mast_id]++;
	ble_addr.a.val[4] = flash_dev_info.slave_mac_addr[flash_dev_info.mast_id];
	ble_addr.a.val[5] = 0xD1 + flash_dev_info.mast_id;
	LOG_HEXDUMP_INF(&ble_addr.a.val[0], 6, "bt id reset new mac:");
	printk("user bt_id_reset %d\n", flash_dev_info.ble_id[flash_dev_info.mast_id]);
	// reset bt id with new address
	int new_id = bt_id_reset(flash_dev_info.ble_id[flash_dev_info.mast_id], &ble_addr, NULL);
	if (new_id < 0) {
		printk("bt_id_reset (err %d)\n", new_id);
		return;
	}
	printk("bt_id %d_reset success\n", new_id);
	flash_dev_info.ble_id[flash_dev_info.mast_id] = new_id;
	adv_param.id = flash_dev_info.ble_id[flash_dev_info.mast_id];
	printk("adv_param id %d\n", adv_param.id);

	ble_status = IDLE_BLE_STATUS;
}

void ble_init(void)
{
	int err;

	if (flash_dev_info.ble_id[0] == 0xff) {
		ble_addr.a.val[4] = 0x00;
		ble_addr.a.val[5] = 0xD1;
		flash_dev_info.ble_id[0] = bt_id_create(&ble_addr, NULL);
		printk("New ID created: %d\n", flash_dev_info.ble_id[0]);
	 } else {
		ble_addr.a.val[4] = flash_dev_info.slave_mac_addr[0];
		ble_addr.a.val[5] = 0xD1;
		flash_dev_info.ble_id[0] = bt_id_create(&ble_addr, NULL);
		printk("ID created: %d\n", flash_dev_info.ble_id[0]);
	 }

	if( flash_dev_info.ble_id[1] == 0xff) {
		ble_addr.a.val[4] = 0x00;
		ble_addr.a.val[5] = 0xD2;
		flash_dev_info.ble_id[1] = bt_id_create(&ble_addr, NULL);
		printk("New ID2 created: %d\n", flash_dev_info.ble_id[1]);
	} else {
		ble_addr.a.val[4] = flash_dev_info.slave_mac_addr[1];
		ble_addr.a.val[5] = 0xD2;
		flash_dev_info.ble_id[1] = bt_id_create(&ble_addr, NULL);
		printk("ID created: %d\n", flash_dev_info.ble_id[1]);
	 }

	if( flash_dev_info.ble_id[2] == 0xff) {
		ble_addr.a.val[4] = 0x00;
		ble_addr.a.val[5] = 0xD3;
		flash_dev_info.ble_id[2] = bt_id_create(&ble_addr, NULL);
		printk("New ID3 created: %d\n", flash_dev_info.ble_id[2]);
	} else {
		ble_addr.a.val[4] = flash_dev_info.slave_mac_addr[2];
		ble_addr.a.val[5] = 0xD3;
		flash_dev_info.ble_id[2] = bt_id_create(&ble_addr, NULL);
		printk("ID created: %d\n", flash_dev_info.ble_id[2]);
	 }

	if( flash_dev_info.ble_id[3] == 0xff) {
		ble_addr.a.val[4] = 0x00;
		ble_addr.a.val[5] = 0xD4;
		flash_dev_info.ble_id[3] = bt_id_create(&ble_addr, NULL);
		printk("New ID4 created: %d\n", flash_dev_info.ble_id[3]);
	} else {
		ble_addr.a.val[4] = flash_dev_info.slave_mac_addr[3];
		ble_addr.a.val[5] = 0xD4;
		flash_dev_info.ble_id[3] = bt_id_create(&ble_addr, NULL);
		printk("ID created: %d\n", flash_dev_info.ble_id[3]);
	 }

	save_dev_info();

	adv_param.id = flash_dev_info.ble_id[flash_dev_info.mast_id];
	printk("Bluetooth initialized id %d\n", adv_param.id);

	LOG_HEXDUMP_INF(ble_addr.a.val, 6, "ble_addr:");

	err = bt_enable(bt_ready);
	if (err) {
		printk("Bluetooth init failed (err %d)\n", err);
		return 0;
	}

	k_work_init_delayable(&ble_change_pipe_delayed_work, ble_change_pipe_delayed_work_handler);
	k_work_init_delayable(&ble_start_pairing_delayed_work, ble_start_pairing_delayed_work_handler);

	#if 0
	if (IS_ENABLED(CONFIG_SAMPLE_BT_USE_AUTHENTICATION)) {
		bt_conn_auth_cb_register(&auth_cb_display);
		printk("Bluetooth authentication callbacks registered.\n");
	}
	#endif
}

/* 定义回调函数 */
static void bond_info_cb(const struct bt_bond_info *info, void *user_data)
{
    char addr_str[BT_ADDR_LE_STR_LEN];

    /* 将蓝牙地址转换为可读字符串 */
    bt_addr_le_to_str(&info->addr, addr_str, sizeof(addr_str));

    /* 打印绑定的设备地址 */
    printk("bond addr: %s\n", addr_str);
}


void ble_start_adv(void)
{
	int err;
#if 0
	bt_addr_le_t bond_addr;
	bt_addr_le_t copy_laste_bonded_addr;
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_copy(&bond_addr, BT_ADDR_LE_NONE);
	bt_foreach_bond(adv_param.id, bond_info_cb, NULL);

	bt_addr_le_to_str(&bond_addr, addr, sizeof(addr));

	printk("bond_addr address: %s\n", addr);
	bt_addr_le_to_str(&copy_laste_bonded_addr, addr, sizeof(addr));
	printk(" copy_laste_bonded_addr address: %s\n", addr);
#endif

	err = bt_le_adv_start(&adv_param, ad, ARRAY_SIZE(ad), sd, ARRAY_SIZE(sd));
	if (err) {
		printk("Advertising failed to start (err %d)\n", err);
		return;
	}

	printk("Advertising successfully started\n");
}

void disconnect_current_connection(void)
{
    if (connected_handle) {
        int err = bt_conn_disconnect(connected_handle, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
        if (err) {
            printk("disconnect error: %d\n", err);
        } else {
            printk("disconnect send\n");
        }
    }
}


void app_ble_report_to_client(void)
{
    unsigned char  *p = pp_fifo_get_ptr(&tx_fifo);
    int ret = 0;

    if(p!=0)
    {
        unsigned char cmd = p[1];

        if(cmd == MOUSE_DATA)
        {
            //unsigned char ret = blc_atts_sendHandleValueNotify(connected_handle,HID_MOUSE_REPORT_INPUT_DP_H,&p[3],sizeof(t_mouse_data)-1);
        }
        else if(cmd==NORMAL_KB_DATA_CMD)
        {
            ret = ble_nortify_keyboard_data(&p[2],8);
        }
        else if(cmd==CONSUME_KB_DATA_CMD)
        {
            ret = ble_nortify_keyboard_data(&p[2], 2);
        }
        else if(cmd==SYSTEM_KB_DATA_CMD)
        {
        }
        else if(cmd==ALL_KB_DATA_CMD)
        {
            ret = ble_nortify_all_key_data(&p[2], REPORT_ALL_KB_SIZE);
        }
        if(ret == 0)
        {
            pp_fifo_pop(&tx_fifo);
        }
    }
}

_attribute_ram_code_sec_noinline_ void app_ble_status_proc(void)
{
    static uint32_t led_tick;
    static uint8_t led_flag = 0;
    static uint32_t interval_led = 1000000;
	int err;
    static uint32_t adv_tick = 0;

	if(ble_status == CON_OK_BLE_STATUS)
    {
        app_ble_report_to_client();

		if(need_save_info)
        {
            need_save_info = 0;
            printk("save info\r\n");
            save_dev_info();
        }
    }
    else if(ble_status == IDLE_BLE_STATUS)
    {
        connect_complete = 0;
        if(need_enter_sleep)
        {
            //app_enter_sleep(need_enter_sleep);
            return;
        }
        else
        {
			//start advertising
			printk("start adv\r\n");
            ble_start_adv();
        }
		adv_tick = k_uptime_get_32();

        if(pair_flag)
        {
             ble_status = ADV_PAIR_BLE_STATUS;
             printk("pair..\r\n");
             interval_led = 100000;
        }
        else
        {
             ble_status = ADV_RECONNECT_BLE_STATUS;
             interval_led = 1000000;
             printk("reconnect\r\n");
        }
         printk("adv\r\n");
         led_tick = k_uptime_get_32();
         led_flag = LED_IS_ON;
    }
    else if(ble_status == ADV_PAIR_BLE_STATUS)
    {
        #if APP_PM_ENABLE
		if ((k_uptime_get_32() - adv_tick) > PAIR_TIMEOUT_US)
        {
			err = bt_le_adv_stop();
			if (err) {
				printk("Advertising failed to stop (err %d)\n", err);
			}
            need_enter_sleep = 1;
            ble_status = IDLE_BLE_STATUS;
        }
        #endif
    }
    else if(ble_status == ADV_RECONNECT_BLE_STATUS)
    {
        #if APP_PM_ENABLE
		if ((k_uptime_get_32() - adv_tick) > RECONN_TIMEOUT_US)
        {
			err = bt_le_adv_stop();
			if (err) {
				printk("Advertising failed to stop (err %d)\n", err);
			}
			need_enter_sleep = 1;
            ble_status = IDLE_BLE_STATUS;
        }
        #endif
    }
    else if(connect_complete == 1)
    {
         led_tick = k_uptime_get_32();
         {
            printk("con is ok\r\n");
            ble_status = CON_OK_BLE_STATUS;
			#if 0
            gpio_set_level(PAIR_LED_PIN,LED_IS_ON);
			#endif
            pp_fifo_reset(&tx_fifo);
         }
    }

    if(connect_complete == 0)
    {
		#if 0
        if(clock_time_exceed(led_tick, interval_led))
        {
            led_tick=clock_time();
            led_flag=1-led_flag;
            gpio_set_level(PAIR_LED_PIN,led_flag);
        }
		#endif
    }
}

_attribute_ram_code_sec_noinline_ void app_ble_main_loop(void)
{
    app_ble_status_proc();
}
