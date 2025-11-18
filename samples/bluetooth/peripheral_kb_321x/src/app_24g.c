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
LOG_MODULE_REGISTER(app_24g);



volatile int last_connect_status = 0xff;
u32 device_id = 0x12345678;

#define PAIR_ACCESS_CODE              0x39517695
ST_FLASH_DEV_INFO flash_dev_info  __attribute__ ((aligned (4)));
int dev_info_idx;

_attribute_data_retention_ u32 adv_begin_tick;
_attribute_data_retention_ u32 adv_count = 0;
u8 need_suspend_flag = 0; // need suspend flag init 0
u8 suspend_wake_up_enable; // suspend wake up en


#define DEVICE_TYPE_INDEX          1// 1 mouse  ,2 keyboard

_attribute_data_retention_ volatile unsigned int rf_state = 0;
_attribute_data_retention_ volatile unsigned int rf_rx_timeout_us = D24G_PAIR_TIMER_OUT;
_attribute_data_retention_ u8 device_channel = 0;

_attribute_data_retention_ int device_ack_received = 0;

u8 device_status = 0; //device status
u8 mouse_send_need_f = 0; //need send data flag
volatile u32 no_ack = 0; //no ack
volatile u32 start_rf_tick = 0; //start rf tick
_attribute_data_retention_ u32 loop_cnt;
_attribute_data_retention_  u8 connect_ok = 0;

#define  PAIR_RF_LEN   19
#define  KM_RF_LEN     13

rf_packet_t    rf_pair_buf =
{
    rf_tx_packet_dma_len(PAIR_RF_LEN + 1),	// dma_len
    19,	// rf_len
};

pair_data_t *p_pair_dat = (pair_data_t*)&rf_pair_buf.dat[0];

rf_packet_t rf_km_buf =
{
    rf_tx_packet_dma_len(KM_RF_LEN + 1),	// dma_len
    13,	// rf_len
};

km_data_t *p_km_data = (km_data_t*)&rf_km_buf.dat[0];//point to km data packet

#if (AES_METHOD == 1)
#define  KM_RF_ENC_LEN     18
rf_packet_t rf_km_buf_enc =
{
    rf_tx_packet_dma_len(KM_RF_ENC_LEN + 1), // dma_len
    18, // rf_len
};
km_data_t *p_km_data_enc = (km_data_t*)&rf_km_buf_enc.dat[0];//point to km enc packet
#endif

#if D24G_OTA_ENABLE //24g ota en
u8 d24g_ota_status = 0; //ota status
u8 d24g_ota_start_flag = 0; //ota start flag
u8 d24g_ota_success_flag = 0; //ota success flag
u32 d24g_ota_start_tick = 0;

rf_packet_t rf_ota_buf =
{
    rf_tx_packet_dma_len(sizeof(ota_data_t) + 1),	// dma_len
    sizeof(ota_data_t),	// rf_len
};
ota_data_t *p_ota_data = (ota_data_t*)&rf_ota_buf.dat[0]; //ota data pointer
ota_ack_data_t p_ota_ack_data ; //ota ack data

#define D24G_OTA_LENGTH  24 //ota len 24
typedef struct{
	u8	report_id;
	u8 	opcode;
	u16	length;
	u8	dat[20];
}ota_buff_t; //ota buff
ota_buff_t ota_buff;
u8 ota_buff_valid_flag;
#endif

u32 wakeup_next_tick = 0;// wake up tick next time
u8 pair_success_flag = 0;//use to assure pair success
u8 dongle_id_need_save_flag = 0; //0:no need save, 1:need save to flash

#if D24G_OTA_ENABLE
MYFIFO_INIT (fifo_km, 48, 16);//The size must be a multiple of 4 bytes
#else
MYFIFO_INIT (fifo_km, 12, 16);//The size must be a multiple of 4 bytes
#endif
_attribute_data_retention_ u8 private_key[16] =
{
    0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10
};

u32 dongle_id;

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

void adv_count_poll(void)
{
    u8 n;
    n = ((u32)(clock_time() - adv_begin_tick)) / SYSTEM_TIMER_TICK_1S;

    adv_begin_tick += n * SYSTEM_TIMER_TICK_1S;

    adv_count += n;
}

/**
 * @brief       This function clear pair flag
 * @return      
 * @note        
 */
void clear_pair_flag(void)
{
    pair_flag = 0;
    tlkapi_send_string_data(APP_LOG_EN, "clear_pair_flag", &pair_flag, 1);
    analog_write(USED_PAIR_ANA_REG,0);
}


/**
 * @brief       This function set pair flag
 * @return      
 * @note        
 */
void set_pair_flag(void)
{
    pair_flag = 1;
    tlkapi_send_string_data(APP_LOG_EN, "set_pair_flag", &pair_flag, 1);
    analog_write(USED_PAIR_ANA_REG, 1);
}


void user_reboot(u8 reason)
{
    write_deep_ana(reason);
    start_reboot();
}


#if D24G_OTA_ENABLE

/**
 * @brief       This function show ota result
 * @param[in]   result	- 
 * @return      
 * @note        
 */
void d24g_ota_resut(u8 result)
{
    tlkapi_send_string_data(APP_LOG_EN, "d24g_ota_result", &result, 1);

	u8 led_blink_times; //led blink times
	
	if(result == OTA_SUCCESS)
	{
		led_blink_times = 3; //success 3
	}
	else
	{
		led_blink_times = 5; //fail 5
	}

	for(u8 i=0;i<led_blink_times;i++)//loop for blink
	{
		gpio_write(D24G_LED, LED_OFF_HW);//led off
		sleep_ms(200);//200ms
	
		gpio_write(D24G_LED, LED_ON_HW);// led on
		sleep_ms(200);
	}
    start_reboot();
}


/**
 * @brief       This function for ota write
 * @param[in]   p_usb	- ota data
 * @return      
 * @note        
 */
u8 d24g_ota_write(ota_buff_t *p_usb)
{
	static	u16 ota_index=0;
	static u16 start_index=0;
	static u32 flash_write_addr=0;
	static u8 first_3_packet_data_buf[16 * 3];
	static u32 fw_size=0;
	static u8 ota_error_flag=0;
	ota_data_st *pd=(ota_data_st*)&p_usb->dat[0];//ota data

	if((pd->cmd== CMD_OTA_START)&&(p_usb->length==2))	//ota start	
	{
		d24g_ota_start_tick = clock_time();  //mark time
		d24g_ota_start_flag = 1;  //mark time
		//notify_rsp_buf_init();
		wd_stop();//stop wd
		ota_index=0;//ota index 0
#if APP_FLASH_PROTECTION_ENABLE
        app_flash_protection_operation(FLASH_OP_EVT_STACK_OTA_WRITE_NEW_FW_BEGIN, 0, 0);
#endif
		flash_write_addr=0;//write flash addr 0
		start_index=0;//start index 0
		fw_size=0;//fw size 0
		//fw_check_value=0;
		//fw_cal_crc=0xffffffff;
		ota_error_flag=OTA_SUCCESS;//ota success
		flash_erase_sector(ota_program_offset);//erase flash sector
		//app_enter_ota_mode();
	#if (BLT_APP_LED_ENABLE)//ota led en
		gpio_set_output_en(D24G_LED, 1);//led output en
		gpio_write(D24G_LED, LED_ON_HW);//led on
	#endif
		d24g_ota_start_tick = clock_time();  //mark time
        tlkapi_send_string_data(APP_LOG_EN, "24g_ota_start", 0, 0);
		return ota_error_flag;
	}
	else if((ota_error_flag==OTA_SUCCESS)&&(d24g_ota_start_flag))//ota next data deal
	{
		if((pd->cmd == CMD_OTA_END)&&(p_usb->length==6))//ota end		
		{
             tlkapi_send_string_data(APP_LOG_EN, "24g_ota_end", 0, 0);
			 u32 *telink_mark=(u32*)&first_3_packet_data_buf[32];//telink mark
			 if(telink_mark[0]!=0x544c4e4b)//telink mark wrong
			 {
			 	ota_error_flag=OTA_FIRMWARE_MARK_ERR;//mark err
				return ota_error_flag;
			 }
			 u32 real_bin_size=0;//real bin size 0
			 real_bin_size=fw_size-4;//fw_size-4
			 if(real_bin_size!=(start_index*16))//not equal start_index*16
			 {
			 	ota_error_flag=OTA_FW_SIZE_ERR;//size err
				return ota_error_flag;
			 }
			
			flash_write_page(ota_program_offset,16 * 3,first_3_packet_data_buf);//write first data buff to ota flash addr

			 u8 read_flash_buf[48];  //read flash buff
			 flash_read_page(ota_program_offset,16 * 3, read_flash_buf);//read first data 

			 if(memcmp(read_flash_buf, first_3_packet_data_buf, 16 * 3))//not equal
			 {  //do not equal
			 	flash_erase_sector(ota_program_offset);//erase ota data 
			 	ota_error_flag=OTA_WRITE_FLASH_ERR;//flash ota write err
				return ota_error_flag;
			}
			 
			u32 temp_ota_program_offset;//ota offset
			temp_ota_program_offset = ota_program_offset;	//ota program offset

			if(!ota_program_offset) ////zero, firmware is stored at flash 0x20000.
			{ 
				ota_program_offset = ota_program_bootAddr; ///NOTE: this flash offset need to set according to OTA offset
			}
			else ////note zero, firmware is stored at flash 0x00000.
			{                   
				ota_program_offset = 0x00000;
			}
			u8 ret=flash_fw_check(0xffffffff);//check fw
			ota_program_offset = temp_ota_program_offset;
			
			if(ret==0)//check ok
			{
				extern u32 fw_crc_init;
                tlkapi_send_string_data(APP_LOG_EN, "fw_crc_init", (u8 *)&fw_crc_init, 0);
				//usb_dp_pullup_en (0);
				//sleep_ms(200);
                tlkapi_send_string_data(APP_LOG_EN, "usb_ota_success", 0, 0);
				u32 flag = 0;//flag 0
				flash_write_page((ota_program_offset ? 0 : ota_program_bootAddr) + 0x20, 4, (u8 *)&flag);	//Invalid flag
				d24g_ota_success_flag = 1;//ota success flag set 1
			}
			else
			{
				flash_erase_sector(ota_program_offset);//erase ota program offset
				ota_error_flag = OTA_FW_CHECK_ERR;//fw_check err
				return ota_error_flag;
				
			}
		}
		else 
		{
			if((p_usb->length%20)!=0)//ota pdu len wrong
			{
				ota_error_flag=OTA_PDU_LEN_ERR;//ota pdu len err
				return ota_error_flag;
			}
			u8 cnt=p_usb->length/20;

			for(u8 i=0;i<cnt;i++)
			{
				pd = (ota_data_st*)&p_usb->dat[20*i];
				//my_printf_aaa("cmd=%d,crc=0x%04x_0x%04x.\r\n",pd->cmd,crc16((u8*)&pd->cmd,18),pd->crc);
				if(crc16_user((u8*)&pd->cmd,18)==pd->crc) //crc16_user
				{
					if(pd->cmd==0x0000)//first_data
		 			{
						memcpy(first_3_packet_data_buf,pd->buf,16);
						start_index = 0;
		 			} else if (pd->cmd==0x0001) {
                        memcpy(&first_3_packet_data_buf[16], pd->buf,16);
                        fw_size = pd->buf[8] | (pd->buf[9] <<8) |(pd->buf[10]<<16) | (pd->buf[11]<<24);
                        start_index = pd->cmd;
                        if((fw_size) > FW_SECTOR_LENGTH)
                        {
                            ota_error_flag=OTA_FW_SIZE_ERR;
                            return ota_error_flag;
                        }

                        if((fw_size) < flash_write_addr)
                        {
                            ota_error_flag = OTA_FW_SIZE_ERR;
                            return ota_error_flag;
                        }
                    } else if (pd->cmd==0x0002) {
                        start_index=pd->cmd;
                        memcpy(&first_3_packet_data_buf[32], pd->buf, 16);
                    }
					else
					{
						if((start_index+1)!=pd->cmd)
						{
							ota_error_flag=OTA_DATA_PACKET_SEQ_ERR;
							return ota_error_flag;
						}
						start_index=pd->cmd;

						if(ota_error_flag==OTA_SUCCESS)
						{
							if((flash_write_addr%4096)==0)
                            {
								flash_erase_sector(flash_write_addr + ota_program_offset);
                            }
                            flash_write_page(flash_write_addr + ota_program_offset,16,pd->buf);
						}
					}
					flash_write_addr+=16;
				}
				else
				{
					ota_error_flag=OTA_DATA_CRC_ERR;
					return ota_error_flag;
				}
			}
       }
	}
	return ota_error_flag;
}


/**
 * @brief       This function deal ota loop
 * @return      
 * @note        
 */
void d24g_ota_loop(void)
{
	if(d24g_ota_start_flag) //ota start flag 1
	{
		if(clock_time_exceed(d24g_ota_start_tick , 120 *1000 *1000))//over 120 s
		{
			d24g_ota_resut(OTA_TIMEOUT); //ota time out
		}
		else if((d24g_ota_success_flag)&&(p_ota_ack_data.pno_no==0))//ota success flag and pno_no 0
		{
			d24g_ota_success_flag = 0; //reset ota success flag 0

			d24g_ota_resut(OTA_SUCCESS);//ota success show
		}
	}
}
#endif



/**
 * @brief       This function deal rx packet
 * @param[in]   p_rf_data	- rx packet
 * @return      
 * @note        
 */
_attribute_ram_code_sec_ u8  rf_rx_process(rf_packet_t *p_rf_data)
{
	if(device_status == STATE_PAIRING) //pair status
	{
	    pair_ack_data_t *pair_ack_dat_ptr=(pair_ack_data_t*)&p_rf_data->dat[0]; //pair ack dat ptr
		{
#if (AES_METHOD == 0) //no aes
        if (((pair_ack_dat_ptr->cmd == PAIR_ACK_CMD) || (pair_ack_dat_ptr->cmd == RECONNECT_ACK_CMD)) && (pair_ack_dat_ptr->did == p_pair_dat->did))//if cmd ==pair_ack_cmd, and ack data did == pair data did
#elif (AES_METHOD == 1) //aes 1
        if (((pair_ack_dat_ptr->cmd == PAIR_ACK_CMD) || (pair_ack_dat_ptr->cmd == RECONNECT_ACK_CMD))&& (memcmp((u8 *)&pair_ack_dat_ptr->did, (u8 *)&p_pair_dat->did, 16)==0))//if cmd ==pair_ack_cmd, and ack data did == pair data did
#endif
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
            	return 1;
        	}
		}
	}
	else if(device_status == STATE_NORMAL)//normal status
	{
		if(mouse_send_need_f == 2)//mouse ack
		{
			km_ack_data_t *km_ack_dat_ptr = (km_ack_data_t *)&p_rf_data->dat[0]; //km ack data ptr

#if (DEVICE_TYPE_INDEX == 1)
            //my_printf_aaa("cmd %d\n", km_ack_dat_ptr->cmd);
			if(km_ack_dat_ptr->cmd == MOUSE_ACK_CMD)//ack data cmd == mouse ack cmd
			{
				return 1;
			}
#endif
		}
	}
#if D24G_OTA_ENABLE //ota en
	else if(device_status == STATE_OTA) //ota status
	{
		if(mouse_send_need_f == 3)//ota ack
		{
			ota_ack_data_t *ota_ack_dat_ptr = (ota_ack_data_t *)&p_rf_data->dat[0];//ota ack dat ptr
			if(ota_ack_dat_ptr->cmd == D24G_OTA_ACK_CMD)//ota ack cmd
			{
				if(ota_ack_dat_ptr->pno_no) //pno_no no 0
				{
					static u16 packet_cnt=0;//cnt set 0
					ota_data_st *pd=(ota_data_st *)&ota_ack_dat_ptr->dat[0]; //ota data ptr
					ota_buff_t *p=(ota_buff_t *)&p_rf_data->dat[7];//ota buf ptr
					switch(ota_ack_dat_ptr->length)//determine ack len
					{
						case 2: //ota - start
						case 6: //ota - end
							if((pd->cmd==0xff01)||(pd->cmd==0xff02))//cmd
							{
								ota_buff_valid_flag = 1;//valid flag

								memcpy(&ota_buff.report_id,&p->report_id,D24G_OTA_LENGTH);//cpy ota received data to ota buff
                                printf("s");
								packet_cnt=0;//packet cnt set 0
							}
							break;
						case 20: //ota data receive
							if((crc16_user((u8*)&ota_ack_dat_ptr->dat[0],18) == pd->crc)&&\
								(packet_cnt==pd->cmd))
							{
								ota_buff_valid_flag = 1;//valid falg

								memcpy(&ota_buff.report_id,&p->report_id,D24G_OTA_LENGTH);//cpy ota received data to ota buff
								packet_cnt++;//pkt ++
                                //printf("o");
							}
							break;
						default:
                            tlkapi_send_string_data(APP_LOG_EN, "---err data", 0, 0);
							break;
					};

					memcpy(&p_ota_ack_data.pno_no,&ota_ack_dat_ptr->pno_no,29);//cpy ack data to p_ota_ack_data

					my_fifo_push(&fifo_km, (u8 *)&p_ota_ack_data.pno_no, 29);//push ota_ack data to fifo
				}

				return 1;
			}
		}
	}
#endif
	
    return 0;
}



/**
 * @brief       This function start pair
 * @return      
 * @note        
 */
void d24_start_pair()
{
    set_pair_flag();

	device_status = STATE_PAIRING;
	rf_rx_timeout_us = D24G_PAIR_TIMER_OUT;
#if (AES_METHOD == 1)
		p_pair_dat->cmd = PAIR_CMD | BIT(7);
#else
		p_pair_dat->cmd = PAIR_CMD; //Load PAIR_CMD in pairing packets
#endif
	adv_count = 0; //adv count 0 
	adv_begin_tick = clock_time()|1; //adv begin tick
}

#if (AES_METHOD)
/**
 * @brief       This function encrypt data
 * @param[in]   encrypted_data	- 
 * @param[in]   key	- 
 * @param[in]   plaintext	- 
 * @return      
 * @note        
 */
void aes_user_encryption(u8 *key, u8 *plaintext, u8 *encrypted_data)
{
	aes_encrypt(key, plaintext, encrypted_data); //aes encrypt
}
 

/**
 * @brief       This function decrypt data
 * @param[in]   decrypted_data	- 
 * @param[in]   encrypted_data	- 
 * @param[in]   key	- 
 * @return      
 * @note        
 */
void aes_user_decryption(u8 *key, u8 *encrypted_data, u8 *decrypted_data)
{
	aes_decrypt(key, encrypted_data, decrypted_data); //aes discrypt
}
#endif


/**
 * @brief       This function init 
 * @return      
 * @note        
 */
void d24_user_init(void)
{
    u32 dev_mac;

    tlkapi_send_string_data(APP_LOG_EN, "d24_user_init", 0, 0);

    flash_read_page(flash_sector_mac_address, 4, (u8 *)&dev_mac);

    u32 device_id = ((dev_mac << 8)|DEVICE_TYPE_INDEX);//did set

	p_pair_dat->cmd = PAIR_CMD; //pair cmd
	p_pair_dat->did = device_id; //did
	p_km_data->did = device_id; //did

#if (AES_METHOD == 1) //aes 1
    p_km_data->cmd = MOUSE_CMD | (1 << 7); //mouse aes cmd
    memcpy((u8*)&private_key[0], (u8*)&device_id ,4); //cpy did to first 4 bytes pri key  
#else
    p_km_data->cmd = MOUSE_CMD; //mouse cmd
#endif

#if D24G_OTA_ENABLE // ota en
	p_ota_data->cmd = D24G_OTA_CMD; // ota cmd
	p_ota_data->did = device_id; //did
	p_ota_ack_data.did = device_id; //ack data did
#endif
    tlkapi_send_string_data(APP_LOG_EN, "---device_id=", &device_id, 4);

    dongle_id = flash_dev_info.dongle_id;

    tlkapi_send_string_data(APP_LOG_EN, "dongle_id:", &dongle_id, 4);
    //u8 ret = gpio_get_level(DPI_BTN_PIN);
    //tlkapi_send_string_data(APP_LOG_EN, "ret:", &ret, 1);
    if ((dongle_id == U32_MAX) || (dongle_id == 0) ||(gpio_get_level(DPI_BTN_PIN)==0 && deep_flag == POWER_ON_ANA)) { // no valid dongle id
        set_pair_flag();
    }

    if (pair_flag) // pair flag 1
	{
        device_status = STATE_PAIRING; //device into pair status
		rf_rx_timeout_us = D24G_PAIR_TIMER_OUT; //pair time out
	#if (AES_METHOD == 1) //aes 1
		generateRandomNum(12, &private_key[4]); // random num init
		aes_user_encryption(pub_key, private_key, (u8*)&p_pair_dat->did); //encry pair data
	#endif
    } 
#if D24G_OTA_ENABLE //ota en
	else if(d24g_ota_status) //ota status
	{
		device_status = STATE_OTA; //state ota
		rf_rx_timeout_us = D24G_OTA_TIMER_OUT; //ota time out
		my_fifo_reset(&fifo_km);
	}
#endif
	else 
	{
	#if(AES_METHOD == 1) //aes 1
		memcpy((u8*)&private_key[4], (u8*)&flash_dev_info.key[0], 12); //pri key
	#endif
        device_status = STATE_NORMAL; //device normal
		rf_rx_timeout_us = D24G_COMMUNICATION_TIMER_OUT; // communication time out
    }

    rf_state = RF_IDLE_STATUS;

    wakeup_next_tick = clock_time(); //next wake up tick update
    reset_idle_status(); //reset idle paras
}

/**
 * @brief       This function check rf complet status
 * @return      
 * @note        
 */
void check_rf_complet_status()
{
	static u32 ack_miss_no = 0; // ack miss no

	rf_state = RF_IDLE_STATUS; //rf reset to idle status

	if (device_ack_received) { // receive ack
		ack_miss_no = 0; //reset ack miss 0
		no_ack = 0; //reset no ack  0
		start_rf_tick = 0; //reset start rf tick 0

		if(device_status <= STATE_PAIRING) { // pair status
			if(pair_success_flag) { //pair success
				device_status = STATE_NORMAL; // state normal
				rf_rx_timeout_us = D24G_COMMUNICATION_TIMER_OUT; //rx timout
                connect_ok=1; // connect ok 1
    		    if (flash_dev_info.dongle_id != dongle_id) // new dongle id
		        {
		            flash_dev_info.dongle_id = dongle_id; //update dongle id
                    tlkapi_send_string_data(APP_LOG_EN, "pairing success------------", 0, 0);
				#if (AES_METHOD == 1)//aes method 1
                    memcpy((u8*)&flash_dev_info.key[0], (u8*)&private_key[4], 12);//update pri key
				#endif
					if (dongle_id_need_save_flag) //need to save dongle_id to flash
					{
						save_dev_info_flash();
					}
		        }
				clear_pair_flag(); // clear pair flag
        		reset_idle_status(); //reset idle params
        		
			}
            my_fifo_reset(&fifo_km); //reset fifo
		} 
        else if(device_status == STATE_NORMAL)//normal status
        {
			if (mouse_send_need_f){
                my_fifo_pop(&fifo_km);
            }
            connect_ok = 1;//update connect ok
		}
	#if D24G_OTA_ENABLE // ota en
		else if(device_status == STATE_OTA)//ota status
        {
			if (mouse_send_need_f)//need send ota data
			{
				my_fifo_pop(&fifo_km);//rptr ++

				if(ota_buff_valid_flag)//ota buff vlaid
				{
					u8 ota_error_flag=d24g_ota_write(&ota_buff);//write ota data

					if(ota_error_flag)//has error
					{
                        tlkapi_send_string_data(APP_LOG_EN, "-err flag=%d------------", &ota_error_flag, 1);

						d24g_ota_resut(ota_error_flag);//show error
					}

					ota_buff_valid_flag = 0;//reset ota buff valid

					idle_status_poll();//idle status poll
				}
			}
            connect_ok=1; //ota connect ok
		}
	#endif
		mouse_send_need_f = 0;//reset mouse need send flag
	}
	else
	{
		no_ack++;
        ack_miss_no ++;

        if (ack_miss_no >=3) {
            device_channel = get_next_channel_with_mask(0, device_channel);//update channel
        }

        if(no_ack > 125) {
            connect_ok = 0;
        }
	}  
}



/**
 * @brief       This function deal rf machine loop
 * @return      
 * @note        
 */
void d24g_rf_loop()//rf state machine loop
{
	u8 *ptr = 0;

	if (rf_state == RF_IDLE_STATUS)//rf status idle
	{
		if (device_status <= STATE_PAIRING)//device status not normal
		{
			set_pair_access_code(PAIR_ACCESS_CODE);//set defualt access code
			pair_success_flag = 0;//pair success flag set to 0
			if (device_status == STATE_PAIRING) {
			    app_set_rf_power(RF_POWER_P0dBm);
			}

			connect_ok = 0;//no connect
			ptr = (u8 *)&rf_pair_buf;
			mouse_send_need_f = 1;
			start_rf_tick = 0;

		}
		else if(device_status == STATE_NORMAL) //status normal
		{
		#if (AES_METHOD == 1) //aes 1
			ptr = (u8 *)&rf_km_buf_enc;//normal status pointer to enc km data packet
		#else
			ptr = (u8 *)&rf_km_buf;//normal status pointer to km data packet
		#endif
			set_data_access_code(flash_dev_info.dongle_id);//dongle id to be data access code
			app_set_rf_power(RF_POWER_NORMAL);//set data tx power

            if(mouse_send_need_f == 0)//mouse need send f
            {
                u8 *p =  my_fifo_get (&fifo_km);//get data

				if (p)//if have data
                {
					start_rf_tick = clock_time()|1;//update rf tick
				    mouse_send_need_f = 2;//send data
                    u8 *tmp = (u8 *)&p[0]; //tmp data
                    km_data_t *km_dat1; //km data

                    u8 *src = (u8 *)&p_km_data->cmd; //pointer to p_km_data
                    km_dat1 = (km_data_t*)&src[0];//kmdat1 pointer p_kmdata too

                    memcpy(&km_dat1->km_dat[0], &tmp[0], 6);//cpy fifo km data to km packet 

					p_km_data->pn_no = 1; //pn_no update 1
                    p_km_data->seq_no++;//seq_no ++

				#if (AES_METHOD == 1)//aes 1
                    memcpy((u8 *)&p_km_data_enc->cmd, (u8 *)&p_km_data->cmd, sizeof(km_data_t));//copy km data to km data enc
                    aes_encrypt(private_key, &p_km_data->pn_no, &p_km_data_enc->pn_no);//encry_pmdata to pm data en
				#endif

				}
        	}
		}
	#if D24G_OTA_ENABLE //ota en
		else if(device_status == STATE_OTA) //ota status
		{
			if(d24g_ota_status) //ota status
			{
				ptr = (u8 *)&rf_ota_buf; //ptr to ota buff

				set_data_access_code(flash_dev_info.dongle_id); //set dongle access code
	            app_set_rf_power(RF_POWER_NORMAL); //set tx power

	            if(mouse_send_need_f == 0)//no data need send
	            {
	                u8 *p =  my_fifo_get (&fifo_km);//get data

	                if(p)//has data
	                {
	                    mouse_send_need_f = 3; //mouse need send ota date
						u8 *tmp = (u8 *)&p[0]; //point to data
                        p_ota_data->pno_no = tmp[0];
                        memcpy((u8 *)&p_ota_data->report_id, &tmp[5], 24);//cpy to p ota data
					}
            	}
			}
		}
	#endif
	
		if (mouse_send_need_f) //send
		{
			rf_state = RF_TX_START_STATUS;//rf state to TX status
			//rf_set_tx_rx_off();
			device_ack_received = 0;

            app_rf_set_timeout(rf_rx_timeout_us);
            app_rf_set_chn(rf_chn[device_channel]);
            //tlkapi_send_string_data(APP_LOG_EN, "t:", ptr, 20);
            rf_start_stx2rx(ptr, rf_stimer_get_tick());

            reg_rf_irq_status = 0xffff;
		}

	}
	else if(rf_state==RF_RX_END_STATUS)
	{   //get rx data
        irq_device_rx();
		check_rf_complet_status();
	}
	else if(rf_state==RF_RX_TIMEOUT_STATUS)
	{   //rx timeout
		check_rf_complet_status();
	}

}



/**
 * @brief       This function deal ui_loop
 * @return      
 * @note        
 */
void ui_loop_24g()
{
    static u32 tick_loop_24g;
    u8 wheel_flag = 0;
    u32 wheel_prepare_tick;

	if(clock_time_exceed(tick_loop_24g, 7000)) // 7ms ui loop
	{
		tick_loop_24g = clock_time();//update time

        device_led_process();// led process

        if(connect_ok == 0)//no connect
        {
#if BLT_APP_LED_ENABLE
    		led_2p4_Adv_poll();//adv led
#endif
            adv_count_poll();//adv parameter poll
        }
    }

     app_pp_get_sensor_data();

 	 if ((device_status == STATE_NORMAL)) {
        if (has_new_mouse_data) {
			has_new_mouse_data = 0;
			reset_idle_status();
			my_fifo_push(&fifo_km, &ms_data.btn, sizeof(mouse_data_t) - 1);
            ms_data.z_wheel = 0;//wheel data set 0
            ms_data.x=0;
            ms_data.y=0;

        } else if((idle_count < 3)||ms_data.btn) {
			u8 *p = my_fifo_get(&fifo_km);
            if(p == 0) {
				my_fifo_push(&fifo_km, &ms_data.btn, sizeof(mouse_data_t) - 1);
            }
        }
	 }
}


/**
 * @brief       This function deal 24g pm
 * @return      
 * @note        
 */
void pm_poll(void)
{
	u32 wake_src = 0;//use for set wake src
	u32 interval = 0;
	need_suspend_flag = 0;//need suspend flag

	if(rf_state == RF_IDLE_STATUS) { //rf idle status
		if(device_status <= STATE_PAIRING) { //no normal status
#if D24G_ADV_ENTER_DEEPSLEEP_AFTER_TIME_OUT_ENABLE //enterdeep when adv timeout en
			if(adv_count>=D24G_ADV_TIMER_OUT) {//adv count>adv time out
                app_enter_sleep(D24G_PAIR_TIMEOUT_SLEEP);
            }
#endif
		 	wake_src = PM_WAKEUP_TIMER;//set wake src to timer
		 	interval = 8;//set wake up time interval
			need_suspend_flag = 1;//need suspend flag set to 1
		} else {
			if((my_fifo_number(&fifo_km) <= 1) && (connect_ok == 1)) {
			#if D24G_OTA_ENABLE //ota en
				if(d24g_ota_status) //ota status
				{
				#if D24G_CONNECT_ENTER_DEEPSLEEP_AFTER_TIME_OUT_ENABLE //enterdeep when ota timeout en
					if(idle_count>=D24G_CONNECT_TIME_OUT) {// idle count > ota time out
                        app_enter_sleep(D24G_CONNECT_NO_ACTIVE_TIMEOUT_SLEEP);
					}
				#endif
				}
				else
			#endif
				if((idle_count < 3)||(DEVICE_LED_BUSY)) //idle count <3 and no led event
				{
					if ((ms_param_save.report_rate_index == 4) || (ms_param_save.report_rate_index == 8))
					{
						need_suspend_flag = 1; //need suspend falg set 1
				 		wake_src = PM_WAKEUP_TIMER;//timer wrc
                        interval = ms_param_save.report_rate_index; //wake up interval set to report rate

                        if(suspend_wake_up_enable) {//suspend wake up en
    					#if WHEEL_FUN_ENABLE // wheel fun en
                            wheel_set_wakeup_level_suspend(0);      //wheel gpio wake up disable
    					#endif

    					#if BUTTON_FUN_ENABLE //btn fun en
                            btn_set_wakeup_level_suspend(0);//btn gpio wake up disable
    					#endif

    					#if SENSOR_FUN_ENABLE //sensor fun en
                            sensor_set_wakeup_level_suspend(0);//sensor gpio wake up disable
    					#endif
                            suspend_wake_up_enable = 0; //reset suspend wake up en
                        }

					}
				} else {
					#if D24G_CONNECT_ENTER_DEEPSLEEP_AFTER_TIME_OUT_ENABLE //when connect time out en
						if(idle_count >= D24G_CONNECT_TIME_OUT) { //idle cnt > connect time out
                            app_enter_sleep(D24G_CONNECT_NO_ACTIVE_TIMEOUT_SLEEP);
						}
					#endif

					#if WHEEL_FUN_ENABLE //wheel fun en
						wheel_set_wakeup_level_suspend(1);//wheel wake up
					#endif

					#if BUTTON_FUN_ENABLE //btn fun en
				        btn_set_wakeup_level_suspend(1);//btn wake up
					#endif //

					#if SENSOR_FUN_ENABLE //sensor en
				        sensor_set_wakeup_level_suspend(1);//sensor wake up
					#endif //
						need_suspend_flag = 1;//need suspen 1
                        suspend_wake_up_enable = 1; //suspend wake up en
						wake_src=PM_WAKEUP_TIMER|PM_WAKEUP_PAD;//gpio and timer 
						interval = 100;//100 ms
				}
			}
			else
			{
				if(no_ack > 2000) { //no ack over 2000
					no_ack = 2100; //set no ack 2100
                    tlkapi_send_string_data(APP_LOG_EN, "no ack enter deep:", 0, 0);
                    app_enter_sleep(D24G_RECONNECT_TIMEOUT_SLEEP);
				}
			}
		}

		if(need_suspend_flag) { //need suspend flag 1
			cpu_sleep_wakeup(SUSPEND_MODE, wake_src, (wakeup_next_tick + interval * SYSTEM_TIMER_TICK_1MS));
            wakeup_next_tick = clock_time();
			//pp_rf_init(1);
		}
	}	 
}

/**
 * @brief       This function 24g mouse main loop
 * @return      
 * @note        
 */
void d24_main_loop(void)
{
	u32 temp = 0;//tmp init 0
	static u32 tick_loop = 0; //tick loop 0
	d24g_rf_loop();

	if(device_status <= STATE_PAIRING) //pairing status
	{
		temp = 8000;
	}
	else
	{
		temp = ms_param_save.report_rate_index * 1000;//report time

		if(my_fifo_number(&fifo_km) > 12)//fifo num >12
		{
			temp += 3000;//+3ms
		}
		else if(my_fifo_number(&fifo_km) > 10) //fifo num >10
		{
			temp += 2000;//+2ms
		}
		else if(my_fifo_number(&fifo_km) > 8) //fifo num >8
		{
			temp += 1500;//+1.5ms
		}
		else if(my_fifo_number(&fifo_km) > 6) //fifo num >6
		{
			temp += 1000; //+1ms
		}
		else if(my_fifo_number(&fifo_km) > 3)//fifo num >3
		{
			temp += 500;//+0.5ms
		}
	}


#if D24G_OTA_ENABLE //ota fun en
	if(d24g_ota_status)//ota status
	{
		if(clock_time_exceed(tick_loop, 1000))//over tick loop 1ms
		{
			tick_loop = clock_time();//update tick loop

			u8 *p = my_fifo_get(&fifo_km);//get data
			if(p == 0) {// no data
                //printf("P");
				p_ota_ack_data.pno_no = 0; //pno_no 0
				my_fifo_push(&fifo_km, &p_ota_ack_data.pno_no, 29);//push into fifo
			}

			adv_count_poll(); //adv count poll

			if(d24g_ota_start_flag==0)//no ota start
			{
				device_led_process();//led proc
				
				led_ota_ready_set();//led ota ready show
			}
			else
			{
				gpio_write(D24G_LED, LED_ON_HW);//led on
			}
		}
	}
	else
#endif
	if(need_suspend_flag)
	{
		ui_loop_24g();
		tick_loop = clock_time()|1;
	}
	else if(clock_time_exceed(tick_loop, temp))
	{
		tick_loop += temp * SYSTEM_TIMER_TICK_1US;
        wakeup_next_tick = clock_time();
		ui_loop_24g();
	}


	if(vbus_status == 0) {
        pm_poll();
    }
}

