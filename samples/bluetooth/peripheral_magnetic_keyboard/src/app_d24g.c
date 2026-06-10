/** @file app_2p4g.c
 *  @brief
 */

/*
 * Copyright (c) 2025 Telink Semiconductor
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

#include "app_d24g.h"
#include "app_public.h"

#define LOG_LEVEL LOG_LEVEL_DBG
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(app_2p4g);


#include "stack/multicore_comm/service/mcc.h"
#include "stack/multicore_comm/service/service_d25f.h"


#define WDT_INTV_MS     (300)


/**
 * @brief share memory message handler table
 */
static p24g_sm_cmd_handler_t p24g_cmd_table[P24G_SM_CMD_MAX] = {0};


static inline void app_wdt_init()
{
    if (wd_get_status()) {
        wd_clear_status();
    }
    wd_set_interval_ms(WDT_INTV_MS);
    /**
     * Wd_clear() must be executed before each call to wd_start() to avoid abnormal watchdog reset time because the initial count value is not 0.
     * For example, the watchdog is reset soon or a few minutes later.
     */
    wd_clear();
    wd_start();
}


_attribute_ram_code_sec_ uint8_t p24g_send_sm_msg(uint8_t type, uint8_t op, uint8_t *data, uint8_t len)
{
    static uint8_t TxBuf[64] = {0};

    p24g_evt_t *p_evt = (p24g_evt_t *)TxBuf;

    p_evt->type = type;
    p_evt->opcode = op;
    uint8_t data_len = len > (sizeof(TxBuf) - 2) ? sizeof(TxBuf) - 2 : len;

    if (data && data_len) {
        memcpy(p_evt->data, data, data_len);
        p_evt->len = data_len;
    }

    if (!mcc_d25f_shm_send_msg(TxBuf, sizeof(p24g_evt_t) + p_evt->len, TLK_SHM_MSG_2P4G)) {
        printk("d25f send sm message success\n");
    } else {
        printk("d25f send sm message fail\n");
        return 1;
    }

    return TLK_SUCCESS;
}


_attribute_ram_code_ void app_2p4g_mb_km_data_cb(uint8_t* data)
{
    if (data[0] == 0x7a) {
        data[0] = 0;
    }
}

_attribute_ram_code_sec_ void app_2p4g_d25f_sm_rx_cb(uint8_t *data, uint16_t len)
{

    if (!data || len == 0)
        return;

    uint8_t cmd = data[0];
    if (cmd < P24G_SM_CMD_MAX && p24g_cmd_table[cmd])
    {
        p24g_cmd_table[cmd](data, len);
    }
    else
    {
        printk("error! unknow sm command\n");
    }
}



/**
 * @brief resister a command handler for a specific command type
 */
uint8_t p24g_register_sm_cmd_handler(p24g_sm_cmd_e cmd, p24g_sm_cmd_handler_t handler) 
{
    uint8_t ret = TLK_ERR_INVALID_PARAM;

    if (cmd < P24G_SM_CMD_MAX && handler) {
        p24g_cmd_table[cmd] = handler;
        ret = TLK_SUCCESS;
    }else {
        printk("p24g_register_sm_cmd_handler error %x\n", cmd);
    }

    return ret;
}



void app_2p4g_dual_core_comm_init(void)
{
    mcc_mb_register_cb(TLK_MB_N22_TO_D25F_KM_DATA, (mb_recv_cb_t)app_2p4g_mb_km_data_cb);
    mcc_shm_register_cb(TLK_SHM_MSG_2P4G, app_2p4g_d25f_sm_rx_cb);
}


_attribute_ram_code_sec_ static void app_2p4g_handle_misc(uint8_t *data, uint16_t len)
{
    p24g_evt_t *p_evt = (p24g_evt_t *)data;
    switch (p_evt->opcode) {
        case P24G_SM_OP_MISC_REPORT_RATE:
            printk("report rate changed\n");
            break;

    default:
        break;
    }
}

static void app_p24g_sm_cmd_hanlder_init(void)
{
    p24g_register_sm_cmd_handler(P24G_SM_CMD_MISC,                     app_2p4g_handle_misc);
}




_attribute_ram_code_sec_ void tlk_d25f_to_n22_mode_info(kb_mode_t mode_flag)
{
    volatile uint32_t key = arch_irq_lock();
    
    uint8_t cmd[8] = {0};

    cmd[0] = TLK_MB_D25F_TO_N22_MODE;
    cmd[1] = mode_flag;

    mb_send_with_polling(cmd);
	arch_irq_unlock(key);
}

/**
 * @brief       user initialization when MCU power on or wake_up from deepSleep mode
 * @param[in]   none
 * @return      none
 */
void p24g_user_init_normal(void)
{
    app_wdt_init();

    app_2p4g_dual_core_comm_init();

    app_p24g_sm_cmd_hanlder_init();

    p24g_send_sm_msg(P24G_SM_CMD_SET_KB_MODE, P24G_KB_MODE_2P4G, 0, 0);
 
    printk("d25f kb_p24g_init end\n");
}


/**
 * @brief     BLE main loop
 * @param[in]  none.
 * @return     none.
 */
_attribute_no_inline_ void app_2p4g_main_loop(void)
{
    mcc_d25f_loop();

    wd_clear();
}
