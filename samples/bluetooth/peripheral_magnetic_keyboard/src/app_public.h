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
#include "app_d24g.h"
#include "app_usb.h"
#include "app_fifo.h"
#include "driver.h"


extern struct nvs_fs user_fs;
extern const struct uart_config uart_cfg;
extern const struct device *const uart_dev;
extern const struct gpio_dt_spec device_status_led_pin;

///////////////////////////////////////////////////////////////////////////////////////////
#define PRESS_T_FN_FLAG      0X01
#define PRESS_KB_M_FLAG      0X02
#define PRESS_KB_P_FLAG      0X04
#define PRESS_KB_R_FLAG      0X08
#define PRESS_KB_1_FLAG      0X10
#define PRESS_KB_2_FLAG      0X20
#define PRESS_KB_3_FLAG      0X40
#define PRESS_KB_4_FLAG      0X80
#define PRESS_KB_5_FLAG      0X0100
#define PRESS_KB_6_FLAG      0X0200
#define PRESS_KB_7_FLAG      0X0400


#define BIT_SET(x, n)               ((x) |= BIT(n))
#define BIT_CLR(x, n)               ((x) &= ~BIT(n))
#define BIT_IS_SET(x, n)            ((x) & BIT(n))
#define BIT_FLIP(x, n)              ((x) ^= BIT(n))
#define BIT_SET_HIGH(x)             ((x) |= BIT((sizeof((x)) * 8 - 1)))  // set the highest bit
#define BIT_CLR_HIGH(x)             ((x) &= ~BIT((sizeof((x)) * 8 - 1))) // clr the highest bit
#define BIT_IS_SET_HIGH(x)          ((x) & BIT((sizeof((x)) * 8 - 1)))   // check the highest bit




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

#define USER_STORAGE_APP_INFO_ID                1
#define APP_2P4G_PAIR_INFO_ID                   2
#define APP_2P4G_APP_INFO_ID                    3

typedef struct
{
    uint32_t side_id; //4

    // unsigned char key[12];//12

    uint8_t peer_addr[MAC_ADDR_LEN];//6

    // unsigned char mode;
    // unsigned char mast_id;
    // unsigned short temp1; //24

    // unsigned char  temp2[8]; //32

} ST_FLASH_DEV_INFO;
extern ST_FLASH_DEV_INFO flash_dev_info;
extern int dev_info_idx;
// extern uint32_t  flash_sector_2p4_inf;

typedef struct
{
    // uint32_t side_id; //4
    uint8_t report_rate; //1

} ST_FLASH_DEV_OTHER_INFO;

extern ST_FLASH_DEV_OTHER_INFO flash_dev_other_info;
extern int dev_other_info_idx;
// extern uint32_t  flash_sector_2p4_other_inf;


typedef fifo_cb_t spp_cb_t;
typedef fifo_cb_t kb_cb_t;
extern pl_fifo_t   d25fKbTxFifo;
extern pl_fifo_t   d25fSppTxFifo;

extern volatile unsigned char fun_mode;

static inline kb_mode_t  app_get_mode(void)
{
    #if (DBG_WITH_EVK_EN)
    return KB_MODE_2P4G;
    #else
    return fun_mode;
    #endif
}

void check_vbus(void);

void user_timer_init(void);


void keyboard_comm_init(void);


void public_loop(void);

void special_key_event_handle(void);
unsigned char special_key_press_flag_set(unsigned char key_code);


#ifdef __cplusplus
}
#endif
