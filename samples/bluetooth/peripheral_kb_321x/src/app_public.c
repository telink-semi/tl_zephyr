/* app_public.c - Application main entry point */

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
#include <inttypes.h>
#include <zephyr/drivers/interrupt_controller/riscv_plic.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/fs/nvs.h>
#include <zephyr/storage/flash_map.h>
#include <zephyr/sys/reboot.h>


#include "timer.c"
#include "app_public.h"
#include <zephyr/bluetooth/conn.h>
#include "drivers.h"
#include "stack/ble/ble.h"


#define LOG_LEVEL LOG_LEVEL_DBG
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(app_public);

volatile unsigned char fun_mode = 0;
static unsigned char last_fun_mode = 1;
unsigned char  mode_pin_level = 0;

extern struct bt_conn *connected_handle;
volatile uint32_t user_active_disconnect = 0;
uint8_t mac_public[6];
uint8_t mac_random_static[6];
ST_FLASH_DEV_INFO flash_dev_info  __attribute__ ((aligned (4)));

uint32_t loop_cnt;
uint32_t idle_tick;
uint32_t idle_count;
uint32_t adv_begin_tick;
uint32_t adv_count = 0;

#if TOGGLE_DEBUG_IO_ENABLE
struct gpio_dt_spec toggle_pin = GPIO_DT_SPEC_GET_OR(DT_ALIAS(toggle-sws), gpios, {0});
#endif

struct gpio_dt_spec vbus_check_pin = GPIO_DT_SPEC_GET_OR(DT_ALIAS(vbuscheck0), gpios, {0});
struct gpio_dt_spec mode_pin = GPIO_DT_SPEC_GET_OR(DT_ALIAS(modeswitch), gpios, {0});
struct gpio_dt_spec mos_ctl_pin = GPIO_DT_SPEC_GET_OR(DT_ALIAS(mos-ctrl), gpios, {0});
struct gpio_dt_spec led_24g_pin = GPIO_DT_SPEC_GET_OR(DT_ALIAS(led-24g), gpios, {0});
struct gpio_dt_spec led_ble_pin = GPIO_DT_SPEC_GET_OR(DT_ALIAS(led-ble), gpios, {0});
struct gpio_dt_spec led_bat_pin = GPIO_DT_SPEC_GET_OR(DT_ALIAS(led-bat), gpios, {0});


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

static struct k_spinlock pool_lock;  

_attribute_ram_code_sec_ void pp_fifo_reset (pl_fifo_t *f)
{
     k_spinlock_key_t key = k_spin_lock(&pool_lock);
     f->wptr = 0;
     f->rptr = 0;
     k_spin_unlock(&pool_lock, key);
}

_attribute_ram_code_sec_ int pp_fifo_push(pl_fifo_t *f,unsigned char cmd,unsigned char *buf, unsigned char len)
{
    k_spinlock_key_t key = k_spin_lock(&pool_lock);
    if(len>(f->size-2))
    {
        k_spin_unlock(&pool_lock, key);
        return TLK_ERR_INVALID_LENGTH;
    }
    if (((f->wptr - f->rptr) & 255) <(f->num-1))
    {
        unsigned char *pd =(unsigned char*) (f->p + (f->wptr& (f->num-1)) * f->size);   

        pd[0] = len;
        pd[1]=cmd;
        memcpy (&pd[2], &buf[0], len);

        f->wptr++;
        k_spin_unlock(&pool_lock, key);
        return TLK_SUCCESS;
    }
    k_spin_unlock(&pool_lock, key);
    return TLK_ERR_BUFFER_FULL;
}

_attribute_ram_code_sec_ unsigned char *pp_fifo_get_ptr (pl_fifo_t *f)
{
    k_spinlock_key_t key = k_spin_lock(&pool_lock);
    if (f->rptr != f->wptr)
    {
        unsigned char *p = f->p + (f->rptr & (f->num-1)) * f->size;
        k_spin_unlock(&pool_lock, key);
        return p;
    }
    k_spin_unlock(&pool_lock, key);
    return 0;
}


_attribute_ram_code_sec_ unsigned short pp_fifo_get_num(pl_fifo_t *f)
{
    k_spinlock_key_t key = k_spin_lock(&pool_lock);
    unsigned short num =(f->wptr - f->rptr) & 255;
    k_spin_unlock(&pool_lock, key);
    return num;
}

_attribute_ram_code_sec_ void pp_fifo_pop(pl_fifo_t *f)
 {
    k_spinlock_key_t key = k_spin_lock(&pool_lock);
    f->rptr++;
    k_spin_unlock(&pool_lock, key);
 }


unsigned short consumer_list[]={
0x221,      //0xa3 C_WWW_SEARCH     
0x223,      //0xa4 C_WWW_HOME
0x224,      //0xa5 C_WWW_BACKWARD
0x225,      //0xa6 C_WWW_FORWARD
0x226,      //0xa7 C_WWW_STOP
0x227,      //0xa8 C_WWW_REFRESH
0x22A,      //0xa9 C_MY_FAVORITE
0x183,      //0xaa C_MEDIA_SELECT
    
0x18A,      //0xab C_EMAIL
0x192,      //0xac C_CALCULATOR
0x194,      //0xad C_MY_COMPUTER
0xB5,       //0xae C_NEXT_TRACK
0xB6,       //0xaf C_PRE_TRACK
0xB7,       //0xb0 C_STOP
0xCD,       //0xb1 C_PLAY_PAUSE
0xE2,       //0xb2 C_MUTE   
    
0xE9,       //0xb3 C_VOL_INC
0xEA,       //0xb4 C_VOL_DEC    
0x00,       //0xb5 telink Զ    ??
0x22D,      //0xb6 USAGE ZOOM IN
0x22E,      //0xb7   USAGE ZOOM OUT 
0x236,      //0xb8   USAGE PAN LEFT
0x237,      //0xb9   USAGE PAN RIGHT
0x30B,      //0xba  C_BRIGHT_INC    
    
0x30A,      //0xbb  C_BRIGHT_DEC
0xB8,       //0Xbc   c_rject
0x30,       //0Xbd  C_POWER         
0x19E,      //0Xbe  C_TERMINAL_LOCK 
};

#define KB_TX_FIFO_SIZE 24
#define KB_TX_FIFO_NUM 8

__aligned(4)  unsigned char buf_txfifo[40*8];
pl_fifo_t tx_fifo={  
     .size=40,
     .num=8,
     .wptr=0,
     .rptr=0,
     .p=buf_txfifo,
 };

app_kb_data_t app_key_buf;
unsigned char fn_flag = 0;


/* Timer0 interrupt handler */
_attribute_ram_code_sec_ void timer0_isr(void)
{
    if (timer_get_irq_status(FLD_TMR0_MODE_IRQ)){
		timer_clr_irq_status(FLD_TMR0_MODE_IRQ); //Clear IRQ status

        keyscan_loop();
	}
}

void user_timer_init(void)
{
     /* Timer0 configuration */
    timer_set_init_tick(TIMER0, 0);
    timer_set_cap_tick(TIMER0, 500 * sys_clk.pclk * 1);	//125uS
    timer_set_mode(TIMER0, TIMER_MODE_SYSCLK);
    timer_set_irq_mask(FLD_TMR0_MODE_IRQ);
    IRQ_CONNECT(CONFIG_2ND_LVL_ISR_TBL_OFFSET + IRQ_TIMER0, 2, timer0_isr, 0, 0);
    riscv_plic_set_priority(IRQ_TIMER0, 3);
    riscv_plic_irq_enable(IRQ_TIMER0);

     /* Start timers */
	timer_start(TIMER0);
}



_attribute_ram_code_sec_ void key_fifo(unsigned char key_code)
{
    unsigned char real_key_code = special_key_press_flag_set(key_code);
    if (real_key_code==0) //key_code=0 not save
    {
        return;
    }
    
    app_key_buf.cnt++;
    if (app_key_buf.cnt > MAX_BTN_CNT)
    {
        return;
    }
    app_key_buf.keycode[app_key_buf.cnt-1] = real_key_code;
}


_attribute_ram_code_sec_noinline_ unsigned char proc_hotkey(unsigned char key_code)
{

    if((key_code>=0xe0) && (key_code<0xe8))
    {
        app_key_buf.nk[0] |= (1 << (key_code-0xe0));
        return 1;
    }

    if((key_code >= C_INDEX_START) && (key_code <= C_INDEX_END))
    {
        app_key_buf.ck = key_code;
        return 1;
    }
    
    if ((key_code >= S_SLEEP) && (key_code <= S_WAKEUP))
    {
        app_key_buf.sk = key_code;
        return 1;
    }

    if(key_code>0x9f)
    {
        return 1;
    }

    return 0;
}


_attribute_ram_code_sec_noinline_ unsigned char special_key_press_flag_set(unsigned char key_code)
{
     unsigned char real_key_code = key_code;
     if(key_code==T_FN)
     {
        app_key_buf.special_key_press_f |= PRESS_T_FN_FLAG;
        fn_flag = 1;
     }
     else if(fn_flag)
     {
        switch (key_code)
        {
            case KB_M:
                app_key_buf.special_key_press_f|=PRESS_KB_M_FLAG;
                real_key_code=0;
                break;
            case KB_P:
                app_key_buf.special_key_press_f|=PRESS_KB_P_FLAG;
                real_key_code=0;
                break;
            case KB_H:
                app_key_buf.special_key_press_f|=PRESS_KB_H_FLAG;
                real_key_code=0;
                break;
            case KB_L:
                app_key_buf.special_key_press_f|=PRESS_KB_L_FLAG;
                real_key_code=0;
                break;
             case KB_1:
                app_key_buf.special_key_press_f|=PRESS_KB_1_FLAG;
                real_key_code=0;
                break;
             case KB_2:
                app_key_buf.special_key_press_f|=PRESS_KB_2_FLAG;
                real_key_code=0;
                break;
             case KB_3:
                app_key_buf.special_key_press_f|=PRESS_KB_3_FLAG;
                real_key_code=0;
                break;
             case KB_4:
                app_key_buf.special_key_press_f|=PRESS_KB_4_FLAG;
                real_key_code=0;
                break;
            case KB_F1:
                real_key_code=C_mute;
                break;
            
        }
     }
    
    return real_key_code;
}


_attribute_ram_code_sec_noinline_ void special_key_event_handle(void)
{

     if(app_key_buf.special_key_press_f==0)
     {
        fn_flag=0;
     }
    switch (app_key_buf.special_key_press_f)
    {
        case PRESS_MOUSE_AUTO_BTN_FLAG:
            #if 0
            auto_test_mouse ^= 0x01;
            p24g_send_sm_msg(P24G_SM_CMD_MOUSE_DRAW, 0, 0, 0);
            #endif
            break;
        case PRESS_PAIR_BTN_FLAG:
                if(fun_mode==KB_MODE_2P4G)
                {
                    LOG_INF("2p4g mode pair start\r\n");
                }
                else if(fun_mode==KB_MODE_BLE)
                {
                    printk("ble mode pair start\r\n");
                    user_active_disconnect = 0;
                    user_active_disconnect += MULTI_DEVICE_PAIR_PIPE_1 + flash_dev_info.mast_id;
                    if (connected_handle) {
                        BIT_SET(user_active_disconnect, BLE_START_PAIR);
                    } else {
                        start_pairing_by_delay_work();
                    }                    
                }
            break;
        case PRESS_REPORT_RATE_8K_FLAG:
            if(usb_connected_ok==0)
            {
                if(fun_mode==KB_MODE_2P4G)
                {

                }
            }
            break;
        case PRESS_REPORT_RATE_125_FLAG:
            if(usb_connected_ok==0)
            {
                if(fun_mode==KB_MODE_2P4G)
                {
                }
            }
            break;
        case PRESS_BLE_PIPE1_FLAG:
            printk("switch ble pipe 1\r\n");
            if(fun_mode == KB_MODE_BLE && flash_dev_info.mast_id != 0)
            {
                user_active_disconnect = 0;
                user_active_disconnect += MULTI_DEVICE_CHANGE_PIPE_1;

                if (connected_handle) {
                    BIT_SET(user_active_disconnect, BLE_SWITCH_PIPE);
                } else {
                    start_change_ble_pipe_by_delay_work();
                }
            }
            break;
        case PRESS_BLE_PIPE2_FLAG:
            printk("switch ble pipe 2\r\n");
            if(fun_mode == KB_MODE_BLE  && flash_dev_info.mast_id != 1)
            {
                user_active_disconnect = 0;
                user_active_disconnect += MULTI_DEVICE_CHANGE_PIPE_2;
                if (connected_handle) {
                    BIT_SET(user_active_disconnect, BLE_SWITCH_PIPE);
                } else {
                    start_change_ble_pipe_by_delay_work();
                }
            }
            break;
        case PRESS_BLE_PIPE3_FLAG:
            printk("switch ble pipe 3\r\n");
            if(fun_mode==KB_MODE_BLE && flash_dev_info.mast_id != 2)
            {
                user_active_disconnect = 0;
                user_active_disconnect += MULTI_DEVICE_CHANGE_PIPE_3;
                if (connected_handle) {
                    BIT_SET(user_active_disconnect, BLE_SWITCH_PIPE);
                } else {
                    start_change_ble_pipe_by_delay_work();
                }
            }
            break;
        case PRESS_BLE_PIPE4_FLAG:
            printk("switch ble pipe 4\r\n");
            if(fun_mode==KB_MODE_BLE && flash_dev_info.mast_id != 3)
            {
                user_active_disconnect = 0;
                user_active_disconnect += MULTI_DEVICE_CHANGE_PIPE_4;
                if (connected_handle) {
                    BIT_SET(user_active_disconnect, BLE_SWITCH_PIPE);
                } else {
                    start_change_ble_pipe_by_delay_work();
                }
            }
            break;  
    }
}

_attribute_ram_code_sec_noinline_ void key_data_handle(void)
{
    static unsigned char  nk_cnt=0;
    static unsigned char ck_last=0;
    static unsigned char sk_last=0;
    static unsigned char nk_last[8]={0,0,0,0,0,0,0,0};
    static unsigned char ak_last[ALL_KEY_BUF_SIZE]={0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
    unsigned char i;
    unsigned char index=0;
    unsigned char bit=0;
    unsigned char key=0;
    app_key_buf.nk[0]=0;
    app_key_buf.ck=0;
    app_key_buf.sk=0;
   
    memset((unsigned char *)&app_key_buf.tk_bits[0], 0, ALL_KEY_BUF_SIZE);
     
    for(i = 0; i < app_key_buf.cnt; i++)
    {
        key = app_key_buf.keycode[i];
        if(proc_hotkey(key) == 0)
        {
            index = key/8;
            bit = key%8;
            app_key_buf.tk_bits[index] |= 1 << bit;
        }
    }
    //get normal kb pipe  key bits  and all kb pipe key bits
    int cnt=0;
    memset((unsigned char *)&app_key_buf.nk[2], 0, 6);

    for( i=0;i<ALL_KEY_BUF_SIZE;i++)
    {
        for(int k = 0; k < 8; k++)
        {
            
            if(app_key_buf.tk_bits[i]&(1<<k))
            {
                if(app_key_buf.nk_bits[i]&(1<<k))
                {
                    
                }
                else if(app_key_buf.ak_bits[i]&(1<<k))
                {
                    
                }
                else if(nk_cnt<6)
                {
                    app_key_buf.nk_bits[i]|=(1<<k);
                    nk_cnt++;
                }
                else 
                {
                    app_key_buf.ak_bits[i]|=(1<<k);
                }
            }
            else
            {
                if(app_key_buf.nk_bits[i]&(1<<k))
                {
                    nk_cnt--;
                    app_key_buf.nk_bits[i]&=0xff-(1<<k);
                }
                else if(app_key_buf.ak_bits[i]&(1<<k))
                {
                    app_key_buf.ak_bits[i]&=0xff-(1<<k);
                }
            }

            if(cnt<6)
            {
                if(app_key_buf.nk_bits[i]&(1<<k))
                {
                    app_key_buf.nk[2+cnt]=i*8+k;
                    cnt++;
                }
            }
        
        }
    }

    nk_cnt=cnt;

    for(i=0;i<8;i++)
    {
        if(nk_last[i] != app_key_buf.nk[i])
        {
            memcpy(nk_last,app_key_buf.nk,8);
            //unsigned int r = core_interrupt_disable();
            pp_fifo_push(&tx_fifo, NORMAL_KB_DATA_CMD, &app_key_buf.nk[0], 8);
            //core_restore_interrupt(r);
            break;  
        }
    }

    for(i = 0; i < ALL_KEY_BUF_SIZE; i++)
    {
        if(ak_last[i] != app_key_buf.ak_bits[i])
        {
            memcpy(ak_last,app_key_buf.ak_bits,ALL_KEY_BUF_SIZE);
            //unsigned int r = core_interrupt_disable();
            pp_fifo_push(&tx_fifo, ALL_KB_DATA_CMD, &app_key_buf.ak_bits[0], ALL_KEY_BUF_SIZE);
            //core_restore_interrupt(r);
            break;
        }
    }
    
    if(ck_last != app_key_buf.ck)
    {
        ck_last=app_key_buf.ck;
         unsigned short temp=0;
         if(ck_last!=0)
         {
            temp=consumer_list[ck_last-C_INDEX_START];
         }
         else
         {
             temp=0;
         }

         //unsigned int r = core_interrupt_disable();
         pp_fifo_push(&tx_fifo, CONSUME_KB_DATA_CMD, (unsigned char *)&temp, 2);
         //core_restore_interrupt(r);
    }

    if(sk_last != app_key_buf.sk)
    {
        sk_last = app_key_buf.sk;
        unsigned char temp=1<<(sk_last-S_SLEEP);
        //unsigned int r = core_interrupt_disable();
        pp_fifo_push(&tx_fifo, SYSTEM_KB_DATA_CMD, (unsigned char *)&temp, 1);
        //core_restore_interrupt(r);
    }
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

_attribute_ram_code_sec_ void check_mode(void)
{
    uint8_t mode_pin_level = gpio_pin_get_dt(&mode_pin);

    //printk("mode_pin_level %d\n", mode_pin_level);
}

void save_dev_info(void)
{
	int ret = nvs_write(&user_fs, APP_USER_INFO_ID, (uint8_t *)&flash_dev_info.slave_mac_addr[0], sizeof(ST_FLASH_DEV_INFO));
    printk("NVS Write result: %d\n", ret);
}

static int peripheral_comm_init(void)
{
    int ret;

    random_generator_init();
    //TODO:
    printk("flash_sector_mac_address %x\n", flash_sector_mac_address);
    blc_initMacAddress(flash_sector_mac_address, mac_public, mac_random_static);
    LOG_HEXDUMP_INF(mac_public, 6, "mac_addr");

    /* 初始化NVS文件系统 */
    user_fs.flash_device = NVS_PARTITION_DEVICE;
    user_fs.offset = NVS_PARTITION_OFFSET;
    user_fs.sector_size = NVS_SECTOR_SIZE;
    user_fs.sector_count = NVS_SECTOR_COUNT;

    ret = nvs_mount(&user_fs);
    if (ret) {
        LOG_INF("Error: NVS init failed: %d\n", ret);
        return;
    }
    LOG_INF("NVS initialized successfully.\n");

    ret = nvs_read(&user_fs, APP_USER_INFO_ID, (uint8_t *)&flash_dev_info.slave_mac_addr[0], sizeof(ST_FLASH_DEV_INFO));
    if (ret == -ENOENT) {
        printk("NVS APP_USER_INFO_ID naver saved\n");
        flash_dev_info.slave_mac_addr[0] = 0;
        flash_dev_info.slave_mac_addr[1] = 0;
        flash_dev_info.slave_mac_addr[2] = 0;
        flash_dev_info.slave_mac_addr[3] = 0;
        flash_dev_info.ble_id[0] = 0xff;
        flash_dev_info.ble_id[1] = 0xff;
        flash_dev_info.ble_id[2] = 0xff;
        flash_dev_info.ble_id[3] = 0xff;
        flash_dev_info.mast_id = 0;
        save_dev_info();
    }
    LOG_INF("nvs read APP_USER_INFO_ID: %d\n", ret);
    LOG_INF("slave_mac_addr: %x %x %x %x\n", flash_dev_info.slave_mac_addr[0], flash_dev_info.slave_mac_addr[1], \
                                            flash_dev_info.slave_mac_addr[2], flash_dev_info.slave_mac_addr[3]);
    LOG_INF("mast_id: %d\n", flash_dev_info.mast_id);


    if (!gpio_is_ready_dt(&mode_pin)) {
        LOG_ERR("mode_pin gpio not ready");
        return -ENODEV;
    }

    ret = gpio_pin_configure_dt(&mode_pin, GPIO_INPUT);
    if (ret != 0) {
        printk("Error: failed to configure mode_pin io \n");
    }
    printk("configure mode_pin io ok\n");    

    if (!gpio_is_ready_dt(&vbus_check_pin)) {
        LOG_ERR("Vbus check gpio not ready");
        return -ENODEV;
    }
    ret = gpio_pin_configure_dt(&vbus_check_pin, GPIO_INPUT);
    if (ret != 0) {
        LOG_INF("Error: failed to configure vbus check io \n");
    }
    LOG_INF("configure vbus_check_pin io ok\n");

#if TOGGLE_DEBUG_IO_ENABLE
    if (!gpio_is_ready_dt(&toggle_pin)) {
        LOG_INF("toggle io is not ready\n");
         return -ENODEV;
    }
    ret = gpio_pin_configure_dt(&toggle_pin, GPIO_OUTPUT);
    if (ret != 0) {
        LOG_INF("Error: failed to configure toogle io \n");
    }
    LOG_INF("configure toogle io ok\n");
#endif

    if (!gpio_is_ready_dt(&mos_ctl_pin)) {
        LOG_INF("mos_ctl_pin io is not ready\n");
         return -ENODEV;
    }
    ret = gpio_pin_configure_dt(&mos_ctl_pin, GPIO_OUTPUT);
    if (ret != 0) {
        LOG_INF("Error: failed to configure mos_ctl_pin io \n");
    }
    LOG_INF("configure mos_ctl_pin io ok\n");

    if (!gpio_is_ready_dt(&led_24g_pin)) {
        LOG_INF("led_24g_pin io is not ready\n");
         return -ENODEV;
    }
    ret = gpio_pin_configure_dt(&led_24g_pin, GPIO_OUTPUT);
    if (ret != 0) {
        LOG_INF("Error: failed to configure led_24g_pin io \n");
    }
    LOG_INF("configure led_24g_pin io ok\n");

    ret = gpio_pin_configure_dt(&mos_ctl_pin, GPIO_OUTPUT);
    if (ret != 0) {
        LOG_INF("Error: failed to configure mos_ctl_pin io \n");
    }
    LOG_INF("configure mos_ctl_pin io ok\n");

    if (!gpio_is_ready_dt(&led_ble_pin)) {
        LOG_INF("led_ble_pin io is not ready\n");
         return -ENODEV;
    }
    ret = gpio_pin_configure_dt(&led_ble_pin, GPIO_OUTPUT);
    if (ret != 0) {
        LOG_INF("Error: failed to configure led_ble_pin io \n");
    }
    LOG_INF("configure led_ble_pin io ok\n");

    if (!gpio_is_ready_dt(&led_bat_pin)) {
        LOG_INF("led_bat_pin io is not ready\n");
         return -ENODEV;
    }
    ret = gpio_pin_configure_dt(&led_bat_pin, GPIO_OUTPUT);
    if (ret != 0) {
        LOG_INF("Error: failed to configure led_bat_pin io \n");
    }
    LOG_INF("configure led_bat_pin io ok\n");

    #if BATT_CHECK_ENABLE
    /*adc init*/
    app_battery_check_init();
    #endif

    k_busy_wait(1000); // wait for 5ms
    check_mode();

    last_fun_mode = fun_mode;
}

/**
 * @brief    BLE Controller IRQs initialization
 */
static void tlx_bt_irq_init()
{
#if CONFIG_SOC_RISCV_TELINK_TL321X || CONFIG_SOC_RISCV_TELINK_TL721X || CONFIG_SOC_RISCV_TELINK_TL322X || CONFIG_SOC_RISCV_TELINK_TL323X
	plic_preempt_feature_dis();
	flash_plic_preempt_config(0,1);
#endif

    if (fun_mode == KB_MODE_BLE) {
        /* Init STimer IRQ */
        IRQ_CONNECT(IRQ_SYSTIMER + CONFIG_2ND_LVL_ISR_TBL_OFFSET, 2, stimer_irq_handler, 0, 0);
        plic_set_priority(IRQ_SYSTIMER, 2);
    }

	/* Init RF IRQ */
#if CONFIG_DYNAMIC_INTERRUPTS
	irq_connect_dynamic(IRQ_ZB_RT + CONFIG_2ND_LVL_ISR_TBL_OFFSET, 2, rf_irq_handler, 0, 0);
#else
	IRQ_CONNECT(IRQ_ZB_RT + CONFIG_2ND_LVL_ISR_TBL_OFFSET, 2, rf_irq_handler, 0, 0);
#endif

	plic_set_priority(IRQ_ZB_RT, 2);
}

int keyboard_comm_init(void)
{
    peripheral_comm_init();

    fun_mode = KB_MODE_2P4G;
    tlx_bt_irq_init();

    if (fun_mode == KB_MODE_2P4G)
    {
	     pp_rf_init(1);
         d24_user_init();
    } 
    else if (fun_mode == KB_MODE_BLE)
    {
        ble_init();
    }
    else
    {
        #if USB_APP_FUN_ENABLE
	    /*usb init*/
	    usb_hw_init();
	    #endif
    }

    #if  DIGIT_KEYSCAN_FUN_ENABL
    digit_keyscan_init();
    #endif

    pp_fifo_reset(&tx_fifo);

    return 0;
}


#if USE_K_TIMER_SCAN_MATRIX
_attribute_ram_code_sec_ void keyscan_loop(struct k_work *work)
#else
_attribute_ram_code_sec_ void keyscan_loop(void)
#endif
{
#if USB_APP_FUN_ENABLE
    if (fun_mode == KB_MODE_USB /*&& vbus_status == 1*/)// TODO: vbus_status
    {
        app_usb_main_loop();
    }
#endif
#if ALG_KEYSCAN_APP_FUN_ENABLE
    key_scan();
#endif
#if  DIGIT_KEYSCAN_FUN_ENABL
     #if TOGGLE_DEBUG_IO_ENABLE
     gpio_pin_set_dt(&toggle_pin, 1);
     #endif
     /*
      * hw digital keyscan 1.3us
      * sw digital keyscan 96M 88us    192M 47us
      */
     digit_keyscan_handle();
     #if TOGGLE_DEBUG_IO_ENABLE
     gpio_pin_set_dt(&toggle_pin, 0);
     #endif
#endif

    if(app_get_kb_mode() == KB_MODE_2P4G) {
         d24_main_loop();
    }
}


 _attribute_ram_code_sec_ void public_loop(void)
{
    static uint32_t loop_cnt;
    static uint32_t last_time = 0;

    loop_cnt++;

    if(app_get_kb_mode() == KB_MODE_BLE)
    {
        app_ble_main_loop();
    }

    if (loop_cnt > 15) { // (15 * 3)ms
        loop_cnt = 0;

        if(k_uptime_get_32() - last_time > 5000)//5s
        {
            last_time = k_uptime_get_32();
            printk("fun_mode %d \n", fun_mode);
            printk("vbus_status %d \n", vbus_status);
            printk("ble_status %d \n", ble_status);
            printk("mast_id %d \n", flash_dev_info.mast_id);
            printk("usb_connected_ok %d \n", usb_connected_ok);
            printk("rf_state %d \n", rf_state);
        }

        if (BIT_IS_SET(user_active_disconnect, BLE_SWITCH_PIPE)) { 
            printk("disconnect in loop \r\n");
            disconnect_current_connection();
            BIT_CLR(user_active_disconnect, BLE_SWITCH_PIPE);
        } else if(BIT_IS_SET(user_active_disconnect, BLE_START_PAIR)){
            disconnect_current_connection();
            BIT_CLR(user_active_disconnect, BLE_START_PAIR);
        }


    #if (BATT_CHECK_ENABLE)
        if ((k_uptime_get_32() - lowBattDet_tick) > 5000)
        {
            LOG_INF("battery check\r\n");
            lowBattDet_tick = k_uptime_get_32();
            app_battery_check(0);
        }
    #endif

#if USB_APP_FUN_ENABLE
        app_usb_status_check();
#endif

        // TODO: check_mode();
        #if 0
        if (fun_mode != last_fun_mode) {
            printk("fun_mode change %d \n", fun_mode);
            pp_fifo_reset(&tx_fifo);

            if (fun_mode == KB_MODE_2P4G){
                if (last_fun_mode == KB_MODE_USB) {
                    printk("usb mode_exit enter 2p4g mode\r\n");
                    // app_usb_mode_exit();
                    sys_reboot(SYS_REBOOT_COLD);
                } else if (last_fun_mode == KB_MODE_BLE) {
                    printk("ble_mode_exit enter 2p4g mode\r\n");
                    // ble_mode_exit();
                    sys_reboot(SYS_REBOOT_COLD);
                }
            } else if (fun_mode == KB_MODE_BLE){
                if (last_fun_mode == KB_MODE_USB) {
                    printk("app_usb_mode_exit  enter ble mode\r\n");
                    // app_usb_mode_exit();
                    //ble_init();
                    sys_reboot(SYS_REBOOT_COLD);
                } else if (last_fun_mode == KB_MODE_2P4G) {
                    printk("app_usb_mode_exit enter 2p4g mode\r\n");
                    // app_2p4g_mode_exit();
                    sys_reboot(SYS_REBOOT_COLD);
                }
                // ble_mode_enter();
            } else if (fun_mode == KB_MODE_USB){
                if (last_fun_mode == KB_MODE_2P4G) {
                    printk("2p4g mode exit enter usb mode\r\n");
                    // app_2p4g_mode_exit();
                    sys_reboot(SYS_REBOOT_COLD);
                } else if (last_fun_mode == KB_MODE_BLE) {
                    printk("ble mode exit enter usb mode\r\n");
                     //ble_mode_exit();
                     //bt_disable();
                     sys_reboot(SYS_REBOOT_COLD);
                }
                if (vbus_status == 1) {
                     // app_usb_mode_enter();
                }
            }
            last_fun_mode = fun_mode;
        }
        #endif
    }
}


/**
 * @brief	The keyboard display the PC status
 * @param	status, PC status indicator
 * @return	none
 */
_attribute_ram_code_ void kb_led_out(uint8_t status)
{
#if HAS_NUM_LED
	led_num_set((status&0x01)?LED_ON:LED_OFF);
#endif

#if HAS_CAPS_LED
	led_caps_set((status&0x02)?LED_ON:LED_OFF);
#endif

#if HAS_SCROLL_LED
	led_scroll_set((status&0x04)?LED_ON:LED_OFF);
#endif
}


/**
 * @brief	clear_pair_flag
 * @param	none
 * @return	none
 */
void clear_pair_flag(void)
{
    printk("[PUBLIC] clear_pair_flag\n");
    pair_flag = 0;
    analog_write(USED_PAIR_ANA_REG, pair_flag);
}


/**
 * @brief	set_pair_flag
 * @param	none
 * @return	none
 */
void set_pair_flag(void)
{
    pair_flag = 1;
    printk("[PUBLIC] set_pair_flag %d\n", pair_flag);
    analog_write(USED_PAIR_ANA_REG, pair_flag);
}

/**
 * @brief	clear idle count
 * @param	none
 * @return	none
 */
_attribute_ram_code_ void reset_idle_status(void)
{
	if (pair_flag)
	{
		return;
	}
	idle_count = 0;
	loop_cnt = 0;
	idle_tick = clock_time();
	adv_begin_tick = idle_tick|1;
	adv_count = 0;
}

/**
 * @brief	idle count poll
 * @param	none
 * @return	none
 */
_attribute_ram_code_ void idle_status_poll(void)
{
    u8 n;

    n = ((u32)(clock_time() - idle_tick)) / SYSTEM_TIMER_TICK_1S;

    idle_tick += n * SYSTEM_TIMER_TICK_1S;

    idle_count += n;
}

/**
 * @brief	ADV count poll
 * @param	none
 * @return	none
 */
_attribute_ram_code_ void adv_count_poll(void)
{
    u8 n;

    n = ((u32)(clock_time() - adv_begin_tick)) / SYSTEM_TIMER_TICK_1S;

    adv_begin_tick += n * SYSTEM_TIMER_TICK_1S;

    adv_count += n;
}