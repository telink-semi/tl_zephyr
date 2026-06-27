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
#include <zephyr/drivers/uart.h>

#include <zephyr/settings/settings.h>

#include "app_d24g.h"
#include "app_public.h"

#define LOG_LEVEL LOG_LEVEL_DBG
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(app_2p4g);


#include "stack/multicore_comm/service/mcc.h"
#include "stack/multicore_comm/service/service_d25f.h"


#define WDT_INTV_MS     (300)

_attribute_aligned_(4) app_ctx_t app_ctx;
_attribute_aligned_(4) app_dual_core_flag_ctx_t app_dual_core_flag_ctx;

uint8_t last_clock_select = 0xFF;
static volatile uint32_t spp_tick = 0;


/**
 * @brief share memory message handler table
 */
static p24g_sm_cmd_handler_t p24g_cmd_table[P24G_SM_CMD_MAX] = {0};

void mcc_d25f_to_n22_set_clk_info(void);

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

 _attribute_ram_code_sec_ void app_clock_init(app_clock_config_e select)
{
    if (last_clock_select == select) {
        return;
    }
#if ALG_KEYSCAN_APP_FUN_ENABLE
    ks_pwm_mode_disable();
#endif
    last_clock_select = select;
    switch (select) 
    {
        case CLOCK_CONFIG_1V1_192_96:
            pm_set_dig_ldo(DIG_VOL_1V1_MODE, 1000);
            PLL_192M_D25F_96M_HCLK_N22_96M_PCLK_96M_MSPI_48M;
            // k_busy_wait(100);
            mcc_d25f_to_n22_set_clk_info();

            #if ALG_KEYSCAN_APP_FUN_ENABLE
            alg_keyscan_init(KEYSCAN_PWM_CLOCK_96M);
            ks_pwm_mode_disable();
            ks_pwm_mode_enable();
            #endif

            LOG_INF("clock changed to 96M");
            break;

        case CLOCK_CONFIG_1V_192_32:
            PLL_192M_D25F_32M_HCLK_N22_32M_PCLK_32M_MSPI_48M;
            pm_set_dig_ldo(DIG_VOL_1V_MODE, 1000); //1.0
            // k_busy_wait(100);
            mcc_d25f_to_n22_set_clk_info();

            #if ALG_KEYSCAN_APP_FUN_ENABLE
            alg_keyscan_init(KEYSCAN_PWM_CLOCK_32M);
            ks_pwm_mode_disable();
            ks_pwm_mode_enable();
            #endif

            LOG_INF("clock changed to 32M");
            break;

        default:
            break;
    }

	/* Verify configure() - set device configuration using data in cfg */
	int ret = uart_configure(uart_dev, &uart_cfg);
    if (ret != 0) {
        LOG_ERR("Error: failed to configure uart \n");
    }
    LOG_INF("configure uart ok\n");
}

_attribute_ram_code_sec_ void app_2p4g_clock_reinit(report_rate_t report_rate)
{
    if (report_rate == REPORT_RATE_8K || report_rate == REPORT_RATE_4K)
    {
        app_clock_init(CLOCK_CONFIG_1V1_192_96);
        #if APP_WDT_ENABLE
        if (app_get_mode() == KB_MODE_2P4G)
        {
            app_wdt_init();
        }
        #endif
    }
    else
    {
        app_clock_init(CLOCK_CONFIG_1V_192_32);
        #if APP_WDT_ENABLE
        if (app_get_mode() == KB_MODE_2P4G)
        {
            app_wdt_init();
        }
        #endif
    }
}


_attribute_ram_code_sec_ uint8_t p24g_send_sm_msg(uint8_t type, uint8_t op, uint8_t *data, uint8_t len)
{
    static uint8_t TxBuf[64] = {0};

    p24g_evt_t *p_evt = (p24g_evt_t *)TxBuf;

    p_evt->type = type;
    p_evt->opcode = op;
    uint8_t data_len = len > (sizeof(TxBuf) - 2) ? sizeof(TxBuf) - 2 : len;

    if (data && data_len) {
        tmemcpy(p_evt->data, data, data_len);
        p_evt->len = data_len;
    }

    if (!mcc_d25f_shm_send_msg(TxBuf, sizeof(p24g_evt_t) + p_evt->len, TLK_SHM_MSG_2P4G)) {
        // LOG_INF("d25f send sm message success\n");
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

    mcc_d25f_mb_send_data(TLK_MB_D25F_TO_N22_2P4G_KB_TX_ADDRESS, cmd);

    address = (u32)&d25fSppTxFifo;
    cmd[3] = (uint8_t)(address & 0xff);
    cmd[4] = (uint8_t)(address >> 8 & 0xff);
    cmd[5] = (uint8_t)(address >> 16 & 0xff);
    cmd[6] = (uint8_t)(address >> 24 & 0xff);

    mcc_d25f_mb_send_data(TLK_MB_D25F_TO_N22_2P4G_SPP_TX_ADDRESS, cmd);

    address = (u32)&app_dual_core_flag_ctx;
    cmd[3] = (uint8_t)(address & 0xff);
    cmd[4] = (uint8_t)(address >> 8 & 0xff);
    cmd[5] = (uint8_t)(address >> 16 & 0xff);
    cmd[6] = (uint8_t)(address >> 24 & 0xff);

    mcc_d25f_mb_send_data(TLK_MB_D25F_TO_N22_2P4G_APP_CTX_ADDRESS, cmd);

    printk("d25fKbTxFifo: %x\nd25fSppTxFifo: %x\napp_dual_core_flag_ctx: %x\n",&d25fKbTxFifo, &d25fSppTxFifo, &app_dual_core_flag_ctx);
}


_attribute_ram_code_sec_ static void app_2p4g_handle_save_pairing_info(uint8_t *data, uint16_t len)
{
    p24g_evt_t *p_evt = (p24g_evt_t *)data;

    if (p_evt->type == P24G_SM_CMD_SAVE_PAIR_INFO)
    {
        tmemcpy(flash_dev_info.peer_addr, p_evt->data, MAC_ADDR_LEN);
        uint32_t side_id = fnv1a_hash(flash_dev_info.peer_addr, MAC_ADDR_LEN);
        
        if (flash_dev_info.side_id != side_id)
        {
            flash_dev_info.side_id = side_id;
            // save_data_to_flash(flash_sector_2p4_inf, sizeof(ST_FLASH_DEV_INFO), (unsigned char *)&flash_dev_info.side_id, (int *)&dev_info_idx);

            int ret = nvs_write(&user_fs, APP_2P4G_PAIR_INFO_ID, (unsigned char *)&flash_dev_info.side_id, sizeof(ST_FLASH_DEV_INFO));
            LOG_INF("NVS peer_address Write result: %d\n", ret);
        }
        else
        {
            LOG_INF("NVS peer_address is same.");
        }
    }
}

_attribute_ram_code_sec_ static void app_2p4g_save_report_rate_info(uint8_t rr)
{
    if (flash_dev_other_info.report_rate != rr)
    {
        flash_dev_other_info.report_rate = rr;

        int ret = nvs_write(&user_fs, APP_2P4G_APP_INFO_ID, (unsigned char *)&flash_dev_other_info.report_rate, sizeof(ST_FLASH_DEV_OTHER_INFO));
        if(ret)
        {
            LOG_INF("NVS saving report rate success.");
        }
        else
        {
            LOG_INF("NVS saving report rate fail.");
        }
    }
}


_attribute_ram_code_sec_ uint8_t p24g_enable_pairing(bool enable)
{
    return p24g_send_sm_msg(P24G_SM_CMD_PAIRING, enable, 0, 0);
}

_attribute_ram_code_sec_ uint8_t p24g_enable_reconn(bool enable)
{
    return p24g_send_sm_msg(P24G_SM_CMD_SET_STATE, P24G_SM_OP_ENABLE_RECONN, &enable, 1);
}

_attribute_ram_code_sec_ uint8_t p24g_change_report_rate(report_rate_t report_rate)
{
    return p24g_send_sm_msg(P24G_SM_CMD_REPORT_RATE_CHANGE, P24G_SM_OP_NONE, &report_rate, 1);
}

_attribute_ram_code_sec_ uint8_t p24g_rf_enter_idle(void)
{
    return p24g_send_sm_msg(P24G_SM_CMD_LL_CONTROL, P24G_SM_OP_ENTER_RF_IDLE, 0, 0);
}


_attribute_ram_code_sec_ static void app_2p4g_handle_set_state(uint8_t *data, uint16_t len)
{
    p24g_evt_t *p_evt = (p24g_evt_t *)data;

    if (p_evt->type == P24G_SM_CMD_SET_STATE)
    {
        if (p_evt->opcode == TPSLL_EVT_DEV_CONNECTED)
        {
            app_ctx.dev_status = STATE_CONNECTED;
            LOG_INF("connected");
        }
        else if (p_evt->opcode == TPSLL_EVT_DEV_DISCONNECTED)
        {
            app_ctx.dev_status = STATE_DISCONNECTED;
            app_ctx.report_rate = ((uint8_t* )p_evt->data)[0];
            LOG_INF("disconnected");
        }
        else if (p_evt->opcode == TPSLL_EVT_SAVE_REPORT_RATE)
        {
            app_2p4g_save_report_rate_info(((uint8_t* )p_evt->data)[0]);
            LOG_INF("saving RR(0x%02X) ...", ((uint8_t* )p_evt->data)[0]);
        }
        else if (p_evt->opcode == TPSLL_EVT_REPORT_RATE_CHANGED)
        {
            app_ctx.report_rate = ((uint8_t* )p_evt->data)[0];
            LOG_INF("report rate changed to 0x%02X\n", ((uint8_t *)p_evt->data)[0]);
        }
        else if (p_evt->opcode == TPSLL_EVT_SPP_DATA_RECV)
        {
            uint8_t *spp_data = (uint8_t *)p_evt->data;
        }
        else if (p_evt->opcode == TPSLL_EVT_PAIR_TIMEOUT)
        {
            LOG_INF("pair timeout");
        }
        else if (p_evt->opcode == TPSLL_EVT_RECONNECT_TIMEOUT)
        {
            LOG_INF("reconnect timeout");
        }
        else if (p_evt->opcode == TPSLL_EVT_SLEEP_ENTER_REQ)
        {
            LOG_INF("sleep enter req");
        }
        else if (p_evt->opcode == TPSLL_EVT_USR_SPEC_DATA_RECV)
        {
            LOG_INF("recv user data: %d", ((uint8_t *)p_evt->data)[0]);
        }
    }
}


_attribute_ram_code_sec_ static void app_2p4g_handle_spp_data(uint8_t *data, uint16_t len)
{
    p24g_evt_t *p_evt = (p24g_evt_t *)data;
    // LOG_INF("spp type(%d)op(0x%02x)len(%d)d(%d)", p_evt->type, p_evt->opcode, p_evt->len, p_evt->data[0]);
    // printk("spp type(%d)op(0x%02x)len(%d)d(%d)\n", p_evt->type, p_evt->opcode, p_evt->len, p_evt->data[0]);

    if (p_evt->type == P24G_SM_CMD_DATA_TYPE_SPP)
    {
        if (p_evt->opcode == TPSLL_SPP_LED_STATUS)
        {
            // app_pc_kb_led_status(p_evt->data[0]);
        }
        else if (p_evt->opcode == TPSLL_SPP_TEST_DATA)
        {
            static uint16_t last_id = 0xffff;
            uint8_t cur_id = p_evt->data[0];

            if (last_id == 0xffff) {
                last_id = cur_id;
            } else {
                uint8_t expected_id = (uint8_t)(last_id + 1);

                if (cur_id != expected_id) {
                    // printf("expected %u, got %u\n", 
                    // expected_id, cur_id);
                }
                last_id = cur_id;
            }
            #if 0
            for (int i = 1; i < p_evt->len; i++) {
                if (p_evt->data[i] != ((p_evt->data[0] + i) & 0xff)) {

                    printf("err spp_rx(%d)", p_evt->len);
                    for(int j = 0; j < p_evt->len; j++) {
                        printf("%d>", p_evt->data[j]);
                    }
                    printf("\n");
                    break;
                }
            }
            #endif
        }
    }
    else if (p_evt->type == P24G_SM_CMD_SPP_SEND_COMP)
    {
        uint16 pdu_len = p_evt->len - 9;
        fifo_cb_t spp_cb = *(fifo_cb_t *)(&p_evt->data[pdu_len]);
        void *user_arg = *(void **)(&p_evt->data[pdu_len + 4]);

        // printk("spp len(%d)spp_cb:%X\nspp send success:\n", p_evt->len, spp_cb);

        if (spp_cb)
        {
            spp_cb(p_evt->data, pdu_len, true, user_arg);
        }
    }

}



_attribute_ram_code_sec_ static void app_2p4g_handle_misc(uint8_t *data, uint16_t len)
{
    p24g_evt_t *p_evt = (p24g_evt_t *)data;
    switch (p_evt->opcode) {
        case P24G_SM_OP_MISC_REPORT_RATE: //report rate changed
            // app_2p4g_clock_reinit((report_rate_t)p_evt->data[0]);
            LOG_INF("report rate changed %d", p_evt->data[0]);
            break;
        default:
            break;
    }
}


static void app_p24g_sm_cmd_hanlder_init(void)
{
    p24g_register_sm_cmd_handler(P24G_SM_CMD_SAVE_PAIR_INFO,           app_2p4g_handle_save_pairing_info);
    p24g_register_sm_cmd_handler(P24G_SM_CMD_SET_STATE,                app_2p4g_handle_set_state);
    p24g_register_sm_cmd_handler(P24G_SM_CMD_DATA_TYPE_SPP,            app_2p4g_handle_spp_data);
    p24g_register_sm_cmd_handler(P24G_SM_CMD_SPP_SEND_COMP,            app_2p4g_handle_spp_data);
    p24g_register_sm_cmd_handler(P24G_SM_CMD_MISC,                     app_2p4g_handle_misc);
}

static void app_p24g_send_info_2_n22(void)
{
    p24g_send_sm_msg(P24G_SM_CMD_MISC, P24G_SM_OP_MISC_TRANS_MAC, app_ctx.mac, MAC_ADDR_LEN);

    if (dev_info_idx >= 0)
    {
        p24g_send_sm_msg(P24G_SM_CMD_MISC, P24G_SM_OP_MISC_PEER_INFO, flash_dev_info.peer_addr, MAC_ADDR_LEN);
        p24g_enable_reconn(true); 
    }
    else{
        p24g_enable_pairing(true);
    }

    // if (dev_other_info_idx >= 0) {
        p24g_send_sm_msg(P24G_SM_CMD_MISC, P24G_SM_OP_MISC_REPORT_RATE, &flash_dev_other_info.report_rate, 1);
    // }
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

_attribute_ram_code_sec_ uint8_t app_send_spp_data(uint8_t *data, uint8_t len, uint8_t retry_num, spp_cb_t cb, void *user_arg)
{
    uint8_t ret = 0;

    if((app_ctx.dev_status != STATE_CONNECTED) || app_dual_core_flag_ctx.is_stk_busy)
    {
        return TLK_ERR_INVALID_STATE;
    }
    if (!data)
    {
        return TLK_ERR_NULL;
    }
    if (len > KM_SPP_MAX_LEN)
    {
        return TLK_ERR_INVALID_LENGTH;
    }

    ret = pp_fifo_push_extra(&d25fSppTxFifo, TPSLL_SPP_DATA, data, len, retry_num, cb, user_arg);

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

    app_p24g_send_info_2_n22();

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
