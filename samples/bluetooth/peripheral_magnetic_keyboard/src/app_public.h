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
#include "tl_fifo.h"
#include "tl_string.h"
#include "driver.h"

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

typedef fifo_cb_t kb_cb_t;
extern pl_fifo_t tx_fifo;
extern pl_fifo_t   d25fKbTxFifo;
extern pl_fifo_t   d25fSppTxFifo;

extern volatile unsigned char fun_mode;

static inline kb_mode_t  app_get_mode(void)
{
    #if (DBG_WITH_EVK_EN)
    return APP_D24G_MODE;
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
