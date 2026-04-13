/** @file
 *  @brief app_24g.h
 */

/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __APP_2P4G_RF_H__
#define __APP_2P4G_RF_H__

#include "rf_common.h"

#define rf_stimer_get_tick()     stimer_get_tick()
#define RF_SYSTEM_TIMER_TICK_1US SYSTEM_TIMER_TICK_1US

#define RF_CRC_LENGTH          3
#define pp_rf_packet_crc_ok(p) ((p[(p[5] + 13 + RF_CRC_LENGTH)] & 0x01) == 0x0)

extern int device_ack_received;
extern const unsigned char rf_chn[];
extern uint8_t device_status;

typedef enum {
	RF_IDLE_STATUS = 0,
	RF_TX_START_STATUS = 1,
	RF_TX_END_STATUS = 2,
	RF_RX_START_STATUS = 3,
	RF_RX_END_STATUS = 4,
	RF_RX_TIMEOUT_STATUS = 5,
} APP_RF_STATUS_E;
extern volatile unsigned int rf_status;

enum {
	EMPTY_CMD = 0x00,
	EMPTY_ACK_CMD = 1,

	PAIR_CMD = 2,     /* pair cmd */
	PAIR_ACK_CMD = 3, /* pair ack cmd */

	MOUSE_CMD = 4,     /* mouse cmd */
	MOUSE_ACK_CMD = 5, /* mouse ack cmd */

	KB_CMD = 6,     /* kb cmd */
	KB_ACK_CMD = 7, /* kb ack cmd */

	RECONNECT_CMD = 8,     /* reconnect cmd */
	RECONNECT_ACK_CMD = 9, /* reconnect ack cmd */

	D24G_OTA_CMD = 10,     /* ota cmd */
	D24G_OTA_ACK_CMD = 11, /* ota ack cmd */
};

typedef struct {
	uint32_t dma_len; /* dma len */

	uint8_t rf_len; /* rf len */
	uint8_t dat[59];
} rf_packet_t;

typedef struct {
	uint8_t cmd; /* data type */
	uint8_t seq_no;
	uint8_t pno_no;
	uint8_t resv;
	uint32_t did;    /* device id */
	uint8_t key[12]; /* key */
} pair_data_t;           /* pair data struct */

typedef struct {
	uint8_t cmd; /* data type */
	uint8_t seq_no;
	uint8_t pno_no;
	uint8_t resv;
	uint8_t tick_0;
	uint8_t tick_1;
	uint8_t chn;
	uint8_t host_led_status; /* host led status */

	uint32_t gid;    /* dongle ID */
	uint32_t did;    /* Device ID */
	uint8_t key[12]; /* key */

} pair_ack_data_t; /* Paired ACK packet */

typedef struct {
	uint8_t cmd;    /* bit7=0: no aes  =1: aes */
	uint8_t seq_no; /* The frame serial number */
	uint8_t pn_no;
	uint8_t key_type;
	uint32_t did;      /* Device ID */
	uint8_t km_dat[8]; /* mouse data or kb */

	uint16_t crc16; /* Software CRC16 */

} km_data_t; /* Communication packet */

typedef struct {

	uint8_t cmd;    /* data type */
	uint8_t seq_no; /* The frame serial number */
	uint8_t pno_no;
	uint8_t tick_0;
	uint8_t tick_1;
	uint8_t chn;             /* chanel */
	uint8_t host_led_status; /* host led status */
	uint8_t resv;
} km_ack_data_t; /* km ack data struct */

typedef enum {
	RP_125,
	RP_250,
	RP_500,
	RP_1000,
	RP_2000,
	RP_4000,
	RP_8000,
} REPORT_RATE;

typedef struct {
	uint8_t map[5];
	uint8_t table[37];
	uint8_t hop;
	signed char idx;
} rf_channel_param_t;

typedef struct {
	uint32_t side_id;

	uint8_t dev_now_status;
	uint8_t pair_success_flag;
	uint8_t rsv[2];
} app_async_st;
extern app_async_st app_inf;

/**
*  @brief	 Passing spp data (not mouse) to the underlying protocol layer
*  @param[in]  cmd - data command
*  @param[in]  buf -  data buffer
*  @param[in]  length -  data length

*  @return	0:success  ,other :fail
*/

int pp_notify_spp(uint8_t cmd, uint8_t *buf, int length);
/**
 * @brief   2.4g rf init
 * @param[in]   none
 * @return  none
 */

void pp_rf_init(uint8_t init_fastsettle);
/**
 * @brief   irq_handler for 2.4gE stack and RF interrupt
 * @param[in]   none
 * @return  none
 */

void pp_rf_irq_handler(void);
/**
 * @brief   time0 interrupt  for 2.4g   stack
 * @param[in]   none
 * @return  none
 */

void pp_timer0_irq_handler(void);

/**
 * @brief   2.4g stack main loop
 * @param[in]   v  latency value
 * @return  none
 */

void pp_sdk_main_loop(void);

/**
 * @brief   set_cconnect_timeout
 * @param[in]   timeout_ms   unit is 1 ms
 * @return  none
 */

void pp_set_cconnect_timeout(int timeoout_ms);

/**
 * @brief   time0 init  for 2.4g   stack
 * @param[in]   none
 * @return  none
 */

void pp_timer0_init(void);

/**
 *  @brief   get fifo number
 *  @param[in]  none
 *  @return  fifo  number
 */

uint32_t pp_get_rf_tx_fifo_num(void);

_attribute_ram_code_sec_ uint8_t get_next_channel_with_mask(uint32_t mask, uint8_t chn);

void set_pair_access_code(uint32_t code);

/**
 * @brief       This function set data access code
 * @param[in]   code	- access code
 * @return
 * @note
 */
void set_data_access_code(uint32_t code);

extern uint8_t rf_rx_process(rf_packet_t *p_rf_data);

void irq_device_rx(void);

extern _attribute_ram_code_ void app_set_rf_power(rf_power_level_index_e idx);

extern _attribute_ram_code_ void app_rf_set_timeout(unsigned short timeout_us);

extern _attribute_ram_code_ void app_rf_set_chn(signed char chn);

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __cplusplus
}
#endif

#endif /* __APP_2P4G_H__  */
