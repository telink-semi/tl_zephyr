/* app_24g.c - Application main entry point */

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

#include "app_24g.h"
#include "app_public.h"

#define LOG_LEVEL LOG_LEVEL_DBG
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(app_24g);


#include "stack/multicore_comm/service/mcc.h"
#include "stack/multicore_comm/service/service_d25f.h"

extern pl_fifo_t d25fKbTxFifo;

#define WDT_INTV_MS     (300)    

#define APP_SLEEP_PREPARE_COUNT             (4)
#define APP_SUSPEND_LIMIT_MIN_TIME_US       (1500)
#define APP_SUSPEND_LONG_TIME_MIN_US        (8000)
#define APP_DELAY_ENTER_SUSPEND_TIME_US     (0)
#define APP_WAKEUP_EARLY_TIME_US            (50)
#define APP_WFI_WAKEUP_EARLY_TIME_US        (20)


extern volatile bool n22_rssi_scan_done;

volatile p24g_device_status_e g_state = STATE_POWERON;
static volatile p24g_device_status_e g_work_state = STATE_NONE;
static volatile p24g_device_status_e g_power_state = STATE_NONE;
static volatile uint32_t g_wakeup_stick = 0;
static volatile uint32_t spp_tick = 0;
static uint8_t g_report_rate = REPORT_RATE_8K;

volatile p24g_device_status_e last_connect_status = STATE_NONE;
volatile unsigned int tick_status;
app_ctx_t app_ctx;



//ZH_TODO
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
 * @brief share memory message handler table
 */
static p24g_sm_cmd_handler_t p24g_cmd_table[P24G_SM_CMD_MAX] = {0};
uint8_t g_app_suspend_ret_flag = 0;

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
static inline void app_timer1_start(uint32_t sys_tick)
{
    unsigned int tick = sys_tick * sys_clk.pclk / SYSTEM_TIMER_TICK_1US;
    timer_set_init_tick(TIMER1, 0);
    timer_set_cap_tick(TIMER1, tick);
    timer_start(TIMER1);
}
static inline void app_timer1_stop(void)
{
    timer_stop(TIMER1);
}
static inline void app_timer1_init(void)
{
    timer_set_init_tick(TIMER1, 0);

    timer_set_mode(TIMER1, TIMER_MODE_SYSCLK);

    timer_set_irq_mask(FLD_TMR1_MODE_IRQ);
    tlkapi_printf(APP_LOG_EN, "timer1 init\r\n");

#if defined(MCU_CORE_TL322X_N22)
    clic_interrupt_enable(IRQ_TIMER1);
#else
    plic_interrupt_enable(IRQ_TIMER1);
#endif
}
_attribute_ram_code_sec_ static void app_2p4g_set_power_state(p24g_device_status_e state, uint32_t tick)
{
    // if ((state == STATE_TO_IDLE) && (g_power_state != STATE_TO_SLEEP)) {
    //     g_power_state = STATE_TO_IDLE;
    //     if (tick) {
    //         app_timer1_start((tick - stimer_get_tick() - APP_WFI_WAKEUP_EARLY_TIME_US * SYSTEM_TIMER_TICK_1US));
    //     }
    // } else if (state == STATE_TO_SLEEP) {
    //     g_wakeup_stick = tick;
    //     // app_inf.step_2t2r = STEP_2T2R_NONE;
    //     g_power_state = STATE_TO_SLEEP;
    // }
}

static inline bool app_2p4g_is_work_busy(void)
{
    return (g_work_state == STATE_BUSY);
}


static inline char *tl_hex_to_str(const void *buf, uint8_t len)
{
    static const char hex[] = "0123456789abcdef";
    static char       str[301];
    const uint8_t    *b = buf;
    uint8_t                i;

    len = min(len, (sizeof(str) - 1) / 3);

    for (i = 0; i < len; i++) {
        str[i * 3]     = hex[b[i] >> 4];
        str[i * 3 + 1] = hex[b[i] & 0xf];
        str[i * 3 + 2] = ' ';
    }

    str[i * 3] = '\0';

    return str;
}

static _attribute_ram_code_sec_ uint16_t app_2p4g_d25f_rx_packet_parse(uint8_t *data, uint16_t len)
{
    p24g_evt_t *p_evt = (p24g_evt_t *) data;

    if (p_evt->type == P24G_MB_CMD_USB) {
        return sizeof(p24g_evt_t) + p_evt->len;
    }

    return len;
}

static _attribute_ram_code_sec_ void app_2p4g_packet_enqueue(uint8_t *data, uint16_t len)
{
    // print_app_public("event receive: %s", tl_hex_to_str(data, len));

    p24g_evt_t *p_evt = (p24g_evt_t *) data;

    if (p_evt->type == P24G_MB_CMD_USB) {
        switch (p_evt->opcode) {
            case P24G_USB_OP_EP_WRITE:
                p24g_usb_pkt_t *p_pkt = (p24g_usb_pkt_t *)p_evt->data;
                usb0hw_write_ep_data((p_pkt->ep_addr & (~USB0_DIR_IN_MASK)), p_pkt->data, p_pkt->len);
                // usbd_ep_write(0, p_pkt->ep_addr, p_pkt->data, p_pkt->len);
                break;

            case P24G_USB_OP_EP_READ:
                break;
            default:
                break;
        }
    }
}

_attribute_ram_code_sec_ void app_2p4g_d25f_rx_packet_handler(uint8_t *data, unsigned int len)
{
    while (len != 0) {
        uint16_t parsed_len = app_2p4g_d25f_rx_packet_parse(data, len);
        app_2p4g_packet_enqueue(data, parsed_len);
        len -= parsed_len;
        data += parsed_len;
    }
}

_attribute_ram_code_sec_ void event_test(void)
{
     static unsigned int tick=0;
     static uint8_t TxBuf[64] = {0}; //[!!important]
     static uint8_t cnt = 0;

     if(clock_time_exceed(tick, 2000000)) {
         tick=stimer_get_tick();

         p24g_evt_t * p_evt = (p24g_evt_t *)TxBuf;

         p_evt->type = P24G_MB_CMD_USB;
         p_evt->opcode = P24G_USB_OP_EP_WRITE;

         p24g_usb_pkt_t *p_pkt = (p24g_usb_pkt_t *)p_evt->data;

         p_pkt->ep_addr = 0x02;
         p_pkt->len = 8;

         p_pkt->data[0] = cnt++;
         p_pkt->data[1] = 0x01;
         p_pkt->data[2] = 0x01;
         p_pkt->data[3] = 0x02;
         p_pkt->data[4] = 0x02;
         p_pkt->data[5] = 0x03;
         p_pkt->data[6] = 0x03;
         p_pkt->data[7] = 0x04;

         p_evt->len = sizeof(p24g_usb_pkt_t) + p_pkt->len;

         if(!mcc_d25f_hci_send_msg(TxBuf, sizeof(p24g_evt_t) + p_evt->len)) {
//         if(!tlk_n22_sync_send_message(TLK_SHARE_MEMORY_MESSAGE_TYPE_BLE, TxBuf, sizeof(p24g_evt_t) + p_evt->len)) {
             tlkapi_printf(APP_LOG_EN, "d25f send sm message success\n");
         } else {
             tlkapi_printf(APP_LOG_EN, "d25f send sm message fail\n");
         }
     }
}


_attribute_ram_code_sec_ uint8_t p24g_send_sm_msg(uint8_t type, uint8_t op, uint8_t *data, uint8_t len)
{
    static uint8_t TxBuf[64] = {0}; //[!!important]

    p24g_evt_t *p_evt = (p24g_evt_t *)TxBuf;

    p_evt->type = type;
    p_evt->opcode = op;
    uint8_t data_len = len > (sizeof(TxBuf) - 2) ? sizeof(TxBuf) - 2 : len;

    if(data && data_len)
    {
        memcpy(p_evt->data, data, data_len);
        p_evt->len = data_len;
    }
    //TODO
    // 	gpio_function_en(GPIO_PB4);
	// gpio_output_en(GPIO_PB4);
	// gpio_input_dis(GPIO_PB4);
	// DBG_GPIO_TOGGLE(APP_IO_EN, GPIO_PB4);
	// delay_us(27);
	// DBG_GPIO_TOGGLE(APP_IO_EN, GPIO_PB4);
    
    if (!mcc_d25f_shm_send_msg(TxBuf, sizeof(p24g_evt_t) + p_evt->len, TLK_SHM_MSG_2P4G))
    {
        //if(!tlk_n22_sync_send_message(TLK_SHARE_MEMORY_MESSAGE_TYPE_BLE, TxBuf, sizeof(p24g_evt_t) + p_evt->len)) {
        tlkapi_printf(1, "d25f send sm message success\n");
        //tlkapi_printk(TLK_LOG_EN, "d25f send sm message success %x %x\n", type, op);
    }
    else
    {
        tlkapi_printf(1, "d25f send sm message fail\n");
        //tlkapi_printk(TLK_LOG_EN, "d25f send sm message faillll\n");
        return 1; // Error: message sending failed
    }
    // DBG_GPIO_TOGGLE(APP_IO_EN, GPIO_PA6);
    return TLK_SUCCESS; // Success
}

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


_attribute_ram_code_ void app_2p4g_mb_km_data_cb(uint8_t* data)
{
    
    if(data[0] == 0x7a)
    {
        data[0] = 0;
        // app_mouse_report_to_usb(&data[1]);
    }
    else if(data[6] == 0xa7 && data[5] == 0x57 && data[4] == 0x5a)
    {
        // tlkapi_send_string_data(APP_LOG_EN, "now usb init",data,7);
      //  n22_rssi_scan_done = true;
    }else{
        // tlkapi_send_string_data(APP_LOG_EN, "d25f kb app_2p4g_mb_km_data_cb",data,7);
    }
    // tlkapi_send_string_data(APP_LOG_EN, "d25f kb app_2p4g_mb_km_data_cb",data,7);
    //  tlkapi_printk(TLK_LOG_EN, "d25f kb app_2p4g_mb_km_data_cb: %x %x %x %x\n", data[0], data[1], data[2], data[3]);
    
}


_attribute_ram_code_sec_ void app_2p4g_d25f_sm_rx_cb(uint8_t *data, uint32_t len)
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
        tlkapi_send_string_data(APP_LOG_EN, "error! unknow sm command", data, len);
    }

#if 0
        tlkapi_printf(APP_LOG_EN, "sm rx %x %x %x %x %x %x %x %x\r\n",
                      p_evt->type, p_evt->opcode, p_evt->len,
                      p_evt->data[0], p_evt->data[1], p_evt->data[2], p_evt->data[3]);
        tlkapi_send_string_data(APP_LOG_EN, "d25f sm rx cb", data, len);
#endif
}



_attribute_ram_code_sec_ uint8_t p24g_send_spp_data(uint8_t cmd, unsigned char *data, unsigned char len)
{
    uint8_t ret = TLK_ERR_INVALID_LENGTH;

    if (len < 17) {
        ret = pp_fifo_push(&d25fSppTxFifo, cmd, data, len);
    }

    return ret;
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
        tlkapi_printf(APP_LOG_EN, "p24g_register_sm_cmd_handler error %x\n", cmd);
    }

    return ret;
}


/**
 * @brief       user initialization when MCU wake_up from deepSleep_retention mode
 * @param[in]   none
 * @return      none
 */
_attribute_ram_code_ void p24g_user_init_deepRetn(void)
{
#if (PM_DEEPSLEEP_RETENTION_ENABLE)
   

    /***
     * TL321X(buteo):
     *   PLL_192M_CCLK_96M_HCLK_48M_PCLK_24M_MSPI_48M, the time from retention_reset(.s file) to here is 550us.
     *   PLL_192M_CCLK_32M_HCLK_32M_PCLK_32M_MSPI_48M, the time from retention_reset(.s file) to here is 578us
     */
    DBG_CHN0_HIGH;
    irq_enable();


    #if (BATT_CHECK_ENABLE)
    adc_hw_initialized = 0;
    #endif

    #if (TLKAPI_DEBUG_ENABLE)
    tlkapi_debug_deepRetn_init();
    #endif
#endif
}

void app_2p4g_dual_core_comm_init(void)
{

    mcc_mb_register_cb(TLK_MB_N22_TO_D25F_KM_DATA, app_2p4g_mb_km_data_cb);
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


_attribute_ram_code_sec_ static void app_2p4g_handle_save_pairing_info(uint8_t *data, uint16_t len)
{
    p24g_evt_t *p_evt = (p24g_evt_t *)data;

    if (p_evt->type == P24G_SM_CMD_SAVE_PAIR_INFO)
    {
        memcpy(flash_dev_info.peer_addr, p_evt->data, MAC_ADDR_LEN);
        uint32_t side_id = fnv1a_hash(flash_dev_info.peer_addr, MAC_ADDR_LEN);
        
        if (flash_dev_info.side_id != side_id)
        {
            flash_dev_info.side_id = side_id;
            //save_data_to_flash(flash_sector_2p4_inf, sizeof(ST_FLASH_DEV_INFO), (unsigned char *)&flash_dev_info.side_id, (int *)&dev_info_idx);

            int ret = nvs_write(&user_fs, APP_2P4G_PAIR_INFO_ID, (unsigned char *)&flash_dev_info.side_id, sizeof(ST_FLASH_DEV_INFO));
            printk("NVS APP_2P4G_PAIR_INFO_ID Write result: %d\n", ret);
        }
        
    }
}

_attribute_ram_code_sec_ static void app_2p4g_save_report_rate_info(uint8_t rr)
{
    if ((rr == REPORT_RATE_8K) || (rr == REPORT_RATE_125)) {
        flash_dev_other_info.report_rate = rr;
        uint32_t side_id = fnv1a_hash(flash_dev_other_info.report_rate, 1);

        if (flash_dev_other_info.side_id != side_id)
        {
            flash_dev_other_info.side_id = side_id;
            tlkapi_send_string_data(APP_LOG_EN, "saving other info", &flash_dev_other_info.side_id, 5);

            //save_data_to_flash(flash_sector_2p4_other_inf, sizeof(ST_FLASH_DEV_OTHER_INFO), (unsigned char *)&flash_dev_other_info.side_id, (int *)&dev_other_info_idx);
            int ret = nvs_write(&user_fs, APP_2P4G_APP_INFO_ID, (unsigned char *)&flash_dev_other_info.side_id, sizeof(ST_FLASH_DEV_OTHER_INFO));
            printk("NVS APP_2P4G_APP_INFO_ID Write result: %d\n", ret);
        }
    }
}

_attribute_ram_code_sec_ static void app_2p4g_handle_set_state(uint8_t *data, uint16_t len)
{
    p24g_evt_t *p_evt = (p24g_evt_t *)data;
    if (p_evt->type == P24G_SM_CMD_SET_STATE)
    {
        if (p_evt->opcode <= STATE_PAIRING_TIMEOUT) {
            app_d24p_set_state(p_evt->opcode);
        }

        if (p_evt->opcode == STATE_CONNECTED)
        {
            tlkapi_send_string_data(APP_LOG_EN, "connected", data, len);

            spp_tick = stimer_get_tick() | 1;
        }
        else if (p_evt->opcode == STATE_DISCONNECTED)
        {
             if(p_evt->data[0] == P24G_LL_CONN_TIMEOUT)
            {
                p24g_enable_reconn(true);
            }
        }
        else if (p_evt->opcode == STATE_PAIRING)
        {

        }else if (p_evt->opcode == STATE_RF_IDLE)
        {

        }else if (p_evt->opcode == STATE_PAIRING_TIMEOUT)
        {
            p24g_enable_pairing(true);
        }
        else if (p_evt->opcode == STATE_IDLE)
        {
            uint32_t wakeup_stick = 0;

            wakeup_stick = p_evt->data[3];
            wakeup_stick = (wakeup_stick << 8) + p_evt->data[2];
            wakeup_stick = (wakeup_stick << 8) + p_evt->data[1];
            wakeup_stick = (wakeup_stick << 8) + p_evt->data[0];

            app_2p4g_set_power_state(STATE_TO_IDLE, wakeup_stick);
        }
        else if (p_evt->opcode == STATE_SLEEP)
        {
            uint32_t wakeup_stick = 0;

            wakeup_stick = p_evt->data[3];
            wakeup_stick = (wakeup_stick << 8) + p_evt->data[2];
            wakeup_stick = (wakeup_stick << 8) + p_evt->data[1];
            wakeup_stick = (wakeup_stick << 8) + p_evt->data[0];

            // DBG_GPIO_TOGGLE(STACK_IO_EN, GPIO_PD7);
            app_2p4g_set_power_state(STATE_TO_SLEEP, wakeup_stick);
        }
    }
}

static inline void app_2p4g_clock_reinit(uint8_t report_rate)
{
    g_report_rate = report_rate;
    if (report_rate == REPORT_RATE_8K) {
        app_clock_init(CLOCK_CONFIG_1V1_96_96);
        if (app_get_kb_mode() == KB_MODE_2P4G) {
            app_wdt_init();
        }
    } else {
        app_clock_init(CLOCK_CONFIG_1V_48_24);
        if (app_get_kb_mode() == KB_MODE_2P4G) {
            app_wdt_init();
        }
    }
}

_attribute_ram_code_sec_ void app_2p4g_clock_reover(void)
{
    if (g_report_rate == REPORT_RATE_8K) {
        app_clock_init(CLOCK_CONFIG_1V1_96_96);
        if (app_get_kb_mode() == KB_MODE_2P4G) {
            app_wdt_init();
        }
    } else {
        app_clock_init(CLOCK_CONFIG_1V_48_24);
        if (app_get_kb_mode() == KB_MODE_2P4G) {
            app_wdt_init();
        }
    }
}

_attribute_ram_code_sec_ static void app_2p4g_handle_spp_data(uint8_t *data, uint16_t len)
{
    p24g_evt_t *p_evt = (p24g_evt_t *)data;
    if (p_evt->opcode == P24G_SPP_LED_STATUS)
    {
        app_pc_kb_led_status(p_evt->data[0]);

    }
    else if (p_evt->opcode == P24G_SPP_TEST_DATA)
    {
        tlkapi_send_string_data(APP_LOG_EN, "rx spp data", data, len);

    }
}

_attribute_ram_code_sec_ static void app_2p4g_handle_misc(uint8_t *data, uint16_t len)
{
    p24g_evt_t *p_evt = (p24g_evt_t *)data;
    switch (p_evt->opcode) {
        case P24G_SM_OP_MISC_REPORT_RATE: //report rate changed
            tlkapi_send_string_data(APP_LOG_EN, "report rate changed", data, len);
            app_2p4g_clock_reinit(data[3]);
            break;

        case P24G_SM_OP_MISC_SAVE_REPORT_RATE:
            app_2p4g_save_report_rate_info(data[3]);
            // DBG_GPIO_TOGGLE(APP_IO_EN, GPIO_PH0);
            tlkapi_send_string_data(APP_LOG_EN, "report rate info saved", data, len);
            app_2p4g_clock_reinit(data[3]);
            break;

        case P24G_SM_OP_MISC_RF_MODE:
            tlkapi_printf(APP_LOG_EN, "rf mode:%x\n", p_evt->data[0]);
            app_ctx.rf_mode = p_evt->data[0];
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

    #if (HW_BOARD_TYPE == HW_EVK_KEYBOARD || HW_EVK_BOARD == 1)
    else{
        tlkapi_send_string_data(APP_LOG_EN, "enter pairing mode ", 0, 0);
        p24g_enable_pairing(true);
    }
    #endif
    if (dev_other_info_idx >= 0) {
        p24g_send_sm_msg(P24G_SM_CMD_MISC, P24G_SM_OP_MISC_REPORT_RATE, &flash_dev_other_info.report_rate, 1);
    }
}


static void app_debug_io_init(void)
{
#if (ALG_KEYSCAN_APP_FUN_ENABLE && APP_IO_EN)
    gpio_function_en(GPIO_PD5 | GPIO_PD6 | GPIO_PD7);
    gpio_output_en(GPIO_PD5 | GPIO_PD6 | GPIO_PD7);
    gpio_input_dis(GPIO_PD5 | GPIO_PD6 | GPIO_PD7);
#endif
}

/**
 * @brief       user initialization when MCU power on or wake_up from deepSleep mode
 * @param[in]   none
 * @return      none
 */
void p24g_user_init_normal(void)
{
    app_wdt_init();

    // app_debug_io_init();

    app_2p4g_dual_core_comm_init();

    app_p24g_sm_cmd_hanlder_init();

    app_p24g_send_info_2_n22();//call this fun after sm init

    p24g_send_sm_msg(P24G_SM_CMD_SET_KB_MODE, P24G_KB_MODE_2P4G, 0, 0);
 
    app_timer1_init();
    tlkapi_send_string_data(APP_LOG_EN, "d25f kb _p24g_init end", 0, 0);
}


_attribute_ram_code_sec_ void app_p24g_spp_send_handle(void)
{
    unsigned char  *p= pp_fifo_get_ptr(&tx_fifo);
    
    if(p!=0)
    {
        unsigned char len=p[0];
        unsigned char cmd=p[1];
        
        unsigned char ret=0; 
        if(cmd==CONSUME_KB_DATA_CMD)
        {
             ret = p24g_send_spp_data(P24G_SPP_CONSUME_KEY_DATA, &p[2], len);
        }
        else if(cmd==ALL_KB_DATA_CMD)
        {
            ret = p24g_send_spp_data(P24G_SPP_ALL_KEY_DATA, &p[2], len);
        }
        if(ret==TLK_SUCCESS)
        {
            pp_fifo_pop(&tx_fifo);
            tlkapi_send_string_data(APP_LOG_EN, "spp send OK", &ret, 1);
        }
        else
        {
            tlkapi_send_string_data(APP_LOG_EN, "spp send failed", &ret, 1);
        }
    }
}


/**
 * @brief       in 2.4g mode  device status check
 * @param[in]    none
 * @return      none
 */

_attribute_ram_code_sec_ void app_pp_check_connect_status(void)
{
    static unsigned int led_tick;

    #if 1
    // if(last_connect_status!=app_inf.dev_now_status)
    if(last_connect_status!=app_d24p_get_state() )
    {
        tick_status=clock_time()|1;
        led_tick=clock_time()|1;
        #if 0
        if(last_connect_status==STATE_PAIRING)
        {
            //auto_draw_flag=0;
        }
        #endif
        // last_connect_status=app_inf.dev_now_status;
        last_connect_status = app_d24p_get_state();
        // print_app_public("d24g_status=%d\r\n", last_connect_status);

        #if 0
        if (app_inf.pair_success_flag)
        {
           
            app_inf.pair_success_flag = 0;
            
            if (flash_dev_info.side_id != app_inf.side_id)
            {
                 
                flash_dev_info.side_id = app_inf.side_id;
                DBG_GPIO_TOGGLE(APP_IO_EN, GPIO_PG6);
                save_data_to_flash(flash_sector_2p4_inf, sizeof(ST_FLASH_DEV_INFO), (unsigned char *)&flash_dev_info.side_id, (int *)&dev_info_idx);
                DBG_GPIO_TOGGLE(APP_IO_EN, GPIO_PG6);
            }

        }
        #endif
        
        // if((app_inf.dev_now_status==STATE_NORMAL))
        if((app_d24p_get_state() == STATE_CONNECTED))
        {
            gpio_set_level(PAIR_LED_PIN,LED_IS_ON);
        }
    }
#endif


    if(usb_connected_ok)
    {
        gpio_set_level(PAIR_LED_PIN,LED_IS_OFF);
    }
    // else  if(app_inf.dev_now_status==STATE_RECONNECT)
    else  if(app_d24p_get_state() == STATE_RECONNECT)
    {
        #if 0
        if(clock_time_exceed(tick_status, RECONN_TIMEOUT_US))
        {
        
            //pp_rf_enter_idle(1);
            
            
            if(pp_get_rf_link_status()==IDLE_RF_STATUS)
            {
                //app_enter_sleep(D24G_RECONNECT_TIMEOUT_SLEEP);
            }
            
        }
        #endif

        if(clock_time_exceed(led_tick, 1000000))
        {
            led_tick=clock_time();
            
            DBG_GPIO_TOGGLE(APP_IO_EN, PAIR_LED_PIN);
            
//          tlkapi_printf(APP_LOG_EN, "powron\n");
        }
    }
    else  if(app_d24p_get_state() == STATE_PAIRING)
    {
        if(clock_time_exceed(led_tick, 100000))
        {
            led_tick=clock_time();
            DBG_GPIO_TOGGLE(APP_IO_EN, PAIR_LED_PIN);
        }
    }
    if((app_d24p_get_state() == STATE_CONNECTED))
    {
        app_p24g_spp_send_handle();
    }
}

#define SPP_TEST_EN 0
uint8_t spp_buf[16] = {0x0, 0x13, 0x34, 0x45, 0x56, 0x67, 0x78, 0x89};
_attribute_ram_code_sec_ static void app_spp_send_data(void)
{
    DBG_GPIO_TOGGLE(APP_IO_EN, GPIO_PH0);
    spp_tick = stimer_get_tick() | 1;
    int ret = p24g_send_spp_data(P24G_SPP_TEST_DATA, spp_buf, sizeof(spp_buf));
    if (ret != TLK_SUCCESS)
    {
        tlkapi_send_string_data(APP_LOG_EN, "spp send test data failed", &ret, 1);
    }
    spp_buf[0]++;
}

_attribute_ram_code_sec_ void app_timer1_irq_handler(void)
{
    if (timer_get_irq_status(FLD_TMR1_MODE_IRQ)) {
        app_timer1_stop();
        timer_clr_irq_status(FLD_TMR1_MODE_IRQ); //clear irq status
    }
}

_attribute_ram_code_sec_ static void app_2p4g_sleep_check_loop(void)
{
    static uint8_t sleep_prepare_cnt = APP_SLEEP_PREPARE_COUNT;
    #if APP_DELAY_ENTER_SUSPEND_TIME_US
    static uint32_t stick = 0;
    #endif
    uint8_t short_suspend_flag = 1;

    if (app_2p4g_is_work_busy() || app_is_usb_det_in()) {
        return;
    }

    if ((g_power_state == STATE_TO_SLEEP)) {
        #if APP_DELAY_ENTER_SUSPEND_TIME_US
        if (!stick) {
            stick = stimer_get_tick() | 1;
        }
        #endif
        if (!sleep_prepare_cnt) {
            #if APP_DELAY_ENTER_SUSPEND_TIME_US
            if (stick && tick1_exceed_tick2(stimer_get_tick(), stick + APP_DELAY_ENTER_SUSPEND_TIME_US * SYSTEM_TIMER_TICK_1US)) {
                stick = 0;
            #endif
            g_power_state = STATE_NONE;
            sleep_prepare_cnt = APP_SLEEP_PREPARE_COUNT;
            if (tick1_exceed_tick2(g_wakeup_stick, stimer_get_tick() + APP_SUSPEND_LIMIT_MIN_TIME_US * SYSTEM_TIMER_TICK_1US)) {
                // gpio_set_level(GPIO_PH2, 0);
                // app_proc_before_sleep();
                // DBG_GPIO_TOGGLE(APP_IO_EN,  GPIO_PD7);
                if (tick1_exceed_tick2(g_wakeup_stick, stimer_get_tick() + APP_SUSPEND_LONG_TIME_MIN_US * SYSTEM_TIMER_TICK_1US)) {
                    short_suspend_flag = 0;
                    // gpio_set_level(GPIO_PH2, 1);
                }
                pm_set_suspend_power_cfg(FLD_PD_ZB_EN, short_suspend_flag);

                g_app_suspend_ret_flag = 1;
                // DBG_STACK_GPIO_SET_LEVEL(APP_IO_EN, GPIO_PH2, 0);
                pm_sleep_wakeup(SUSPEND_MODE, PM_WAKEUP_TIMER, PM_TICK_STIMER, g_wakeup_stick
                                - (APP_WAKEUP_EARLY_TIME_US + (short_suspend_flag ? 0 : 600)) * SYSTEM_TIMER_TICK_1US);
                // DBG_STACK_GPIO_SET_LEVEL(APP_IO_EN, GPIO_PH2, 1);

                // DBG_GPIO_TOGGLE(APP_IO_EN,  GPIO_PD7);
                // app_proc_after_sleep();
                if (!short_suspend_flag) {
                    // gpio_set_level(GPIO_PH2, 0);
                    pm_set_dig_module_power_switch(FLD_PD_ZB_EN, PM_POWER_UP);
                    p24g_send_sm_msg(P24G_SM_CMD_MISC, P24G_SM_OP_MISC_LONG_SUSP_RET, 0, 0);
                } else {
                    p24g_send_sm_msg(P24G_SM_CMD_MISC, P24G_SM_OP_MISC_SUSP_RET, 0, 0);
                }
                // gpio_set_level(GPIO_PH2, 1);
                app_2p4g_set_power_state(STATE_TO_IDLE, 0);
            }
            #if APP_DELAY_ENTER_SUSPEND_TIME_US
            }
            #endif
        } else {
            sleep_prepare_cnt--;
        }
    } else if (g_power_state == STATE_TO_IDLE) {
        if (!sleep_prepare_cnt) {
            g_power_state = STATE_NONE;
            sleep_prepare_cnt = APP_SLEEP_PREPARE_COUNT;
            // DBG_STACK_GPIO_SET_LEVEL(APP_IO_EN, GPIO_PH2, 0);
                // gpio_set_level(GPIO_PG7, 0);
            core_entry_wfi_mode();
                // gpio_set_level(GPIO_PG7, 1);
            // DBG_STACK_GPIO_SET_LEVEL(APP_IO_EN, GPIO_PH2, 1);
            app_2p4g_set_power_state(STATE_TO_IDLE, 0);
        } else {
            sleep_prepare_cnt--;
        }
    }
}

/**
 * @brief     BLE main loop
 * @param[in]  none.
 * @return     none.
 */
_attribute_no_inline_ void app_2p4g_main_loop(void)
{
    //tlk_multi_core_communication_loop();
    mcc_d25f_loop();
////////////////////////////////////// Debug entry /////////////////////////////////
#if (TLKAPI_DEBUG_ENABLE)
    tlkapi_debug_handler();
#endif


    app_pp_check_connect_status();
    
    wd_clear();

#if SPP_TEST_EN
    if (spp_tick && clock_time_exceed(spp_tick, 300000))
    {
        app_spp_send_data();
    }
#endif

    app_2p4g_sleep_check_loop();
}
