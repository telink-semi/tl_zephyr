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


extern my_fifo_t	fifo_km;
extern u8 device_channel;
extern volatile unsigned int rf_state;
extern u8 d24g_ota_status;
extern u8 deep_flag;


typedef enum
{
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

#endif // __APP_2P4G_H__