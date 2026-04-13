/** @file
 *  @brief app_24g.h
 */

/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __APP_2P4G_H__
#define __APP_2P4G_H__

#define DEVICE_TYPE_INDEX 2 /* 1:mouse, 2:keyboard */

extern uint8_t device_channel;
extern volatile unsigned int rf_state;
extern uint8_t d24g_ota_status;
extern uint8_t deep_flag;
extern uint8_t has_new_key_event;

#define MAX_BTN_CNT 6
typedef struct {
	uint8_t cnt;
	uint8_t keycode[MAX_BTN_CNT];
	uint8_t press_cnt;
	uint32_t special_key_press_f;
} keyboard_data_t;
extern keyboard_data_t key_buf;

typedef enum {
	STATE_POWERON = 0,
	STATE_PAIRING,
	STATE_NORMAL,
	STATE_OTA,
} DEVICE_STATE;

void d24_user_init(void);

void d24_main_loop(void);

void d24g_ota_loop(void);

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __cplusplus
}
#endif

#endif /* __APP_2P4G_H__ */
