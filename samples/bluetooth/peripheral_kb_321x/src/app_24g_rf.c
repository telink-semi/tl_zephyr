/* app_24g.c - Application main entry point */

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

#include "app_24g.h"
#include "app_public.h"

#define LOG_LEVEL LOG_LEVEL_DBG
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(app_24g_rf);


#define RX_FIFO_NUM         4
#define RX_FIFO_DEP         128
#define RX_FRAME_SIZE       (RX_FIFO_NUM * RX_FIFO_DEP)

_attribute_data_retention_ volatile unsigned char rx_packet[RX_FRAME_SIZE]  __attribute__((aligned(4)));


//-------------------- rf init------------------
#define FRE_OFFSET 	0
#define MAX_RF_CHANNEL  16
const unsigned char rf_chn[MAX_RF_CHANNEL] =
{
    FRE_OFFSET + 5, FRE_OFFSET + 9, FRE_OFFSET + 13, FRE_OFFSET + 17,
    FRE_OFFSET + 22, FRE_OFFSET + 26, FRE_OFFSET + 30, FRE_OFFSET + 35,
    FRE_OFFSET + 40, FRE_OFFSET + 45, FRE_OFFSET + 50, FRE_OFFSET + 55,
    FRE_OFFSET + 60, FRE_OFFSET + 65, FRE_OFFSET + 70, FRE_OFFSET + 76,
};

_attribute_aligned_(4)  u8 buf_rf_rxfifo[64*2];
_attribute_aligned_(4)  pl_fifo_t  rf_rxfifo =
{
	.size = 64,
	.num = 2,
	.wptr = 0,
	.rptr = 0,
	.p = buf_rf_rxfifo,
};


#define EMPTY_CNT     500

extern _attribute_data_retention_sec_ rf_fast_settle_t *g_fast_settle_cal_val_ptr;
void		rf_fast_settle_get_val(rf_tx_fast_settle_time_e tx_settle_us, rf_rx_fast_settle_time_e rx_settle_us, rf_fast_settle_t *fs_cv);
void		rf_fast_settle_set_val(rf_tx_fast_settle_time_e tx_settle_us, rf_rx_fast_settle_time_e rx_settle_us, rf_fast_settle_t *fs_cv);


_attribute_data_retention_sec_ rf_fast_settle_t fs_cv_2m;


/**
 *  @brief      This function is used to get the calibration value of rf tx/rx fast settle
 *  @param[in]  tx_settle_us    After adjusting the timing sequence, the time required for tx to settle.
 *  @param[in]  rx_settle_us    After adjusting the timing sequence, the time required for rx to settle.
 *  @param[in]  fs_cv           Fast settle related calibration value storage variable address.
 *  @return     none
 */
void rf_fast_settle_get_val(rf_tx_fast_settle_time_e tx_settle_us, rf_rx_fast_settle_time_e rx_settle_us, rf_fast_settle_t *fs_cv)
{
	u8 ble_tx_packet[6];
	unsigned char rf_data_len = 2;
	ble_tx_packet[4]=0;
	ble_tx_packet[5]=0;
	unsigned int rf_tx_dma_len = rf_tx_packet_dma_len(rf_data_len);
	ble_tx_packet[3] = (rf_tx_dma_len >> 24)&0xff;
	ble_tx_packet[2] = (rf_tx_dma_len >> 16)&0xff;
	ble_tx_packet[1] = (rf_tx_dma_len >> 8)&0xff;
	ble_tx_packet[0] = rf_tx_dma_len&0xff;
	//tx
	
    rf_set_tx_rx_off_auto_mode();      //STOP_RF_STATE_MACHINE;
    rf_clr_irq_status(FLD_RF_IRQ_ALL); //CLEAR_ALL_RFIRQ_STATUS;

    rf_set_tx_settle_time(115);        //adjust TX settle time
    rf_set_tx_dma(2, 128);

    for (unsigned char chn = 0; chn <= 80; chn++) {
        rf_set_chn(chn);
        rf_start_stx(ble_tx_packet, rf_stimer_get_tick());

        while (!(rf_get_irq_status(FLD_RF_IRQ_TX)))
            ;
        rf_clr_irq_status(FLD_RF_IRQ_TX);
        rf_tx_fast_settle_get_cal_val(tx_settle_us, chn, fs_cv);

        rf_set_tx_rx_off_auto_mode(); //STOP_RF_STATE_MACHINE;
        rf_clr_irq_status(FLD_RF_IRQ_ALL);
    }

    //rx
    rf_set_rx_settle_time(85); //adjust RX settle time

    #if 0//defined(MCU_CORE_B91) || defined(MCU_CORE_B92)
    rf_start_srx(rf_stimer_get_tick());
    delay_us(85); //Wait for the rx packetization action to complete
    rf_rx_fast_settle_get_cal_val(rx_settle_us, 0, fs_cv);
    while (rf_receiving_flag())
        ;
    rf_set_tx_rx_off_auto_mode(); //STOP_RF_STATE_MACHINE;
    rf_clr_irq_status(FLD_RF_IRQ_ALL);
    #elif 1//defined(MCU_CORE_TL721X) || defined(MCU_CORE_TL321X)
    rf_set_tx_rx_off_auto_mode(); //STOP_RF_STATE_MACHINE;
    rf_clr_irq_status(FLD_RF_IRQ_ALL);
    for (unsigned char chn = 4; chn <= 80; chn += 10) {
        rf_set_chn(chn);
        rf_start_srx(rf_stimer_get_tick());
        delay_us(85); //Wait for the rx packetization action to complete

        rf_rx_fast_settle_get_cal_val(rx_settle_us, chn, fs_cv);
        while (rf_receiving_flag())
            ;
        rf_set_tx_rx_off_auto_mode(); //STOP_RF_STATE_MACHINE;
        rf_clr_irq_status(FLD_RF_IRQ_ALL);
    }
    #endif
}


/**
 *  @brief      This function is used to set the calibration value of rf tx/rx fast settle
 *  @param[in]  tx_settle_us    After adjusting the timing sequence, the time required for tx to settle.
 *  @param[in]  rx_settle_us    After adjusting the timing sequence, the time required for rx to settle.
 *  @param[in]  fs_cv           Fast settle related calibration value storage variable address.
 *  @return     none
 */
void rf_fast_settle_set_val(rf_tx_fast_settle_time_e tx_settle_us, rf_rx_fast_settle_time_e rx_settle_us, rf_fast_settle_t *fs_cv)
{
        g_fast_settle_cal_val_ptr = fs_cv;
    #if 0//defined(MCU_CORE_B91) || defined(MCU_CORE_B92)
        for (unsigned char chn = 4; chn <= 80; chn += 10) {
            rf_tx_fast_settle_set_cal_val(tx_settle_us, chn, fs_cv);
        }
        rf_rx_fast_settle_set_cal_val(rx_settle_us, 0, fs_cv);
    #elif 1//defined(MCU_CORE_TL721X) || defined(MCU_CORE_TL321X)
        for (unsigned char chn = 4; chn <= 80; chn += 10) {
            rf_tx_fast_settle_set_cal_val(tx_settle_us, chn, fs_cv);
            rf_rx_fast_settle_set_cal_val(rx_settle_us, chn, fs_cv);
        }
    #endif
}


/**
 *  @brief      This function is used to set rf tx/rx fast settle
 *  @param[in]  tx_settle_us    After adjusting the timing sequence, the time required for tx to settle.
 *  @param[in]  rx_settle_us    After adjusting the timing sequence, the time required for rx to settle..
 *  @return     none
*/
void rf_fast_settle_setup(rf_tx_fast_settle_time_e tx_settle_us, rf_rx_fast_settle_time_e rx_settle_us)
{
	u8 ble_tx_packet[6];
	unsigned char rf_data_len = 2;
	ble_tx_packet[4]=0;
	ble_tx_packet[5]=0;
	unsigned int rf_tx_dma_len = rf_tx_packet_dma_len(rf_data_len);
	ble_tx_packet[3] = (rf_tx_dma_len >> 24)&0xff;
	ble_tx_packet[2] = (rf_tx_dma_len >> 16)&0xff;
	ble_tx_packet[1] = (rf_tx_dma_len >> 8)&0xff;
	ble_tx_packet[0] = rf_tx_dma_len&0xff;
	//tx
	rf_set_tx_rx_off_auto_mode();//STOP_RF_STATE_MACHINE;
	rf_clr_irq_status(FLD_RF_IRQ_ALL);//CLEAR_ALL_RFIRQ_STATUS;

	rf_set_tx_settle_time(115);//adjust TX settle time
	rf_set_tx_dma(2,128);

	for(unsigned char chn=0;chn<=80;chn++)
	{
		rf_set_chn(chn);
		rf_start_stx(ble_tx_packet,rf_stimer_get_tick());
		delay_us(115);//Wait for the tx packetization action to complete, ensuring that the settle time on each channel

		rf_tx_fast_settle_update_cal_val(tx_settle_us,chn);

		rf_set_tx_rx_off_auto_mode();//STOP_RF_STATE_MACHINE;
		rf_clr_irq_status(FLD_RF_IRQ_ALL);
	}

	//rx
	rf_set_rx_settle_time(85);//adjust RX settle time
	rf_start_srx(rf_stimer_get_tick());
	delay_us(85);//Wait for the rx packetization action to complete

#if (MCU_CORE_TYPE == MCU_CORE_B91)

	rf_rx_fast_settle_update_cal_val(rx_settle_us,0);
	rf_set_tx_rx_off_auto_mode();//STOP_RF_STATE_MACHINE;
	rf_clr_irq_status(FLD_RF_IRQ_ALL);
#elif ((MCU_CORE_TYPE == MCU_CORE_TL721X)||(MCU_CORE_TYPE == MCU_CORE_TL321X))
	rf_set_tx_rx_off_auto_mode();//STOP_RF_STATE_MACHINE;
	rf_clr_irq_status(FLD_RF_IRQ_ALL);
	for(unsigned char chn=4;chn<=80;chn+=10)
	{
		rf_set_chn(chn);
		rf_start_srx(rf_stimer_get_tick());
		delay_us(85);//Wait for the rx packetization action to complete

		rf_rx_fast_settle_update_cal_val(rx_settle_us,chn);

		rf_set_tx_rx_off_auto_mode();//STOP_RF_STATE_MACHINE;
		rf_clr_irq_status(FLD_RF_IRQ_ALL);
	}

#endif
}

/**
 * @brief       This function set to init rf drv private 2m
 * @return      
 * @note        
 */
_attribute_ram_code_sec_ void pp_rf_init(u8 init_fastsettle)
{
    rf_mode_init();
    rf_set_pri_2M_mode();

    pm_set_suspend_power_cfg(FLD_PD_ZB_EN, 1);

    //rf_set_rx_dma(rf_rxfifo.p, rf_rxfifo.num - 1, rf_rxfifo.size);
    rf_set_rx_dma(rx_packet, RX_FIFO_NUM-1, RX_FIFO_DEP);
    //rf_set_rx_maxlen(rf_rxfifo.size - 4);
    rf_set_tx_dma(2, 128);

    rf_set_preamble_len(4);
    rf_set_access_code_len(5);
    reg_rf_modem_sync_thres_ble = 38;


    if (init_fastsettle) {
        rf_fast_settle_get_val(TX_SETTLE_TIME_23US, RX_SETTLE_TIME_15US, &fs_cv_2m);
        rf_fast_settle_set_val(TX_SETTLE_TIME_23US, RX_SETTLE_TIME_15US, &fs_cv_2m);
        if(-1 == rf_fast_settle_config(TX_SETTLE_TIME_23US,RX_SETTLE_TIME_15US))
        {
            //Incorrect configuration.
        }
        rf_tx_fast_settle_en();
        rf_rx_fast_settle_en();
        rf_set_tx_settle_time(23);
        rf_set_rx_settle_time(15);
    }


    plic_interrupt_enable(IRQ_ZB_RT);
    rf_set_irq_mask(FLD_RF_IRQ_TX|FLD_RF_IRQ_RX|FLD_RF_IRQ_RX_TIMEOUT|FLD_RF_IRQ_FIRST_TIMEOUT);
}


/**
 * @brief       This function set pair access code
 * @param[in]   code	- access code
 * @return      
 * @note        
 */
void set_pair_access_code(u32 code)
{
	//rf_access_code_comm(code);

    reg_rf_access_code =  ((code & 0xffffff00) | 0x71);
    reg_rf_access_4 = code;
    //The following two lines of code are for trigger access code in S2,S8 mode.It has no effect on other modes.
    reg_rf_modem_mode_cfg_rx1_0 &= ~FLD_RF_LR_TRIG_MODE;
    write_reg8(0x170425, read_reg8(0x170425) | 0x01);
}

/**
 * @brief       This function set data access code
 * @param[in]   code	- access code
 * @return      
 * @note        
 */
void set_data_access_code(u32 code)
{
	//rf_access_code_comm(code);
    reg_rf_access_code =  ((code & 0xffffff00) | 0x77);
    reg_rf_access_4 = code;
    //The following two lines of code are for trigger access code in S2,S8 mode.It has no effect on other modes.
    reg_rf_modem_mode_cfg_rx1_0 &= ~FLD_RF_LR_TRIG_MODE;
    write_reg8(0x170425, read_reg8(0x170425) | 0x01);
}

/**
*  @brief 	 rf start rx
*  @param[in]  tick  rx start tick time  
*  @param[in]  timeout_us  rx timeout
*  @return	none
*/

_attribute_ram_code_sec_ void pp_rf_start_srx(unsigned int tick, u32 timeout_us)
{
	reg_rf_ll_rx_fst_timeout = timeout_us;					// first timeout.
	reg_rf_ll_cmd_schedule = tick;
	reg_rf_ll_ctrl3 |= FLD_RF_R_CMD_SCHEDULE_EN;	// Enable cmd_schedule mode.
	reg_rf_ll_cmd = 0x86;
}


/**
 * @brief       This function deal 2m private irq
 * @return      
 * @note        
 */
_attribute_ram_code_sec_ void pp_rf_irq_handler()
{
    if(rf_get_irq_status(FLD_RF_IRQ_RX))
    {
        rf_state = RF_RX_END_STATUS;
        RF_RX_GPIO_DEBUG_SET(0);
		rf_clr_irq_status(FLD_RF_IRQ_RX|FLD_RF_IRQ_CMD_DONE);
    }

    if(rf_get_irq_status(FLD_RF_IRQ_TX))
    {
        //pp_rf_start_srx(rf_stimer_get_tick() + DELAY_RX_AFTER_TX_US * RF_SYSTEM_TIMER_TICK_1US, 300);
        RF_TX_GPIO_DEBUG_SET(0);
        RF_RX_GPIO_DEBUG_SET(1);
        RF_RX_TIMEOUT_GPIO_DEBUG_SET(1);
        rf_state = RF_RX_START_STATUS;
        //tlkapi_send_string_data(APP_LOG_EN, "TV:", 0, 0);
        rf_clr_irq_status(FLD_RF_IRQ_TX|FLD_RF_IRQ_CMD_DONE);
    }

	if(rf_get_irq_status(FLD_RF_IRQ_RX_TIMEOUT))
	{
        rf_state = RF_RX_TIMEOUT_STATUS;
        //tlkapi_send_string_data(APP_LOG_EN, "T:", 0, 0);
        rf_clr_irq_status(FLD_RF_IRQ_RX_TIMEOUT);
	}
	else
	{
		rf_clr_irq_status(0xffff);
	}

}


/**
 * @brief       This function get next channel
 * @param[in]   chn	- 
 * @param[in]   mask	- 
 * @return      
 * @note        
 */
_attribute_ram_code_sec_  u8 get_next_channel_with_mask(u32 mask, u8 chn)
{
	return (chn+3)%15;
}

/**
 * @brief       irq_device_rx
 * @return      
 * @note        
 */
_attribute_ram_code_ void irq_device_rx(void)
{
    unsigned char* raw_pkt = rf_get_rx_packet_addr(RX_FIFO_NUM, RX_FIFO_DEP, rx_packet);
    if(rf_pri_tpll_packet_crc_ok(raw_pkt)) {
        rf_packet_t *p = (rf_packet_t *)(raw_pkt);
        //tlkapi_send_string_data(APP_LOG_EN, "CRC ok:", 0, 0);
        if (rf_rx_process(p))
        {
            device_ack_received = 1;
        }
    }
    raw_pkt[0] = 1;//must
}

_attribute_ram_code_ void app_set_rf_power(rf_power_level_index_e idx)
{
    static u8 last_power = 0xff;

    if(last_power != idx) {
        rf_set_power_level_index(idx);
        last_power = idx;
    }
}

_attribute_ram_code_ void app_rf_set_timeout(unsigned short timeout_us)
{
    static int last_timeout = 0;
    if (last_timeout != timeout_us) {
        rf_set_rx_timeout(timeout_us);
        last_timeout = timeout_us;
    }
}


_attribute_ram_code_ void app_rf_set_chn(signed char chn)
{
    static signed char last_chn = 0;
    if (last_chn != chn) {
        rf_set_chn(chn);
        last_chn = chn;
    }
}

