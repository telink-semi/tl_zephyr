/** @file
 *  @brief HoG Service sample
 */

/*
 * Copyright (c) 2016 Intel Corporation
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
#include "app_24g.h"
#include "app_24g_rf.h"
#include "app_battery.h"
#include "app_kb_matrix.h"
#include "app_common_config.h"


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
extern unsigned int flash_sector_mac_address;

extern uint32_t loop_cnt;
extern uint32_t idle_count;
extern uint32_t adv_begin_tick;
extern uint32_t adv_count;
extern uint8_t mac_public[6];

#define   LED_IS_ON  0
#define   LED_IS_OFF 1

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
#define APP_USER_INFO_ID                1

typedef struct
{
    unsigned char slave_mac_addr[4];//4
    int ble_id[4];//20
    uint32_t dongle_id;
    uint8_t temp2[3]; //23
    uint8_t mast_id;//24
    int idx;//28
} ST_FLASH_DEV_INFO;
extern ST_FLASH_DEV_INFO flash_dev_info;

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
	ERROR_DATA=0X55,
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

    unsigned char nk[8];//normal key

    unsigned char tk_bits[ALL_KEY_BUF_SIZE];  //total key
    unsigned char nk_bits[ALL_KEY_BUF_SIZE];  //all_key
    unsigned char ak_bits[ALL_KEY_BUF_SIZE];  //all_key

    unsigned char press_cnt;
    unsigned char cnt;
    unsigned char ck; // consume_key
    unsigned char sk;  //system_key

    unsigned int special_key_press_f;

    unsigned char keycode[MAX_BTN_CNT];

}app_kb_data_t;

extern app_kb_data_t app_key_buf;
extern app_kb_data_t key_buf_24g;

extern unsigned char    fn_flag;
extern struct gpio_dt_spec toggle_pin;

extern volatile uint32_t user_active_disconnect;
extern volatile unsigned char fun_mode;
extern struct gpio_dt_spec vbus_check_pin;
void key_fifo(unsigned char key_code);
unsigned char proc_hotkey(unsigned char key_code);
unsigned char special_key_press_flag_set(unsigned char key_code);
static int peripheral_comm_init(void);
void special_key_event_handle(void);
int keyboard_comm_init(void);
void app_ble_report_to_client(void);
void public_loop(void);
void k_timer_scan_loop_init(void);
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
void save_dev_info(void);
_attribute_ram_code_ void kb_led_out(uint8_t status);
_attribute_ram_code_ void reset_idle_status(void);
_attribute_ram_code_ void idle_status_poll(void);
_attribute_ram_code_ void adv_count_poll(void);
extern _attribute_ram_code_ void rf_irq_handler(const void *param);
extern _attribute_ram_code_ void stimer_irq_handler(const void *param);


#ifdef __cplusplus
}
#endif
