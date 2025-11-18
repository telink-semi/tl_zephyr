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

#include "app_public.h"
#include "drivers.h"

#define LOG_LEVEL LOG_LEVEL_DBG
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(app_24g);



#define  SCAN_INTERVAL_TIME         8//6

#define PAIR_ACCESS_CODE              0x39517695

volatile unsigned int rf_state;
int device_ack_received;
uint8_t device_channel;

uint8_t pub_key[16]={0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F,0x10};

uint8_t device_status = 0;//device status init0
uint32_t dongle_id;//did
volatile uint16_t no_ack = 0;//no ack init 0
uint8_t keyboard_send_need_f = 0;//init send kb f 0
uint8_t need_suspend_flag = 0; //need suspend flag init 0

uint8_t private_key[16] =
{
    0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10
};

uint32_t tick_loop=0;//tick loop init 0
uint32_t wakeup_next_tick=0;//next wake up tick init 0
uint8_t count=0;//count 0
uint8_t pair_success_flag=0;//pair success init 0
uint8_t dongle_id_need_save_flag = 0; //0:no need save, 1:need save to flash


uint8_t keyboard_buf[BUF_SIZE_KEYBOARD] = {0};
uint8_t keyboard_buf_last[BUF_SIZE_KEYBOARD];
uint8_t has_new_report;
uint8_t active_disconnect_reason = 0;

keyboard_data_t key_buf;

uint8_t kb_led_status;
uint8_t connect_ok = 0;

volatile unsigned int rf_rx_timeout_us;//rx timeout 
volatile uint32_t start_rf_tick = 0; //start rf tick

#define  PAIR_RF_LEN   19
#define  KM_RF_LEN     13
volatile rf_packet_t rf_pair_buf =
{
    rf_tx_packet_dma_len(PAIR_RF_LEN + 1),	// dma_len
    19,	// rf_len
};
pair_data_t *p_pair_dat=(pair_data_t*)&rf_pair_buf.dat[0];//point to pair packet

volatile rf_packet_t rf_km_buf =
{
    rf_tx_packet_dma_len(KM_RF_LEN + 1),	// dma_len
    13, // rf_len
 };

km_data_t *p_km_data = (km_data_t*)&rf_km_buf.dat[0];//point to km data packet

unsigned short 	crc16_poly1[2] = {0, 0xa001};
/**
 * @brief       This function crs 16
 * @param[in]   len	- data len
 * @param[in]   pD	- data
 * @return     crc 
 * @note        
 */
_attribute_ram_code_sec_ unsigned short crc16_user (unsigned char *pD, int len)
{
	unsigned short crc = 0xffff;//crc16 init ffff
    int i,j;

	for(j=len; j>0; j--)//for every data
    {
		unsigned char ds = *pD++;//pointer pd
        for(i=0; i<8; i++)
        {
			crc = (crc >> 1) ^ crc16_poly1[(crc ^ ds ) & 1];//deal every bit
			ds = ds >> 1;//right move 1 bit
        }
    }

	return crc;//return crc
}


/**
 * @brief       This function led out kb 
 * @param[in]   status	- 
 * @return      
 * @note        
 */
_attribute_ram_code_sec_ void kb_led_out_aaa(uint8_t status)
{
#if HAS_NUM_LED //num led en
	led_num_set((status&0x01)?LED_ON:LED_OFF);//set num led 
#endif

#if HAS_CAPS_LED//caps led en
	led_caps_set((status&0x02)?LED_ON:LED_OFF);//set caps led
#endif

#if HAS_SCROLL_LED//scroll led en
	led_scroll_set((status&0x04)?LED_ON:LED_OFF);//set scroll led
#endif
}

/**
 * @brief       This function deal rx packet
 * @param[in]   p_rf_data	- rx packet
 * @return      
 * @note        
 */
_attribute_ram_code_sec_ uint8_t  rf_rx_process(rf_packet_t *p_rf_data)
{
	if(device_status == STATE_PAIRING) //pair status
	{
		pair_ack_data_t *pair_ack_dat_ptr=(pair_ack_data_t*)&p_rf_data->dat[0];

        if (((pair_ack_dat_ptr->cmd == PAIR_ACK_CMD) || (pair_ack_dat_ptr->cmd == RECONNECT_ACK_CMD)) && (pair_ack_dat_ptr->did == p_pair_dat->did))//ack and same did
		{
        	dongle_id = pair_ack_dat_ptr->gid;//dongle id update
        	pair_success_flag = 1;//pair success flag set 1
			if (pair_ack_dat_ptr->cmd == PAIR_ACK_CMD)
			{ //Dongle ACK the pairing command
				dongle_id_need_save_flag = 1; //save dongle_id
			}
			else
			{ //Dongle ACK the reconnect command
				dongle_id_need_save_flag = 0; //not save dongle_id
			}
			printk("pairing success--------- %x\n", pair_ack_dat_ptr->gid);//debug gid
        	return 1;
    	}
	}
	else if(device_status == STATE_NORMAL)//normal status
	{
		km_ack_data_t *km_ack_dat_ptr=(km_ack_data_t*)&p_rf_data->dat[0];
		if((keyboard_send_need_f == 2))//mouse ack
		{
			if(km_ack_dat_ptr->cmd == KB_ACK_CMD)// km ack
			{
				kb_led_status = km_ack_dat_ptr->host_led_status;//read led status
				kb_led_out(kb_led_status);//led set status
				return 1;
			}
		}

		return 1;
	}

    return 0;
}



/**
 * @brief       This function start pair
 * @return      
 * @note        
 */
void d24_start_pair(void)
{
    printk("d24_start_pair\r\n");
    set_pair_flag();//set pair flag
#if REBOOT_WHEN_SWITCH_MODE_AND_CHANNEL_ENABLE
    //user_reboot(CLEAR_FLAG_ANA);
    sys_reboot(SYS_REBOOT_COLD);
#else
    //write_deep_ana1(CLEAR_FLAG_ANA); //write reason to analog register
    device_status = STATE_PAIRING;
    rf_rx_timeout_us = D24G_PAIR_TIMER_OUT;

    p_pair_dat->cmd = PAIR_CMD; //Load PAIR_CMD in pairing packets
#endif
	adv_count = 0; //adv count 0 
	adv_begin_tick = clock_time()|1; //adv begin tick
}

/**
 * @brief       This function init 
 * @return      
 * @note        
 */
void d24_user_init()
{
    uint32_t dev_mac;

    printk("d24_user_init\n");//debug 24g user init

    // TODO:get mac address from flash
    flash_read_page(flash_sector_mac_address, 4, (uint8_t *)&dev_mac);

    uint32_t device_id = ((dev_mac<<8)|DEVICE_TYPE_INDEX);//did set

	p_pair_dat->cmd = PAIR_CMD; //pair cmd
	p_pair_dat->did = device_id; //did
	p_km_data->did = device_id; //did

    p_km_data->cmd = KB_CMD;
    
	printk("device_id %x\n", device_id);
    dongle_id = flash_dev_info.dongle_id; //dongleid
	printk("dongle_id %x\n", flash_dev_info.dongle_id);

    memcpy((uint8_t*)&private_key[0], (uint8_t*)&device_id ,4);

#if  ENTER_PAIR_WHEN_NEVER_PAIRED_ENABLE //pair flag
    if ((dongle_id == uint32_t_MAX) || (dongle_id == 0))
        set_pair_flag(); //set pair
#endif

    if (pair_flag) // pair flag 1
	{
        device_status = STATE_PAIRING; //device into pair status
		rf_rx_timeout_us = D24G_PAIR_TIMER_OUT; //pair time out
        p_pair_dat->did = device_id;
        rf_set_power_level_index(RF_2P4G_POWER_PAIR);
    }
	else
	{
        device_status = STATE_NORMAL;
		rf_rx_timeout_us = D24G_COMMUNICATION_TIMER_OUT;
        rf_set_power_level_index(RF_2P4G_POWER_NORMAL);
    }
    rf_state = RF_IDLE_STATUS;
    wakeup_next_tick = clock_time(); //next wake up tick update
    reset_idle_status();
}

/**
 * @brief       This function check rf complet status
 * @return      
 * @note        
 */
_attribute_ram_code_sec_ void check_rf_complet_status()
{
	rf_state = RF_IDLE_STATUS;
	static uint32_t ack_miss_no = 0; // ack miss no

	if (device_ack_received)
	{
		ack_miss_no = 0; //reset ack miss 0
		no_ack = 0; //reset no ack  0
		start_rf_tick = 0; //reset start rf tick 0

		if(device_status <= STATE_PAIRING) { // pair status
			if(pair_success_flag) { //pair success
				device_status = STATE_NORMAL; // state normal
				rf_rx_timeout_us = D24G_COMMUNICATION_TIMER_OUT; //rx timout
                connect_ok=1; // connect ok 1

        		if (flash_dev_info.dongle_id != dongle_id) {
		            flash_dev_info.dongle_id = dongle_id;

					if (dongle_id_need_save_flag) //need to save dongle_id to flash
					{
						save_dev_info();
					}
					printk("save info flash %x\n", dongle_id);
		        }

				clear_pair_flag();
        		reset_idle_status();
			}
            pp_fifo_reset(&tx_fifo);
		} 
        else if(device_status == STATE_NORMAL)//normal status
		{
			if (keyboard_send_need_f)	// skip to next packet
				pp_fifo_pop(&tx_fifo);//rprt ++
            connect_ok=1;//update connect ok
		}

		keyboard_send_need_f = 0;
	}
	else
	{
		no_ack++;//noack ++
        ack_miss_no ++; //ack miss no ++

		if (ack_miss_no >= 2)
			device_channel = get_next_channel_with_mask(0, device_channel);

		if(no_ack > 125)//no ack>125
		{
			connect_ok = 0;//no conn
			printk("no_ack over 125\n");
		}
	}
}


/**
 * @brief       This function deal rf machine loop
 * @return      
 * @note        
 */
_attribute_ram_code_sec_ void d24g_rf_loop()//rf state machine loop
{
	uint8_t *ptr = 0;

	if (rf_state == RF_IDLE_STATUS)//rf status idle
	{
		if (device_status <= STATE_PAIRING)//device status not normal
		{
			set_pair_access_code(PAIR_ACCESS_CODE);

			pair_success_flag = 0;//pair success flag set to 0
			if (device_status == STATE_PAIRING){
                app_set_rf_power(RF_POWER_P0dBm);
            }

			connect_ok = 0;//no connect
			ptr = (uint8_t *)&rf_pair_buf;//point pair packet
			keyboard_send_need_f = 1;//pairing
			start_rf_tick=0;//reset start rf tick
		}
		else if(device_status == STATE_NORMAL) //status normal
		{
			ptr = (uint8_t *)&rf_km_buf;//normal status pointer to km data packet

			set_data_access_code(flash_dev_info.dongle_id);//dongle id to be data access code
			app_set_rf_power(RF_2P4G_POWER_NORMAL);//set data tx power

			if(keyboard_send_need_f == 0)
			{ 
                uint8_t *p =  pp_fifo_get_ptr(&tx_fifo);//get data
				if (p)//if have data
				{
					start_rf_tick = clock_time()|1;//update rf tick
				    keyboard_send_need_f = 2;
                    uint8_t *tmp = (uint8_t *)&p[0]; //tmp data
                    km_data_t *km_dat1; //km data

                    uint8_t *src = (uint8_t *)&p_km_data->cmd; //pointer to p_km_data
                     km_dat1 = (km_data_t*)&src[0];//kmdat1 pointer p_kmdata too
				    memcpy(&km_dat1->km_dat[0], &tmp[0], 6);//cpy fifo km data to km packet 

                    p_km_data->pn_no = 1; //pn_no update 1
                    p_km_data->seq_no++;//seq_no ++

				#if (AES_METHOD == 1)//aes 1
				    memcpy((uint8_t *)&p_km_data_enc->cmd, (uint8_t *)&p_km_data->cmd, sizeof(km_3_c_1_data_t));
				    aes_encrypt(private_key, &p_km_data->pn_no, &p_km_data_enc->pn_no);
				#endif
				}
			}
		}
	
		if(keyboard_send_need_f)
		{
			rf_state = RF_TX_START_STATUS;//rf state to TX status
			device_ack_received = 0;

            app_rf_set_timeout(rf_rx_timeout_us);
            app_rf_set_chn(rf_chn[device_channel]);
            //tlkapi_send_string_data(APP_LOG_EN, "t:", ptr, 20);
            rf_start_stx2rx(ptr, rf_stimer_get_tick());
            reg_rf_irq_status = 0xffff;//irq status reset to ffff
		}

	}
	else if (rf_state==RF_RX_END_STATUS)//rf status is rf_rx end
	{
        irq_device_rx();//deal rx receive packet
        check_rf_complet_status();//check rf complet 
	}
	else if (rf_state==RF_RX_TIMEOUT_STATUS)//rf status is timeout status
	{
        check_rf_complet_status();//check rf complet
	}
}


/**
 * @brief       This function deal ui_loop
 * @return      
 * @note        
 */
_attribute_ram_code_sec_ void ui_loop_24g()
{

    uint8_t has_new_key_event = 0;

	idle_status_poll();//poll idle status parameters
	
	//device_led_process();
	
    if((connect_ok==0))//no connect
	{
		adv_count_poll();//adv parameter poll
	}

 	 if (device_status == STATE_NORMAL) {//state normal
		if (has_new_key_event) //if has new keyboard action
		{
			has_new_key_event = 0;//reset has new mouse action flag 0
			reset_idle_status();//reset idle parameters
            pp_fifo_push(&tx_fifo, NORMAL_KB_DATA_CMD, (unsigned char *)&keyboard_buf, 6);
		} else if((idle_count < 3) || key_buf.press_cnt) {
       		uint8_t *p = pp_fifo_get_ptr(&tx_fifo);
			if (p == 0)
				pp_fifo_push(&tx_fifo, NORMAL_KB_DATA_CMD, keyboard_buf, 6);
		}
	}
}

#if 0
/**
 * @brief       This function deal 24g pm
 * @return      
 * @note        
 */
void pm_poll()
{
	uint32_t wake_src = 0;//use for set wake src
	uint32_t interval = 0;//use set time wake time
	need_suspend_flag = 0;//need suspend flag

	if(rf_state==RF_IDLE_STATUS)
	{
		if(device_status <= STATE_PAIRING)
		{
		#if D24G_ADV_ENTER_DEEPSLEEP_AFTER_TIME_OUT_ENABLE
			if(adv_count>=D24G_ADV_TIMER_OUT)//30s
            {
                enter_deep();//enter deep
            }
		 #endif
		 	wake_src = PM_WAKEUP_TIMER;//set wake src to timer
		 	interval = 8;//set wake up time interval
			need_suspend_flag = 1;//need suspend flag set to 1
		}
		else
		{
			if ((pp_fifo_get_num(&tx_fifo) <= 1)&&(connect_ok))//if no fifo data
			{
			#if D24G_OTA_ENABLE //ota en
				if(d24g_ota_status) //ota status
				{
				#if D24G_CONNECT_ENTER_DEEPSLEEP_AFTER_TIME_OUT_ENABLE //enterdeep when ota timeout en
					if(idle_count>=D24G_CONNECT_TIME_OUT) {// idle count > ota time out
						enter_deep();//enter deep
					}
				#endif //
				}
				else
			#endif
				if((idle_count < 3)||(DEVICE_LED_BUSY)) //idle count <3 and no led event
				{
					need_suspend_flag = 1;
					wake_src = PM_WAKEUP_TIMER;//set wake src to timer
					interval = SCAN_INTERVAL_TIME;//set wake up time interval
				}
				else
				{
				#if D24G_CONNECT_ENTER_DEEPSLEEP_AFTER_TIME_OUT_ENABLE
					if(idle_count >= D24G_CONNECT_TIME_OUT)
						enter_deep();
				#endif

					btn_set_wakeup_level_suspend(1); //set gpio wake up
					need_suspend_flag = 1;//set need suspend flag to 1
					wake_src = PM_WAKEUP_TIMER|PM_WAKEUP_PAD;//set wake up src to TIMER AND PAD
					interval = 1000;//every 1s wake up to check keyboard action
				}
			}
			else
			{
				if (no_ack>2000)//if no ack over 2000
				{
					no_ack=2100;//set no ack 2100
					if (key_buf.press_cnt == 0)
					{
                        printk("no_ack over 2000\n");
						enter_deep();
					}
				}
			}
		}

        if(need_suspend_flag)
        {
            cpu_sleep_wakeup(SUSPEND_MODE, wake_src, (wakeup_next_tick+ interval*SYSTEM_TIMER_TICK_1MS));//enter pm sleep
            wakeup_next_tick=clock_time();//update wakeup_next_tick
        }
    }
}
#endif

/**
 * @brief       This function 24g mouse main loop
 * @return      
 * @note        
 */
_attribute_ram_code_sec_ void d24_main_loop()
{
	static uint32_t tick_loop = 0;

	d24g_rf_loop();

	if(need_suspend_flag) {
		ui_loop_24g();
		tick_loop = clock_time()|1;
	} else if(clock_time_exceed(tick_loop, SCAN_INTERVAL_TIME*1000)) {
		tick_loop += SCAN_INTERVAL_TIME *1000* SYSTEM_TIMER_TICK_1US;
		wakeup_next_tick = clock_time()|1;
		ui_loop_24g();
	}

	//pm_poll();//pm deal
}