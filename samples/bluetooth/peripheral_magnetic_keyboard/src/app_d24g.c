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


static volatile uint32_t spp_tick = 0;


volatile tpsll_dev_status_e g_state = STATE_POWERON;

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
        LOG_INF("d25f send sm message success\n");
    } else {
        LOG_ERR("d25f send sm message fail\n");
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
        LOG_ERR("error! unknow sm command\n");
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
        LOG_ERR("p24g_register_sm_cmd_handler error %x\n", cmd);
    }

    return ret;
}



void app_2p4g_dual_core_comm_init(void)
{
    mcc_mb_register_cb(TLK_MB_N22_TO_D25F_KM_DATA, (mb_recv_cb_t)app_2p4g_mb_km_data_cb);
    mcc_shm_register_cb(TLK_SHM_MSG_2P4G, app_2p4g_d25f_sm_rx_cb);

    uint8_t cmd[7] = {0};
    uint32_t address = (u32)&d25fKbTxFifo;
    cmd[3] = (uint8_t)(address & 0xff);
    cmd[4] = (uint8_t)(address >> 8 & 0xff);
    cmd[5] = (uint8_t)(address >> 16 & 0xff);
    cmd[6] = (uint8_t)(address >> 24 & 0xff);

    printk("d25fKbTxFifo %x\n",&d25fKbTxFifo);
    mcc_d25f_mb_send_data(TLK_MB_D25F_TO_N22_2P4G_KB_TX_ADDRESS, cmd);

    address = (u32)&d25fSppTxFifo;
    cmd[3] = (uint8_t)(address & 0xff);
    cmd[4] = (uint8_t)(address >> 8 & 0xff);
    cmd[5] = (uint8_t)(address >> 16 & 0xff);
    cmd[6] = (uint8_t)(address >> 24 & 0xff);

    mcc_d25f_mb_send_data(TLK_MB_D25F_TO_N22_2P4G_SPP_TX_ADDRESS, cmd);

    pp_fifo_reset(&d25fSppTxFifo);
}


// _attribute_ram_code_sec_ static void app_2p4g_handle_save_pairing_info(uint8_t *data, uint16_t len)
// {
//     p24g_evt_t *p_evt = (p24g_evt_t *)data;

//     if (p_evt->type == P24G_SM_CMD_SAVE_PAIR_INFO)
//     {
//         memcpy(flash_dev_info.peer_addr, p_evt->data, MAC_ADDR_LEN);
//         uint32_t side_id = fnv1a_hash(flash_dev_info.peer_addr, MAC_ADDR_LEN);
        
//         if (flash_dev_info.side_id != side_id)
//         {
//             flash_dev_info.side_id = side_id;
//             save_data_to_flash(flash_sector_2p4_inf, sizeof(ST_FLASH_DEV_INFO), (unsigned char *)&flash_dev_info.side_id, (int *)&dev_info_idx);

//             // int ret = nvs_write(&user_fs, APP_2P4G_PAIR_INFO_ID, (unsigned char *)&flash_dev_info.side_id, sizeof(ST_FLASH_DEV_INFO));
//             // printk("NVS APP_2P4G_PAIR_INFO_ID Write result: %d\n", ret);
//         }
        
//     }
// }

// _attribute_ram_code_sec_ static void app_2p4g_save_report_rate_info(uint8_t rr)
// {
//     if ((rr == REPORT_RATE_8K) || (rr == REPORT_RATE_125)) {
//         flash_dev_other_info.report_rate = rr;
//         uint32_t side_id = fnv1a_hash(flash_dev_other_info.report_rate, 1);

//         if (flash_dev_other_info.side_id != side_id)
//         {
//             flash_dev_other_info.side_id = side_id;
//             tlkapi_send_string_data(APP_LOG_EN, "saving other info", &flash_dev_other_info.side_id, 5);

//             save_data_to_flash(flash_sector_2p4_other_inf, sizeof(ST_FLASH_DEV_OTHER_INFO), (unsigned char *)&flash_dev_other_info.side_id, (int *)&dev_other_info_idx);
//             // int ret = nvs_write(&user_fs, APP_2P4G_APP_INFO_ID, (unsigned char *)&flash_dev_other_info.side_id, sizeof(ST_FLASH_DEV_OTHER_INFO));
//             // printk("NVS APP_2P4G_APP_INFO_ID Write result: %d\n", ret);
//         }
//     }
// }


_attribute_ram_code_sec_ uint8_t p24g_enable_pairing(bool enable)
{
    return p24g_send_sm_msg(P24G_SM_CMD_PAIRING, enable, 0, 0);
}

_attribute_ram_code_sec_ uint8_t p24g_enable_reconn(bool enable)
{
    return p24g_send_sm_msg(P24G_SM_CMD_SET_STATE, P24G_SM_OP_ENABLE_RECONN, &enable, 1);
}

_attribute_ram_code_sec_ uint8_t p24g_terminate_connect(void)
{
    return p24g_send_sm_msg(P24G_SM_CMD_LL_CONTROL, P24G_SM_OP_TERMINATE_CONN, 0, 0);
}

_attribute_ram_code_sec_ uint8_t p24g_rf_enter_idle(void)
{
    return p24g_send_sm_msg(P24G_SM_CMD_LL_CONTROL, P24G_SM_OP_ENTER_RF_IDLE, 0, 0);
}


_attribute_ram_code_sec_ static void app_2p4g_handle_set_state(uint8_t *data, uint16_t len)
{
    p24g_evt_t *p_evt = (p24g_evt_t *)data;
    // if (p_evt->type == P24G_SM_CMD_SET_STATE)
    // {
    //     if (p_evt->opcode <= STATE_PAIRING_TIMEOUT) {
    //         // app_d24p_set_state(p_evt->opcode);
    //     }

    //     if (p_evt->opcode == STATE_CONNECTED)
    //     {
    //         tlkapi_send_string_data(APP_LOG_EN, "connected", data, len);

    //         spp_tick = stimer_get_tick() | 1;
    //     }
    //     else if (p_evt->opcode == STATE_DISCONNECTED)
    //     {
    //          if(p_evt->data[0] == P24G_LL_CONN_TIMEOUT)
    //         {
    //             p24g_enable_reconn(true);
    //         }
    //     }
    //     else if (p_evt->opcode == STATE_PAIRING)
    //     {

    //     }else if (p_evt->opcode == STATE_RF_IDLE)
    //     {

    //     }else if (p_evt->opcode == STATE_PAIRING_TIMEOUT)
    //     {
    //         p24g_enable_pairing(true);
    //     }
    //     else if (p_evt->opcode == STATE_IDLE)
    //     {
    //         uint32_t wakeup_stick = 0;

    //         wakeup_stick = p_evt->data[3];
    //         wakeup_stick = (wakeup_stick << 8) + p_evt->data[2];
    //         wakeup_stick = (wakeup_stick << 8) + p_evt->data[1];
    //         wakeup_stick = (wakeup_stick << 8) + p_evt->data[0];

    //         // app_2p4g_set_power_state(STATE_TO_IDLE, wakeup_stick);
    //     }
    //     else if (p_evt->opcode == STATE_SLEEP)
    //     {
    //         uint32_t wakeup_stick = 0;

    //         wakeup_stick = p_evt->data[3];
    //         wakeup_stick = (wakeup_stick << 8) + p_evt->data[2];
    //         wakeup_stick = (wakeup_stick << 8) + p_evt->data[1];
    //         wakeup_stick = (wakeup_stick << 8) + p_evt->data[0];

    //         // DBG_GPIO_TOGGLE(STACK_IO_EN, GPIO_PD7);
    //         // app_2p4g_set_power_state(STATE_TO_SLEEP, wakeup_stick);
    //     }
    // }
}


_attribute_ram_code_sec_ static void app_2p4g_handle_spp_data(uint8_t *data, uint16_t len)
{
    p24g_evt_t *p_evt = (p24g_evt_t *)data;
    if (p_evt->opcode == TPSLL_SPP_LED_STATUS)
    {
        // app_pc_kb_led_status(p_evt->data[0]);

    }
    else if (p_evt->opcode == TPSLL_SPP_TEST_DATA)
    {
        tlkapi_send_string_data(APP_LOG_EN, "rx spp data", data, len);

    }
}



_attribute_ram_code_sec_ static void app_2p4g_handle_misc(uint8_t *data, uint16_t len)
{
    p24g_evt_t *p_evt = (p24g_evt_t *)data;
    switch (p_evt->opcode) {
        case P24G_SM_OP_MISC_REPORT_RATE:
            LOG_INF("report rate changed\n");
            break;

    default:
        break;
    }
}

static void app_p24g_sm_cmd_hanlder_init(void)
{
    // p24g_register_sm_cmd_handler(P24G_SM_CMD_SAVE_PAIR_INFO,           app_2p4g_handle_save_pairing_info);
    p24g_register_sm_cmd_handler(P24G_SM_CMD_SET_STATE,                app_2p4g_handle_set_state);
    p24g_register_sm_cmd_handler(P24G_SM_CMD_DATA_TYPE_SPP,            app_2p4g_handle_spp_data);
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


_attribute_ram_code_sec_ uint8_t p24g_send_spp_data(uint8_t cmd, unsigned char *data, unsigned char len)
{
    uint8_t ret = TLK_ERR_INVALID_LENGTH;

    if (len < 65) {
        ret = pp_fifo_push(&d25fSppTxFifo, cmd, data, len);
    }

    return ret;
}

void mcc_d25f_to_n22_set_clk_info(void)
{
    uint8_t cmd[8] = {0};
    uint32_t address = (uint32_t)(&sys_clk);

    cmd[0] = address;
    cmd[1] = address >> 8;
    cmd[2] = address >> 16;
    cmd[3] = address >> 24;

    mcc_d25f_mb_send_data(TLK_MB_D25F_TO_N22_SET_CLK_INFO, cmd);

    // delay_us(20);
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

    p24g_send_sm_msg(P24G_SM_CMD_SET_KB_MODE, KB_MODE_2P4G, 0, 0);
 
    LOG_INF("d25f kb_p24g_init end\n");
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
