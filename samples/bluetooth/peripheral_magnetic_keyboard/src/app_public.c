/** @file app_public.c
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
#include <inttypes.h>
#include <zephyr/drivers/interrupt_controller/riscv_plic.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/fs/nvs.h>
#include <zephyr/storage/flash_map.h>
#include <zephyr/sys/reboot.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/bluetooth/conn.h>

#include "timer.c"
#include "app_public.h"
#include "drivers.h"
#include "app_kb_matrix.h"

#define LOG_LEVEL LOG_LEVEL_DBG
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(app_public);

/*
 * Devicetree node identifiers for the buttons and LED this sample
 * supports.
 */
#define MODE_NODE   DT_ALIAS(ledmode)
#define DEVICE_NODE DT_ALIAS(leddevicestatus)
#define CAP_NODE    DT_ALIAS(ledcap)
#define NUM_NODE    DT_ALIAS(lednum)

#define VBUS_NODE   DT_ALIAS(vbuscheck0)
#define D24G_NODE   DT_ALIAS(mode2p4)
#define BLE_NODE    DT_ALIAS(modeble)
/*
 * Helper macro for initializing a gpio_dt_spec from the devicetree
 * with fallback values when the nodes are missing.
 */
#define GPIO_SPEC(node_id) GPIO_DT_SPEC_GET_OR(node_id, gpios, {0})

/*
 * Create gpio_dt_spec structures from the devicetree.
 */
const struct gpio_dt_spec   mode_led_pin = GPIO_SPEC(MODE_NODE),
                            device_status_led_pin = GPIO_SPEC(DEVICE_NODE),
                            cap_led_pin = GPIO_SPEC(CAP_NODE),
                            num_led_pin = GPIO_SPEC(NUM_NODE),
                            vbus_check_pin = GPIO_SPEC(VBUS_NODE),
                            mode_2p4_pin = GPIO_SPEC(D24G_NODE),
                            mode_ble_pin = GPIO_SPEC(BLE_NODE);



/* 定义NVS使用的Flash存储分区 */
#define NVS_USER_PARTITION user_app_partition
#define NVS_PARTITION_DEVICE FIXED_PARTITION_DEVICE(NVS_USER_PARTITION)
#define NVS_PARTITION_OFFSET FIXED_PARTITION_OFFSET(NVS_USER_PARTITION)
/* NVS扇区大小，需与Flash的擦除页大小匹配 */
#define NVS_SECTOR_SIZE (4096)
/* NVS扇区数量 */
#define NVS_SECTOR_COUNT (2)
/* 定义NVS实例 */
struct nvs_fs user_fs;


ST_FLASH_DEV_INFO flash_dev_info  __attribute__ ((aligned (4)));
int dev_info_idx;

ST_FLASH_DEV_OTHER_INFO flash_dev_other_info  __attribute__ ((aligned (4)));
int dev_other_info_idx;

_attribute_data_retention_ uint32_t flash_sector_2p4_inf=P24G_PAIR_INF_FLASH_ADDR_2M;
_attribute_data_retention_ uint32_t flash_sector_2p4_other_inf=P24G_OTHER_INF_FLASH_ADDR_2M;



volatile unsigned char fun_mode = 0;
static unsigned char last_mode_status=APP_WIRED_USB_MODE;

_attribute_aligned_(4)  unsigned char buf_txfifo[40*8];

pl_fifo_t tx_fifo={  
     .size=40,
     .num=8,
     .wptr=0,
     .rptr=0,
     .p=buf_txfifo,
 };


#define KB_TX_FIFO_SIZE 24
#define KB_TX_FIFO_NUM 16

_attribute_aligned_(4)  unsigned char kb_buf_txfifo[KB_TX_FIFO_SIZE * KB_TX_FIFO_NUM];
pl_fifo_t d25fKbTxFifo = {
    .size = KB_TX_FIFO_SIZE,
    .num = KB_TX_FIFO_NUM,
    .wptr = 0,
    .rptr = 0,
    .p = kb_buf_txfifo,
};


#define SPP_TX_FIFO_NUM 8
_attribute_aligned_(4)  unsigned char spp_buf_txfifo[SPP_TX_FIFO_SIZE_KB * SPP_TX_FIFO_NUM];
pl_fifo_t d25fSppTxFifo = {
    .size = SPP_TX_FIFO_SIZE_KB,
    .num = SPP_TX_FIFO_NUM,
    .wptr = 0,
    .rptr = 0,
    .p = spp_buf_txfifo,
};


 /**
 * @brief FNV-1a 32-bit hash function
 * @param data   
 * @param len    
 * @return 16-bit hash value
 */
 _attribute_ram_code_sec_ uint32_t fnv1a_hash(uint8_t *data, size_t len) 
{
    uint32_t hash = 0x811C9DC5;
    
    for (size_t i = 0; i < len; i++) {
        hash ^= data[i];
        hash = hash + (hash << 24) + (hash << 8) + (hash << 5);
        hash ^= (hash >> 16);//Avalanche Effect
    }

    return hash;
}





static void p24g_pairing_info_check(void)
{
    //dev_info_idx = flash_info_load(flash_sector_2p4_inf, (unsigned char *)&flash_dev_info.side_id, sizeof(ST_FLASH_DEV_INFO));

    int ret = nvs_read(&user_fs, APP_2P4G_PAIR_INFO_ID, (unsigned char *)&flash_dev_info.side_id, sizeof(ST_FLASH_DEV_INFO));
    if (ret == -ENOENT) {
        printk("NVS APP_2P4G_PAIR_INFO_ID naver saved\n");
        LOG_INF("not paired: %x %x\n", flash_dev_info.side_id, flash_sector_2p4_inf);
    } else {
        LOG_INF("paired: %x %x\n", flash_dev_info.side_id, flash_sector_2p4_inf);
    }


    //dev_other_info_idx = flash_info_load(flash_sector_2p4_other_inf, (unsigned char *)&flash_dev_other_info.side_id, sizeof(ST_FLASH_DEV_OTHER_INFO));

    ret = nvs_read(&user_fs, APP_2P4G_APP_INFO_ID, (unsigned char *)&flash_dev_other_info.side_id, sizeof(ST_FLASH_DEV_OTHER_INFO));
    if (ret == -ENOENT) {
        printk("NVS APP_2P4G_APP_INFO_ID naver saved\n");

    } else {
        LOG_INF("read flash get report rate: %x\n", flash_dev_other_info.side_id);
    }
}


_attribute_ram_code_sec_ uint8_t app_2p4g_set_stack_report_rate(uint8_t report_rate)
{
    uint8_t ret = TLK_SUCCESS;

    if (app_d24p_get_state() == STATE_CONNECTED) 
    {
        ret = p24g_send_spp_data(P24G_SPP_REPORT_RATE, &report_rate, 1);
        if (TLK_SUCCESS == ret) {
            ret = p24g_send_sm_msg(P24G_SM_CMD_REPORT_RATE_CHANGE, report_rate, 0, 0);
            // DBG_GPIO_TOGGLE(APP_IO_EN, GPIO_PH0);
        }
    } else {
        ret = TLK_ERR_INVALID_STATE;
    }

    return ret;
}



void app_pc_kb_led_status(unsigned char status)
{
    static unsigned char last_status = 0xff;
    if(last_status!=status)
    {
        last_status=status;
        //gpio_set_level(NUM_LED_PIN, !(status&0x01));
        //gpio_set_level(CAP_LED_PIN, !(status&0x02));
    }
}


_attribute_ram_code_sec_ void check_vbus(void)
{
    static int8_t vbus_cnt = 0;

    if(gpio_pin_get_dt(&vbus_check_pin) == 1) {
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


_attribute_ram_code_sec_ void check_mode(u8 power_on)
{
#if (ALLOW_SWITCH_BLE_2P4G_MODE)
    uint8_t mode_2p4_pin_level = gpio_pin_get_dt(&mode_2p4_pin);
    uint8_t mode_ble_pin_level = gpio_pin_get_dt(&mode_ble_pin);

    if(mode_2p4_pin_level == 0 && mode_ble_pin_level == 0)
    {
        fun_mode = APP_WIRED_USB_MODE;
    }
    else if(mode_2p4_pin_level == 1 && mode_ble_pin_level == 0)
    {
        fun_mode = APP_D24G_MODE;
    }
    else if(mode_2p4_pin_level == 0 && mode_ble_pin_level == 1)
    {
        fun_mode = APP_BLE_MODE;
    }
    if(last_mode_status!=fun_mode)
    {
        pp_fifo_reset(&tx_fifo);
        pp_fifo_reset(&d25fKbTxFifo);
        last_mode_status=fun_mode;
        LOG_INF("switch=%d", last_mode_status);
        if(power_on==0)
        {
            sys_reboot(SYS_REBOOT_COLD);
        }
    }
#else
    fun_mode=APP_D24G_MODE;
    //fun_mode=APP_BLE_MODE;
    //fun_mode=APP_WIRED_USB_MODE;
#endif
}

/* Timer0 interrupt handler */
_attribute_ram_code_sec_ void timer0_isr(void)
{
    if (timer_get_irq_status(FLD_TMR0_MODE_IRQ)){
		timer_clr_irq_status(FLD_TMR0_MODE_IRQ); //Clear IRQ status
    #if  ALG_KEYSCAN_APP_FUN_ENABLE
        key_scan();
    #endif
	}
}

void user_timer_init(void)
{
     /* Timer0 configuration */
    timer_set_init_tick(TIMER0, 0);
    timer_set_cap_tick(TIMER0, 125 * sys_clk.pclk * 1);	//125us
    timer_set_mode(TIMER0, TIMER_MODE_SYSCLK);
    timer_set_irq_mask(FLD_TMR0_MODE_IRQ);
    IRQ_CONNECT(CONFIG_2ND_LVL_ISR_TBL_OFFSET + IRQ_TIMER0, 2, timer0_isr, 0, 0);
    riscv_plic_set_priority(IRQ_TIMER0, 3);
    riscv_plic_irq_enable(IRQ_TIMER0);

     /* Start timers */
	timer_start(TIMER0);
}

static int peripheral_comm_init(void)
{
    int ret;

    if (!gpio_is_ready_dt(&mode_led_pin)) {
        LOG_ERR("mode_led_pin gpio not ready");
        return -ENODEV;
    }
    ret = gpio_pin_configure_dt(&mode_led_pin, GPIO_OUTPUT);
    if (ret != 0) {
        LOG_ERR("Error: failed to configure mode_led_pin io \n");
    }
    LOG_INF("configure mode_led_pin io ok\n");


    if (!gpio_is_ready_dt(&device_status_led_pin)) {
        LOG_ERR("device_status_led_pin gpio not ready");
        return -ENODEV;
    }
    ret = gpio_pin_configure_dt(&device_status_led_pin, GPIO_OUTPUT);
    if (ret != 0) {
        LOG_ERR("Error: failed to configure device_status_led_pin io \n");
    }
    LOG_INF("configure device_status_led_pin io ok\n");


    if (!gpio_is_ready_dt(&cap_led_pin)) {
        LOG_ERR("cap_led_pin gpio not ready");
        return -ENODEV;
    }
    ret = gpio_pin_configure_dt(&cap_led_pin, GPIO_OUTPUT);
    if (ret != 0) {
        LOG_ERR("Error: failed to configure cap_led_pin io \n");
    }
    LOG_INF("configure cap_led_pin io ok\n");


    if (!gpio_is_ready_dt(&num_led_pin)) {
        LOG_ERR("num_led_pin gpio not ready");
        return -ENODEV;
    }
    ret = gpio_pin_configure_dt(&num_led_pin, GPIO_OUTPUT);
    if (ret != 0) {
        LOG_ERR("Error: failed to configure num_led_pin io \n");
    }
    LOG_INF("configure num_led_pin io ok\n");


    gpio_pin_set_dt(&mode_led_pin, 0);
    gpio_pin_set_dt(&device_status_led_pin, 0);
    gpio_pin_set_dt(&cap_led_pin, 0);
    gpio_pin_set_dt(&num_led_pin, 0);


    if (!gpio_is_ready_dt(&mode_2p4_pin)) {
        LOG_ERR("mode_2p4_pin gpio not ready");
        return -ENODEV;
    }
    ret = gpio_pin_configure_dt(&mode_2p4_pin, GPIO_INPUT);
    if (ret != 0) {
        LOG_ERR("Error: failed to configure mode_2p4_pin io \n");
    }
    LOG_INF("configure mode_2p4_pin io ok\n");    


    if (!gpio_is_ready_dt(&mode_ble_pin)) {
        LOG_ERR("mode_ble_pin gpio not ready");
        return -ENODEV;
    }
    ret = gpio_pin_configure_dt(&mode_ble_pin, GPIO_INPUT);
    if (ret != 0) {
        LOG_ERR("Error: failed to configure mode_ble_pin io \n");
    }
    LOG_INF("configure mode_ble_pin io ok\n");    


    if (!gpio_is_ready_dt(&vbus_check_pin)) {
        LOG_ERR("Vbus check gpio not ready");
        return -ENODEV;
    }
    ret = gpio_pin_configure_dt(&vbus_check_pin, GPIO_INPUT);
    if (ret != 0) {
        LOG_ERR("Error: failed to configure vbus check io \n");
    }
    LOG_INF("configure vbus_check_pin io ok\n");
}


void keyboard_comm_init(void)
{
    peripheral_comm_init();

    check_mode(1);

    tlk_d25f_to_n22_mode_info(fun_mode);

    if (fun_mode == APP_D24G_MODE)
    {
	    p24g_user_init_normal();
    } 
    else if (fun_mode == APP_BLE_MODE)
    {
        // ble_init();
    }
    else
    {
        #if USB_APP_FUN_ENABLE
	    /*usb init*/
	    usb_hw_init();
	    #endif
    }

#if ALG_KEYSCAN_APP_FUN_ENABLE
    alg_keyscan_init(KEYSCAN_PWM_CLOCK_96M);
    user_timer_init();
#endif

    pp_fifo_reset(&tx_fifo);
    pp_fifo_reset(&d25fKbTxFifo);

    return 0;
}

// static unsigned char nk_buffer[8]={23, 0}; 
// static unsigned char zero_buffer[8]={0}; 

 _attribute_ram_code_sec_ void public_loop(void)
{
    static uint32_t last_time = 0;

    if(k_uptime_get_32() - last_time > 50)//50ms
    {
        last_time = k_uptime_get_32();
        check_mode(0);
    }

    // static unsigned char flag = 0;

    // flag = 1 - flag;
    // if (flag)
    // {
    //     tpsll_send_keyboard_data(&nk_buffer[0], 8, NORMAL_KB_DATA_CMD, NULL, NULL);
    // }
    // else
    // {
    //     tpsll_send_keyboard_data(&zero_buffer[0], 8, NORMAL_KB_DATA_CMD, NULL, NULL);
    // }

#if USB_APP_FUN_ENABLE
    app_usb_status_check();
#endif

    if (fun_mode == APP_D24G_MODE)
    {
        app_2p4g_main_loop();
    }
    else if (fun_mode == APP_BLE_MODE)
    {
        //TODO
    }
    else if (fun_mode == APP_WIRED_USB_MODE)
    {
#if USB_APP_FUN_ENABLE
        app_usb_main_loop();
#endif
    }
}

_attribute_ram_code_sec_ unsigned char special_key_press_flag_set(unsigned char key_code)
{
     unsigned char real_key_code=key_code;

     if(key_code==T_FN)
     {
        app_key_buf.special_key_press_f|=PRESS_T_FN_FLAG;
        fn_flag=1;
     }
     else if(fn_flag)
     {
        real_key_code=0;
        switch (key_code)
        {
            case KB_M:
                app_key_buf.special_key_press_f|=PRESS_KB_M_FLAG;
                break;
            case KB_P:
                app_key_buf.special_key_press_f|=PRESS_KB_P_FLAG;
                break;
            case KB_F1:
                real_key_code=C_mute;
                break;
             case KB_R:
                app_key_buf.special_key_press_f|=PRESS_KB_R_FLAG;
                break;
              case KB_1:
                app_key_buf.special_key_press_f|=PRESS_KB_1_FLAG;
                break;
              case KB_2:
                app_key_buf.special_key_press_f|=PRESS_KB_2_FLAG;
                break;
              case KB_3:
                app_key_buf.special_key_press_f|=PRESS_KB_3_FLAG;
                break;
              case KB_4:
                app_key_buf.special_key_press_f|=PRESS_KB_4_FLAG;
                break;
              case KB_5:
                app_key_buf.special_key_press_f|=PRESS_KB_5_FLAG;
                break;
              case KB_6:
                app_key_buf.special_key_press_f|=PRESS_KB_6_FLAG;
                break;
              case KB_7:
                app_key_buf.special_key_press_f|=PRESS_KB_7_FLAG;
                break;
            default:
                break;
            
        }
     }
    
    return real_key_code;
}

_attribute_ram_code_sec_ void special_key_event_handle(void)
{
     if(app_key_buf.special_key_press_f==0)
     {
        fn_flag=0;
        return;
     }

    switch (app_key_buf.special_key_press_f)
    {
        case (PRESS_T_FN_FLAG|PRESS_KB_M_FLAG):
            // printf("mouse auto test\n");
            // auto_test_mouse^=0x01;
            // if(usb_connected_ok==0)
            // {
            //     if(fun_mode==APP_D24G_MODE)
            //     {
            //         if(auto_test_mouse)
            //         {
            //             app_rf_set_rx_wait(1);// rx wait
            //         }
            //         else
            //         {
            //             app_rf_set_rx_wait(14);// rx wait
            //         }
            //     }
            // }
            break;
        case (PRESS_T_FN_FLAG|PRESS_KB_P_FLAG):
            // printk("pairing mode\n");
            if(usb_connected_ok==0)
            {
                if(fun_mode==APP_D24G_MODE)
                {
                    LOG_INF("pairing\n");
                    // tpsll_enable_pairing(true);
                }
                else
                {
                    // app_ble_enable_pairing(1);
                }
            }
            break;
        case (PRESS_T_FN_FLAG|PRESS_KB_R_FLAG):
            if(usb_connected_ok)
            {
                // app_usb_report_change();
            }
            break;
        case (PRESS_T_FN_FLAG|PRESS_KB_1_FLAG):
            if(fun_mode==APP_BLE_MODE)
            {
                // app_ble_change_pipe(0);
            }
            else if (fun_mode==APP_D24G_MODE)
            {
                // tpsll_change_report_rate(REPORT_RATE_8K);
            }
            break;
        case (PRESS_T_FN_FLAG|PRESS_KB_2_FLAG):
            if(fun_mode==APP_BLE_MODE)
            {
                // app_ble_change_pipe(1);
            }
            else if (fun_mode==APP_D24G_MODE)
            {
                // tpsll_change_report_rate(REPORT_RATE_4K);
            }
            break;
        case (PRESS_T_FN_FLAG|PRESS_KB_3_FLAG):
            if(fun_mode==APP_BLE_MODE)
            {
                // app_ble_change_pipe(2);
            }
            else if (fun_mode==APP_D24G_MODE)
            {
                // tpsll_change_report_rate(REPORT_RATE_2K);
            }
            break;
        case (PRESS_T_FN_FLAG|PRESS_KB_4_FLAG):
            if(fun_mode==APP_BLE_MODE)
            {
                // app_ble_change_pipe(3);
            }
            else if (fun_mode==APP_D24G_MODE)
            {
                // tpsll_change_report_rate(REPORT_RATE_1K);
            }
            break;
        case (PRESS_T_FN_FLAG|PRESS_KB_5_FLAG):
            if (fun_mode==APP_D24G_MODE)
            {
                // tpsll_change_report_rate(REPORT_RATE_500);
            }
            break;
        case (PRESS_T_FN_FLAG|PRESS_KB_6_FLAG):
            if (fun_mode==APP_D24G_MODE)
            {
                // tpsll_change_report_rate(REPORT_RATE_250);
            }
            break;
        case (PRESS_T_FN_FLAG|PRESS_KB_7_FLAG):
            if (fun_mode==APP_D24G_MODE)
            {
                // tpsll_change_report_rate(REPORT_RATE_125);
            }
            break;
        default:
            break;
    }
}
