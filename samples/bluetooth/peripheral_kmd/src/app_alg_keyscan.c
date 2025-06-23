/* main.c - Application main entry point */

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

#include <zephyr/usb/usb_device.h>
#include <zephyr/usb/class/usb_hid.h>

#include "app_public.h"

#define LOG_LEVEL LOG_LEVEL_DBG
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(alg_keyscan);


#if ALG_KEYSCAN_APP_FUN_ENABLE

#define KEYSCAN_TEST_1XADC                  0//KEYSCAN_1XADC_MODE
#define KEYSCAN_TEST_2XADC                  1//KEYSCAN_2XADC_MODE
#define KEYSCAN_TEST_1XADC_8K_ONCE          2//KEYSCAN_1XADC_8K_ONCE_MODE
#define KEYSCAN_TEST_2XADC_8K_ONCE          3//KEYSCAN_2XADC_8K_ONCE_MODE
#define KEYSCAN_TEST_2XADC_8K_TWICE_M1      4//KEYSCAN_2XADC_8K_TWICE_M1_MODE
#define KEYSCAN_TEST_2XADC_8K_TWICE_M2      5//KEYSCAN_2XADC_8K_TWICE_M2_MODE

#define KS_TEST_MODE                        KEYSCAN_TEST_2XADC_8K_ONCE

#define KS_DMA_LLP_ENABLE 1
#define  DMA1_KEY_SCAN   DMA4
#define  DMA2_KEY_SCAN   DMA5

#define BUFFER_SIZE (256) //the minimum buffer size (unit:byte), adc data format 16bits
#define KEYSCAN_ADC_BUFFER_SIZE (BUFFER_SIZE / 4)
short adc0_buffer[KEYSCAN_ADC_BUFFER_SIZE] = {0}, adc1_buffer[KEYSCAN_ADC_BUFFER_SIZE] = {0};
_attribute_aligned_(4)  unsigned short adc_buffer[KEYSCAN_ADC_BUFFER_SIZE*2] = {0};





#define TOTAL_ROW                       														8
#define TOTAL_COL                      															16

//unsigned char const map_window[TOTAL_COL][TOTAL_ROW] = {
unsigned char map_window[TOTAL_COL][TOTAL_ROW] = {\
/*C01*/{KB_M,       KB_Left,    KP_jiahao,  KP_Del,     KB_N,       KB_RCtrl,   KP_9,           KP_0},\
/*C02*/{KB_B,       T_FN,       KP_8,       KP_enter,   KB_V,       KB_RAlt,    KP_7,           KP_3},\
/*C03*/{KB_C,       KB_Space,   KP_jianhao, KP_2,       KB_X,       KB_LAlt,    KP_chenghao,    KP_1},\
/*C04*/{KB_Z,       KB_LWin,    KP_chuhao,  KP_6,       KB_LShift,  KB_LCtrl,   KB_Num,         KP_5},\
/*C05*/{KB_douhao,  KB_Down,    KP_4,       0,          KB_juhao,   KB_Right,   0,              0},\
/*C06*/{KB_wenhao,  0,          0,          0,          KB_RShift,  0,          0,              0},\
/*C07*/{KB_Up,      0,          0,          0,          0,          0,          0,              0},\
/*C08*/{0,          0,          0,          0,          0,          0,          0,              0},\
/*C09*/{KB_J,       KB_U,       KB_7,       KB_F7,      KB_H,       KB_Y,       KB_6,           KB_F6},\
/*C10*/{KB_G,       KB_T,       KB_5,       KB_F5,      KB_F,       KB_R,       KB_4,           KB_F4},\
/*C11*/{KB_D,       KB_E,       KB_3,       KB_F3,      KB_S,       KB_W,       KB_2,           KB_F2},\
/*C12*/{KB_A,       KB_Q,       KB_1,       KB_F1,      KB_Caps,    KB_Tab,     KB_dunhao,      KB_Esc},\
/*C13*/{KB_K,       KB_I,       KB_8,       KB_F8,      KB_L,       KB_O,       KB_9,           KB_F9},\
/*C14*/{KB_fenhao,  KB_P,       KB_0,       KB_F10,     KB_yinhao,  KB_Lguohao, KB_jianhao,     KB_F11},\
/*C15*/{KB_Enter,   KB_Rguohao, KB_denghao, KB_F12,     0,          KB_xiegang, KB_Back,        KB_Home},\
/*C16*/{0,          0,          KB_PgUp,     KB_Delete, 0,          0,          KB_PgDown,      KB_Insert},\
};

unsigned char hw_now_bits[TOTAL_COL];  //hardware bits
unsigned char hw_last_bits[TOTAL_COL];  //hardware bits

unsigned char get_key_value( unsigned short *buf,unsigned short release_threshold,unsigned short press_threshold)
{
    if(buf[0]<release_threshold)
    {
        return 0;
    }
    else if(buf[0]>press_threshold)
    {
        return 1;
    }
    else
    {
        return 2;
    }
    return 1;
}

unsigned char key_scan(void)
{
    unsigned char has_new_key_event=0;
    unsigned char press_flag=0;
    app_key_buf.press_cnt=0;
    app_key_buf.cnt=0;
    app_key_buf.special_key_press_f=0;

    //tmemset(&app_key_buf.hw_now_bits[0], 0, TOTAL_COL);
     for(int col=0;col<TOTAL_COL;col++)
     {
        for(int i=0;i<8;i++)
        {  
       
           if(map_window[col][i]!=0)
           {
                press_flag = get_key_value(&adc_buffer[8*col+i],0x310,0x360);
 				LOG_INF("adc_buffer[%d] = %d", 8*col+i, adc_buffer[8*col+i]);
           }
             if(press_flag==1)
             {
                hw_now_bits[col]|=(1<<i);
                app_key_buf.press_cnt++;
                key_fifo(map_window[col][i]);
				LOG_INF("press_flag %d, x %d, y %d", col, i);
             }
             else if(press_flag==0)
             {
               hw_now_bits[col]&=0xff-(1<<i);
             }
             #if 0
             if((app_key_buf.hw_last_bits[col]&(1<<i))!=(app_key_buf.hw_now_bits[col]&(1<<i)))
             {
                debug_print_keyscan("col=%d,row=%d,adc_buf=0x%0x, ",col,i,adc_buffer[8*col+i]);
             }
             #endif
        }
        if(hw_last_bits[col]!=hw_now_bits[col])
        {
            has_new_key_event=1;
            hw_last_bits[col]=hw_now_bits[col];
            // debug_print_keyscan("col=%d,hw_now_bits=%02x\r\n",col,app_key_buf.hw_last_bits[col]);
        }
     }

     if(has_new_key_event)
     {
         #if 0
        for(int i=0;i<app_key_buf.cnt;i++)
        {
            debug_print_keyscan("%02X ",app_key_buf.keycode[i]);
        }
        debug_print_keyscan("\r\n");
        #endif
       // TODO: key_data_handle();
        //print_app_public("special_key_press_f=%d\r\n",app_key_buf.special_key_press_f);
       // TODO:  special_key_event_handle();
     }

    return has_new_key_event;
}
#endif


#if DIGIT_KEYSCAN_FUN_ENABL









#endif