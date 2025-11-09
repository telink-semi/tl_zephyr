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

unsigned char  g_report_rate_pin_level=0;
static unsigned char  g_last_report_rate_pin_level=0;

extern struct bt_conn *connected_handle;
volatile uint32_t user_active_disconnect = 0;

//ZH_TODO
ST_FLASH_DEV_INFO flash_dev_info  __attribute__ ((aligned (4)));
int dev_info_idx;

ST_FLASH_DEV_OTHER_INFO flash_dev_other_info  __attribute__ ((aligned (4)));
int dev_other_info_idx;

#if (HW_EVK_BOARD == 1)
_attribute_data_retention_ uint32_t flash_sector_2p4_inf=P24G_PAIR_INF_FLASH_ADDR_4M;
_attribute_data_retention_ uint32_t flash_sector_2p4_other_inf=P24G_OTHER_INF_FLASH_ADDR_4M;
#else
_attribute_data_retention_ uint32_t flash_sector_2p4_inf=P24G_PAIR_INF_FLASH_ADDR_2M;
_attribute_data_retention_ uint32_t flash_sector_2p4_other_inf=P24G_OTHER_INF_FLASH_ADDR_2M;
#endif
// _attribute_data_retention_ u32 flash_sector_ble_app_smp=BLE_APP_SMP_FLASH_ADDR_1M ;
// _attribute_data_retention_ u32 flash_sector_ble_app_pipe=BLE_APP_PIPE_FLASH_ADDR_1M;
//

#if TOGGLE_DEBUG_IO_ENABLE
struct gpio_dt_spec toggle_pin = GPIO_DT_SPEC_GET_OR(DT_ALIAS(toggle4), gpios, {0});
struct gpio_dt_spec toggle_pin_b5 = GPIO_DT_SPEC_GET_OR(DT_ALIAS(toggle5), gpios, {0});
#endif

#if (HW_BOARD_TYPE == HW_PRJ_KEYBOARD )
    struct gpio_dt_spec vbus_check_pin = GPIO_DT_SPEC_GET_OR(DT_ALIAS(vbuscheck0), gpios, {0});
    struct gpio_dt_spec mode_slect_pin = GPIO_DT_SPEC_GET_OR(DT_ALIAS(modeslect0), gpios, {0});
#elif(HW_BOARD_TYPE== HW_DIGIT_KEYBOARD)
    struct gpio_dt_spec vbus_check_pin = GPIO_DT_SPEC_GET_OR(DT_ALIAS(vbuscheck0), gpios, {0});
    struct gpio_dt_spec mode_2p4_pin = GPIO_DT_SPEC_GET_OR(DT_ALIAS(mode2p4), gpios, {0});
    struct gpio_dt_spec mode_ble_pin = GPIO_DT_SPEC_GET_OR(DT_ALIAS(modeble), gpios, {0});
#endif


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

__aligned(4)  unsigned char kb_buf_txfifo[KB_TX_FIFO_SIZE * KB_TX_FIFO_NUM];
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
    timer_set_cap_tick(TIMER0, 125 * sys_clk.pclk * 1);	//125uS
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

//ZH_TODO
 /**
  * @brief   save data to flash
  * @param[in]   addr    - the base address of flash.
  * @param[in]   len     - the length(in byte, must be above 0) of content needs to read out from the page.
  * @param[in]   buf     - data  buffer(ram address)
  * @param[in,out]  offset     - The offset address of the last data
  * @return  1: success  ,other: fail
  */
 
_attribute_ram_code_sec_ int save_data_to_flash(unsigned long addr, int len, unsigned char *buf,int *offset)
{
     unsigned char tmp_buf[len];
     int tmp_offset=0;
     
 
     if(offset[0]>(4096-2*len))
     {
         flash_erase_sector(addr);
         offset[0]=-len;
     }
 
     tmp_offset=offset[0]+len;
     
     for(unsigned char i=0;i<3;i++)
     {
         flash_write_page(addr+tmp_offset, len, buf);
         flash_read_page(addr+tmp_offset, len, tmp_buf);
         
         if(memcmp(tmp_buf, buf, len)==0)    //ZH_TODO memcpy to tmemcpy
         {
             offset[0]=tmp_offset;
             return 1;//#define SUCCESS                   0x00
         }
         else
         {
             flash_erase_sector(addr);
             tmp_offset=0;
         }
     }
 
     return 0;//write fail  
 }

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


 /**
  * @brief   read flash information
  * @param[in]   s_addr    - the base address of flash.
  * @param[in]   len     - the length(in byte, must be above 0) of content needs to read out from the page.
  * @param[out]  d_addr     - the start address of the buffer(ram address). 
  * @return  The offset address of the last data
  */
 
int flash_info_load(unsigned int s_addr, unsigned char *d_addr,  int len)
{
     int idx;
     unsigned int buf;
     for (idx = 0; idx < (4096 - len); idx += len)
     {
         flash_read_page((unsigned int)(s_addr + idx), 4, (unsigned char *)(&buf));
         if (buf == 0xffffffff)
         {
             break;
         }
     }
     idx -= len;
     if (idx < 0)        // no binding
     {
         return idx;
     }
     flash_read_page((unsigned int)(s_addr + idx), len, d_addr);
 
     if (idx > 3000)             //3k, erase flash
     {
         flash_erase_sector((unsigned int)s_addr);
 
        // sleep_us(10);
 
         flash_write_page((unsigned int)s_addr, len, d_addr);
         idx = 0;
     }
     return idx;
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

    if (app_d24p_get_state() == STATE_CONNECTED) {
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

_attribute_ram_code_sec_noinline_ void special_key_event_handle(void)
{
    uint8_t ret;

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
                    // DBG_GPIO_TOGGLE(APP_IO_EN, GPIO_PA6);
                    p24g_enable_pairing(true);
                }
                else if(fun_mode==KB_MODE_BLE)
                {
                    printk("ble mode pair start\r\n");
                    user_active_disconnect = 0;
                    user_active_disconnect += MULTI_DEVICE_PAIR_PIPE_1 + ble_app_pip_info.mast_id;
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
                    uint8_t ret = app_2p4g_set_stack_report_rate(REPORT_RATE_8K);

                    if (TLK_SUCCESS == ret) {
                        //LOG_INF("report rate change(%s)...\n", g_last_report_rate_pin_level ? "8k" : "125");
                        LOG_INF("report rate change()...\n");
                    } else {
                        LOG_INF("report rate change failed(ret %d)\n", ret);
                    }
                }
            }
            break;
        case PRESS_REPORT_RATE_125_FLAG:
            if(usb_connected_ok==0)
            {
                if(fun_mode==KB_MODE_2P4G)
                {
                    uint8_t ret = app_2p4g_set_stack_report_rate(REPORT_RATE_125);

                    if (TLK_SUCCESS == ret) {
                        //LOG_INF("report rate change(%s)...\n", g_last_report_rate_pin_level ? "8k" : "125");
                        LOG_INF("report rate change...\n");
                    } else {
                        LOG_INF("report rate change failed(ret %d)\n", ret);
                    }
                }
            }
            break;
        case PRESS_BLE_PIPE1_FLAG:
            printk("switch ble pipe 1\r\n");
            if(fun_mode == KB_MODE_BLE && ble_app_pip_info.mast_id != 0)
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
            if(fun_mode == KB_MODE_BLE  && ble_app_pip_info.mast_id != 1)
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
            if(fun_mode==KB_MODE_BLE && ble_app_pip_info.mast_id != 2)
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
            if(fun_mode==KB_MODE_BLE && ble_app_pip_info.mast_id != 3)
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
            if ((app_get_kb_mode() == KB_MODE_2P4G) && (usb_connected_ok == 0))
            {
                pp_fifo_push(&d25fKbTxFifo, NORMAL_KB_DATA_CMD, &app_key_buf.nk[0], nk_cnt + 2);
                DBG_GPIO_TOGGLE(APP_IO_EN, GPIO_PB5);
            }
            else
            {
                pp_fifo_push(&tx_fifo, NORMAL_KB_DATA_CMD, &app_key_buf.nk[0], 8);
            }
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
#if ((HW_BOARD_TYPE == HW_PRJ_KEYBOARD)||(HW_BOARD_TYPE == HW_DIGIT_KEYBOARD))

    uint8_t mode_2p4_pin_level = gpio_pin_get_dt(&mode_2p4_pin);
    uint8_t mode_ble_pin_level = gpio_pin_get_dt(&mode_ble_pin);

    if(mode_2p4_pin_level == 0 && mode_ble_pin_level == 0)
    {
        fun_mode = KB_MODE_USB;
    }
    else if(mode_2p4_pin_level == 0 && mode_ble_pin_level == 1)
    {
        fun_mode = KB_MODE_2P4G;
    }
    else if(mode_2p4_pin_level == 1 && mode_ble_pin_level == 0)
    {
        fun_mode = KB_MODE_BLE;
    }
    else
    {
        fun_mode = KB_MODE_USB;
    }
#endif
}

_attribute_ram_code_sec_ void app_clock_init(app_clock_config_e select)
{
    static uint8_t last_select = 0xFF;

    if (last_select == select) {
        return;
    }

    last_select = select;
    switch (select) 
    {
        case CLOCK_CONFIG_1V1_192_96:
            pm_set_dig_ldo(DIG_VOL_1V1_MODE, 1000);
            PLL_192M_D25F_192M_HCLK_N22_96M_PCLK_96M_MSPI_48M; // 192M 96M
            break;

        case CLOCK_CONFIG_1V1_96_96:
            pm_set_dig_ldo(DIG_VOL_1V1_MODE, 1000);
            PLL_192M_D25F_96M_HCLK_N22_96M_PCLK_96M_MSPI_48M; // 96M 96M
            break;

        case CLOCK_CONFIG_1V1_48_48:
            pm_set_dig_ldo(DIG_VOL_1V1_MODE, 1000);
            PLL_144M_D25F_48M_HCLK_N22_48M_PCLK_48M_MSPI_48M; // 48M 48M
            break;

        case CLOCK_CONFIG_1V_72_36:
            PLL_144M_D25F_72M_HCLK_N22_36M_PCLK_36M_MSPI_48M; // 72M 36M
            pm_set_dig_ldo(DIG_VOL_1V_MODE, 1000);
            break;

        case CLOCK_CONFIG_1V_64_32:
            PLL_192M_D25F_64M_HCLK_N22_32M_PCLK_32M_MSPI_48M; // 64M 32M
            pm_set_dig_ldo(DIG_VOL_1V_MODE, 1000);
            break;

        case CLOCK_CONFIG_1V_48_24:
            PLL_192M_D25F_48M_HCLK_N22_24M_PCLK_24M_MSPI_48M; // 48M 24M
            pm_set_dig_ldo(DIG_VOL_1V_MODE, 1000); //1.0
            break;
        
        default:
            break;
    }

    // delay_ms(1);

    // mcc_d25f_to_n22_set_clk_info();
}


static void peripheral_comm_init(void)
{
    int ret;

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

    ret = nvs_read(&user_fs, USER_STORAGE_APP_INFO_ID, (uint8_t *)&ble_app_pip_info.slave_mac_addr[0], sizeof(ST_BLE_APP_PIPE_INFO));
    if (ret == -ENOENT) {
        printk("NVS USER_STORAGE_APP_INFO_ID naver saved\n");
        ble_app_pip_info.slave_mac_addr[0] = 0;
        ble_app_pip_info.slave_mac_addr[1] = 0;
        ble_app_pip_info.slave_mac_addr[2] = 0;
        ble_app_pip_info.slave_mac_addr[3] = 0;
        ble_app_pip_info.ble_id[0] = 0xff;
        ble_app_pip_info.ble_id[1] = 0xff;
        ble_app_pip_info.ble_id[2] = 0xff;
        ble_app_pip_info.ble_id[3] = 0xff;
        ble_app_pip_info.mast_id = 0;
        save_ble_app_info();
    }
    LOG_INF("nvs read USER_STORAGE_APP_INFO_ID: %d\n", ret);
    LOG_INF("slave_mac_addr: %x %x %x %x\n", ble_app_pip_info.slave_mac_addr[0], ble_app_pip_info.slave_mac_addr[1], \
                                            ble_app_pip_info.slave_mac_addr[2], ble_app_pip_info.slave_mac_addr[3]);
    LOG_INF("mast_id: %d\n", ble_app_pip_info.mast_id);


    if (!gpio_is_ready_dt(&mode_2p4_pin)) {
        LOG_ERR("mode_2p4_pin gpio not ready");
        return -ENODEV;
    }

    ret = gpio_pin_configure_dt(&mode_2p4_pin, GPIO_INPUT);
    if (ret != 0) {
        printk("Error: failed to configure mode_2p4_pin io \n");
    }
    printk("configure mode_2p4_pin io ok\n");    

    if (!gpio_is_ready_dt(&mode_ble_pin)) {
        LOG_ERR("mode_ble_pin gpio not ready");
        return -ENODEV;
    }

    ret = gpio_pin_configure_dt(&mode_ble_pin, GPIO_INPUT);
    if (ret != 0) {
        printk("Error: failed to configure mode_ble_pin io \n");
    }
    printk("configure mode_ble_pin io ok\n");    

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

    #if BATT_CHECK_ENABLE
    /*adc init*/
    app_battery_check_init();
    #endif

    k_busy_wait(1000); // wait for 5ms
    check_mode();
    last_fun_mode = fun_mode;
    if (fun_mode == KB_MODE_2P4G)
    {
        uint8_t mac_public[6];
        uint8_t mac_random_static[6];
        random_generator_init();
        blc_initMacAddress(flash_sector_mac_address, mac_public, mac_random_static);
        memcpy(app_ctx.mac, mac_public, MAC_ADDR_LEN);
        p24g_pairing_info_check();
    }
}

extern void mb_irq_handler(void);
static int soc_tlx_mcc_init(void)
{
    IRQ_CONNECT(IRQ_MAILBOX_N22_TO_D25 + CONFIG_2ND_LVL_ISR_TBL_OFFSET, 2, mb_irq_handler, 0, 0);
	volatile uint32_t key = arch_irq_lock();
    sys_n22_start();
    mcc_d25f_service_init();
	arch_irq_unlock(key);

	ske_dig_en();

}


int keyboard_comm_init(void)
{
    peripheral_comm_init();
    soc_tlx_mcc_init();
    tlk_d25f_to_n22_mode_info(fun_mode);

    if (fun_mode == KB_MODE_2P4G)
    {
	    p24g_user_init_normal();
    } 
    else if (fun_mode == KB_MODE_BLE)
    {
        ble_init();
    }
    else
    {
        app_clock_init(CLOCK_CONFIG_1V1_48_48);
        #if USB_APP_FUN_ENABLE
	    /*usb init*/
	    usb_hw_init();
	    #endif
    }

    #if  DIGIT_KEYSCAN_FUN_ENABL
    digit_keyscan_init();
    #endif

    pp_fifo_reset(&tx_fifo);
    pp_fifo_reset(&d25fKbTxFifo);

    return 0;
}


#if USE_K_TIMER_SCAN_MATRIX
_attribute_ram_code_sec_ void keyscan_loop(struct k_work *work)
#else
_attribute_ram_code_sec_ void keyscan_loop(void)
#endif
{
#if USB_APP_FUN_ENABLE
    if (fun_mode == KB_MODE_USB && vbus_status == 1)
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
        app_2p4g_main_loop();
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
            printk("mast_id %d \n", ble_app_pip_info.mast_id);
            printk("usb_connected_ok %d \n", usb_connected_ok);
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

        check_mode();

        if (fun_mode != last_fun_mode) {
            printk("fun_mode change %d \n", fun_mode);
            pp_fifo_reset(&tx_fifo);
            pp_fifo_reset(&d25fKbTxFifo);

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
    }
}
