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

#define rf_stimer_get_tick() stimer_get_tick()
#define RF_SYSTEM_TIMER_TICK_1US   SYSTEM_TIMER_TICK_1US

#define RF_CRC_LENGTH   3
#define pp_rf_packet_crc_ok(p)            ((p[(p[5]+13+RF_CRC_LENGTH)] & 0x01) == 0x0)


extern int device_ack_received;
extern const unsigned char rf_chn[];


typedef enum 
{
	RF_IDLE_STATUS =	0,
    RF_TX_START_STATUS=1,
    RF_TX_END_STATUS=2,
    RF_RX_START_STATUS=3,
    RF_RX_END_STATUS=4,
	RF_RX_TIMEOUT_STATUS=5,

}APP_RF_STATUS_E;
extern volatile unsigned int rf_status;

enum
{
    EMPTY_CMD = 0x00,
    EMPTY_ACK_CMD = 1,

    PAIR_CMD=2,//pair cmd
    PAIR_ACK_CMD=3,//pair ack cmd

	MOUSE_CMD=4,//mouse cmd
	MOUSE_ACK_CMD=5,//mouse ack cmd

	KB_CMD=6,//kb cmd
	KB_ACK_CMD=7,//kb ack cmd

	RECONNECT_CMD=8,//reconnect cmd
	RECONNECT_ACK_CMD=9,//reconnect ack cmd

	D24G_OTA_CMD = 10, //ota cmd
	D24G_OTA_ACK_CMD = 11, //ota ack cmd
};


typedef struct{
    u32 dma_len;//dma len

    u8 rf_len; //rf len
    u8  dat[59];
} rf_packet_t;

typedef struct
{	
	u8 cmd;//data type
	u8 seq_no;
	u8 pno_no;
	u32 did;//device id
	u8 key[12]; //key
} pair_data_t;//pair data struct

typedef struct
{	
	u8 cmd;//data type
	u8 seq_no;
	u8 pno_no;
	
	u8 tick_0;
	u8 tick_1;
	u8 chn;
	u8 host_led_status;//host led status
	
	u32 gid;	//dongle ID
	u32 did;	//Device ID
	u8 key[12]; //key

} pair_ack_data_t;	//Paired ACK packet


typedef struct
{
	u8	cmd;//bit7=0: no aes  =1: aes
	u8	seq_no;	//The frame serial number
	u8	pn_no; 	//

	u32 did;	//Device ID

	u8  km_dat[6];//mouse data or kb
	u8  rsv1[3]; //for aes  16 bytes

	u16 crc16;	//Software CRC16 

} km_data_t;	//Communication packet

typedef struct
{
	
	u8 cmd;//data type
	u8 seq_no;	//The frame serial number
	u8 pno_no;
	
	u8 tick_0;
	u8 tick_1;
	u8 chn;//chanel
	u8 host_led_status;//host led status
	
} km_ack_data_t;//km ack data struct

typedef struct
{
	u8	cmd;//bit7=0: no aes  =1: aes
	u8	seq_no;
	u8  pno_no;
	u32 did;

	u8	report_id;
	u8 	opcode;
	u16	length;	
	u8 dat[20]; //include:u16 package_cnt; u8 data[16]; u16 crc16;
} ota_data_t;

typedef struct
{
	u8	cmd;//bit7=0: no aes  =1: aes
	u8	seq_no;
	u8  pno_no;
	u32 did;

	u8	report_id;
	u8 	opcode;
	u16	length;	
	u8 dat[20]; //include:u16 package_cnt; u8 data[16]; u16 crc16;
} ota_ack_data_t;

typedef struct
{
	u16 cmd;
	u8 buf[16];
	u16 crc;
}ota_data_st;


typedef enum
{
	EMPTY_DATA_CMD=0,
	PAIR_DATA_CMD=1,
	RECONNECT_DATA_CMD=2,
	MOUSE_DATA=3,
	SPP_DATA=4,
	SPP_DATA_ACK=5,
	//TEST_DATA_CMD=0XFE,
	ERROR_DATA=0X55,
	
}M_RF_CMD_ENUM;

typedef enum
{
	RP_125,
	RP_250,
	RP_500,
	RP_1000,
	RP_2000,
	RP_4000,
	RP_8000,	
}REPORT_RATE;



typedef struct{
	u8 map[5];
	u8 table[37];
	u8  hop;
	s8 	idx;
} rf_channel_param_t;



typedef struct
{
	u32 side_id;
	
	u8 dev_now_status;
	u8 pair_success_flag;
	u8 rsv[2];
} app_async_st;
extern app_async_st  app_inf;

typedef	struct {
	u16		size;
	u16		num;
	
	u16		wptr;
	u16		rptr;
	
	u8*		p;
}	pl_fifo_t;


/**
*  @brief 	 Passing spp data (not mouse) to the underlying protocol layer
*  @param[in]  cmd - data command
*  @param[in]  buf -  data buffer
*  @param[in]  length -  data length

*  @return	0:success  ,other :fail
*/

int pp_notify_spp(u8 cmd,u8 *buf,int length);
/**
 * @brief   2.4g rf init
 * @param[in]   none
 * @return  none
 */

void pp_rf_init(u8 init_fastsettle);
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
  *  @brief 	 get fifo number
  *  @param[in]  none
  *  @return	 fifo  number
 */

u32 pp_get_rf_tx_fifo_num(void);


_attribute_ram_code_sec_ u8 get_next_channel_with_mask(u32 mask, u8 chn);

void set_pair_access_code(u32 code);

/**
 * @brief       This function set data access code
 * @param[in]   code	- access code
 * @return      
 * @note        
 */
void set_data_access_code(u32 code);

extern u8 rf_rx_process(rf_packet_t *p_rf_data);

void irq_device_rx(void);

_attribute_ram_code_ void app_set_rf_power(rf_power_level_index_e idx);

_attribute_ram_code_ void app_rf_set_timeout(unsigned short timeout_us);

_attribute_ram_code_ void app_rf_set_chn(signed char chn);

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __cplusplus
}
#endif

#endif // __APP_2P4G_H__