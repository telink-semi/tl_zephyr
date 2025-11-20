/** @file app_public.h
 *  @brief
 */

/*
 * Copyright (c) 2025 Telink Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifdef __cplusplus
extern "C" {
#endif
#include "app_config.h"
#include "compiler.h"
#include "hog.h"
#include "app_usb.h"
#include "app_ble.h"
#include "app_2p4g.h"
#include "app_battery.h"
#include "app_kb_matrix.h"
#include "app_alg_keyscan.h"

#define BIT_SET(x, n)               ((x) |= BIT(n))
#define BIT_CLR(x, n)               ((x) &= ~BIT(n))
#define BIT_IS_SET(x, n)            ((x) & BIT(n))
#define BIT_FLIP(x, n)              ((x) ^= BIT(n))
#define BIT_SET_HIGH(x)             ((x) |= BIT((sizeof((x)) * 8 - 1)))  // set the highest bit
#define BIT_CLR_HIGH(x)             ((x) &= ~BIT((sizeof((x)) * 8 - 1))) // clr the highest bit
#define BIT_IS_SET_HIGH(x)          ((x) & BIT((sizeof((x)) * 8 - 1)))   // check the highest bit

typedef struct __attribute__((packed))  {
    unsigned short      size;
    unsigned short      num;

    unsigned short      wptr;
    unsigned short      rptr;

    unsigned char   *p;
}   pl_fifo_t;

void pp_fifo_reset (pl_fifo_t *f);
unsigned short pp_fifo_get_num(pl_fifo_t * f);
unsigned char *pp_fifo_get_ptr(pl_fifo_t * f);
int pp_fifo_push(pl_fifo_t * f, unsigned char cmd, unsigned char * buf, unsigned char len);
void pp_fifo_pop(pl_fifo_t *f);

extern pl_fifo_t tx_fifo;
extern pl_fifo_t d25fKbTxFifo;
extern pl_fifo_t d25fSppTxFifo;

extern void mb_irq_handler(void);

typedef struct
{
    uint32_t side_id; //4

    uint8_t peer_addr[MAC_ADDR_LEN];//6

} ST_FLASH_DEV_INFO;
extern ST_FLASH_DEV_INFO flash_dev_info;
extern int dev_info_idx;
extern uint32_t  flash_sector_2p4_inf;

typedef struct
{
    uint32_t side_id; //4
    uint8_t report_rate; //1

} ST_FLASH_DEV_OTHER_INFO;

extern ST_FLASH_DEV_OTHER_INFO flash_dev_other_info;
extern int dev_other_info_idx;
extern uint32_t  flash_sector_2p4_other_inf;

#define LED_IS_ON  0
#define LED_IS_OFF 1

///////////////////////////////////////////////////////////////////////////////////////////
#define PRESS_T_FN_FLAG      0X01
#define PRESS_KB_M_FLAG      0X02
#define PRESS_KB_P_FLAG      0X04
#define PRESS_KB_1_FLAG      0X08
#define PRESS_KB_2_FLAG      0X10
#define PRESS_KB_3_FLAG      0X20
#define PRESS_KB_4_FLAG      0X40
#define PRESS_KB_H_FLAG      0X80
#define PRESS_KB_L_FLAG      0X0100



#define PRESS_PAIR_BTN_FLAG             (PRESS_T_FN_FLAG|PRESS_KB_P_FLAG)
#define PRESS_MOUSE_AUTO_BTN_FLAG       (PRESS_T_FN_FLAG|PRESS_KB_M_FLAG)
#define PRESS_REPORT_RATE_8K_FLAG       (PRESS_T_FN_FLAG|PRESS_KB_H_FLAG)
#define PRESS_REPORT_RATE_125_FLAG      (PRESS_T_FN_FLAG|PRESS_KB_L_FLAG)
#define PRESS_BLE_PIPE1_FLAG       (PRESS_T_FN_FLAG|PRESS_KB_1_FLAG)
#define PRESS_BLE_PIPE2_FLAG       (PRESS_T_FN_FLAG|PRESS_KB_2_FLAG)
#define PRESS_BLE_PIPE3_FLAG       (PRESS_T_FN_FLAG|PRESS_KB_3_FLAG)
#define PRESS_BLE_PIPE4_FLAG       (PRESS_T_FN_FLAG|PRESS_KB_4_FLAG)

extern struct nvs_fs user_fs;
#define USER_STORAGE_APP_INFO_ID                1
#define APP_2P4G_PAIR_INFO_ID                   2
#define APP_2P4G_APP_INFO_ID                    3

enum {
    EMPTY_DATA_CMD=0,
    PAIR_DATA_CMD=1,
    RECONNECT_DATA_CMD=2,
    MOUSE_DATA=3,
    SPP_DATA=4,
    SPP_DATA_ACK=5,
    NORMAL_KB_DATA_CMD=6,
    CONSUME_KB_DATA_CMD=7,
    SYSTEM_KB_DATA_CMD=8,
    ALL_KB_DATA_CMD=9,
};


enum {
    MULTI_DEVICE_CMD = 0,
    MULTI_DEVICE_CHANGE_PIPE_1,
    MULTI_DEVICE_CHANGE_PIPE_2,
    MULTI_DEVICE_CHANGE_PIPE_3,
    MULTI_DEVICE_CHANGE_PIPE_4,
    MULTI_DEVICE_PAIR_PIPE_1,
    MULTI_DEVICE_PAIR_PIPE_2,
    MULTI_DEVICE_PAIR_PIPE_3,
    MULTI_DEVICE_PAIR_PIPE_4,
};

enum {
    BLE_USER_CMD = 0,
    BLE_SWITCH_PIPE = 7,
    BLE_START_PAIR = 16,
};

#define ALL_KEY_BUF_SIZE                            16
#define MAX_BTN_CNT                                 128//8*16  TOTAL_ROW*TOTAL_COL
#define REPORT_ALL_KB_SIZE                          16

typedef struct{

    unsigned char nk[8];  //normal key 
        
    unsigned char tk_bits[ALL_KEY_BUF_SIZE];  //total key
    unsigned char nk_bits[ALL_KEY_BUF_SIZE];  //all_key
    unsigned char ak_bits[ALL_KEY_BUF_SIZE];  //all_key

    unsigned char press_cnt;
    unsigned char cnt;
    unsigned char ck;  // consume_key
    unsigned char sk;  //system_key
        
    unsigned int special_key_press_f;
    
    unsigned char keycode[MAX_BTN_CNT]; 

}app_kb_data_t;

extern app_kb_data_t app_key_buf;
extern unsigned char    fn_flag;
extern struct gpio_dt_spec toggle_pin;

extern volatile uint32_t user_active_disconnect;
extern volatile unsigned char fun_mode;
extern struct gpio_dt_spec vbus_check_pin;

void key_fifo(unsigned char key_code);

unsigned char proc_hotkey(unsigned char key_code);

unsigned char special_key_press_flag_set(unsigned char key_code);

/**
 * @brief   read flash information
 * @param[in]   s_addr    - the base address of flash.
 * @param[in]   len     - the length(in byte, must be above 0) of content needs to read out from the page.
 * @param[out]  d_addr     - the start address of the buffer(ram address). 
 * @return  The offset address of the last data
 */

int flash_info_load(unsigned int s_addr, unsigned char *d_addr,  int len);


/**
 * @brief   save data to flash
 * @param[in]   addr    - the base address of flash.
 * @param[in]   len     - the length(in byte, must be above 0) of content needs to read out from the page.
 * @param[in]   buf     - data  buffer(ram address)
 * @param[in,out]  offset     - The offset address of the last data
 * @return  1: success  ,other: fail
 */

_attribute_ram_code_sec_ int save_data_to_flash(unsigned long addr, int len, unsigned char *buf,int *offset);
uint32_t fnv1a_hash(uint8_t *data, size_t len);

static _always_inline int tick1_exce_tick2(uint32_t tick1, uint32_t tick2)
{
    return (uint32_t)(tick1 - tick2) < BIT(30);
}

void app_proc_before_sleep(void);

void app_proc_after_sleep(void);

void user_timer_init(void);

void key_data_handle(void);

void special_key_event_handle(void);

void app_clock_init(app_clock_config_e select);

void keyboard_comm_init(void);

void app_ble_report_to_client(void);

void public_loop(void);

void k_timer_scan_loop_init(void);

void app_pc_kb_led_status(unsigned char status);

#if USE_K_TIMER_SCAN_MATRIX
_attribute_ram_code_sec_ void keyscan_loop(struct k_work *work);
#else
_attribute_ram_code_sec_ void keyscan_loop(void);
#endif

void start_change_ble_pipe_by_delay_work(void);

_attribute_ram_code_sec_noinline_ void app_ble_main_loop(void);

static inline kb_mode_t  app_get_kb_mode(void)
{
    #if (HW_BOARD_TYPE==HW_EVK_KEYBOARD)
    return KB_MODE_2P4G;
    #else
    return fun_mode;
    #endif
}

#ifdef __cplusplus
}
#endif
