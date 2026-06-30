/** @file app_kb_matrix.c
 *  @brief
 */

/*
 * Copyright (c) 2025 Telink Semiconductor
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
#include "app_kb_matrix.h"
#include "app_alg_keyscan.h"

#define LOG_LEVEL LOG_LEVEL_DBG
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(app_kb_matrix);

//const unsigned short consumer_list[]={
unsigned short consumer_list[]={
0x221,      //0xa3 C_WWW_SEARCH     
0x223,      //0xa4 C_WWW_HOME
0x224,      //0xa5 C_WWW_BACKWARD
0x225,      //0xa6 C_WWW_FORWARD
0x226,      //0xa7 C_WWW_STOP
0x227,      //0xa8 C_WWW_REFRESH
0x22A,      //0xa9 C_MY_FAVORITE
0x183,      //0xaa C_MEDIA_SELECT
    
0x18A,      //0xab C_EMAIL
0x192,      //0xac C_CALCULATOR
0x194,      //0xad C_MY_COMPUTER
0xB5,       //0xae C_NEXT_TRACK
0xB6,       //0xaf C_PRE_TRACK
0xB7,       //0xb0 C_STOP
0xCD,       //0xb1 C_PLAY_PAUSE
0xE2,       //0xb2 C_MUTE   
    
0xE9,       //0xb3 C_VOL_INC
0xEA,       //0xb4 C_VOL_DEC    
0x00,       //0xb5 telink�Զ����??
0x22D,      //0xb6 USAGE ZOOM IN
0x22E,      //0xb7   USAGE ZOOM OUT 
0x236,      //0xb8   USAGE PAN LEFT
0x237,      //0xb9   USAGE PAN RIGHT
0x30B,      //0xba  C_BRIGHT_INC    
    
0x30A,      //0xbb  C_BRIGHT_DEC
0xB8,       //0Xbc   c_rject
0x30,       //0Xbd  C_POWER         
0x19E,      //0Xbe  C_TERMINAL_LOCK 
};
app_kb_data_t app_key_buf;
unsigned char fn_flag=0;


_attribute_ram_code_sec_ void key_fifo(unsigned char key_code)
{
    unsigned char real_key_code=special_key_press_flag_set(key_code);
    if (real_key_code==0) //key_code=0 not save
    {
        return;
    }
    
    
     app_key_buf.cnt++;
    if (app_key_buf.cnt > MAX_BTN_CNT)
    {
        return;
    }
    app_key_buf.keycode[app_key_buf.cnt-1] = real_key_code;
}


_attribute_ram_code_sec_ unsigned char proc_hotkey(unsigned char key_code)
{

    if((key_code>=0xe0) && (key_code<0xe8))
    {
        app_key_buf.nk[0] |= (1 << (key_code-0xe0));
        return 1;
    }

    if((key_code >= C_INDEX_START) && (key_code <= C_INDEX_END))
    {
        app_key_buf.ck = key_code;
        return 1;
    }
    
    if ((key_code >= S_SLEEP) && (key_code <= S_WAKEUP))
    {
        app_key_buf.sk = key_code;
        return 1;
    }

#if 0
   for(int i=0;i<nk_cnt;i++)
   {
        if(app_key_buf.nk[2+i]==key_code)
        {
            nk_bit|=1<<i;
            return 1;
        }
   }
    
#endif
    if(key_code>0x9f)
    {
        return 1;
    }

    return 0;
}

_attribute_ram_code_sec_ uint8_t tpsll_send_keyboard_data(uint8_t *data, uint8_t len, uint8_t cmd, kb_cb_t cb, void *user_arg)
{
    if(!data)
    {
        return TLK_ERR_NULL;
    }

    if (cmd == NORMAL_KB_DATA_CMD)
    {
        return  pp_fifo_push(&d25fKbTxFifo, NORMAL_KB_DATA_CMD, data, len);
    }
    else if (cmd == CONSUME_KB_DATA_CMD)
    {
        return  pp_fifo_push(&d25fKbTxFifo, CONSUME_KB_DATA_CMD, data, len);
    }
    else if (cmd == SYSTEM_KB_DATA_CMD)
    {
        return  pp_fifo_push(&d25fKbTxFifo, SYSTEM_KB_DATA_CMD, data, len);
    }
    else if (cmd == ALL_KB_DATA_CMD)
    {
        return  pp_fifo_push(&d25fKbTxFifo, ALL_KB_DATA_CMD, data, len);
    }
    return TLK_SUCCESS;
}

_attribute_ram_code_sec_ void key_data_handle(void)
{
    static  unsigned char  nk_cnt=0;
    static unsigned char ck_last=0;
    static  unsigned char sk_last=0;
    static  unsigned char nk_last[8]={0,0,0,0,0,0,0,0};
    static  unsigned char ak_last[ALL_KEY_BUF_SIZE]={0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
    unsigned char i;
    unsigned char index=0;
    unsigned char bit=0;
    unsigned char key=0;
    bool nk_flg=false;
    bool ak_flg=false;
    app_key_buf.nk[0]=0;
    // tmemset((u8 *)&app_key_buf.ak[0], 0, sizeof(app_key_buf.ak));
    app_key_buf.ck=0;
    app_key_buf.sk=0;
   
    tmemset((unsigned char *)&app_key_buf.tk_bits[0], 0, ALL_KEY_BUF_SIZE);
    for(i=0; i < app_key_buf.cnt; i++)
    {
        key=app_key_buf.keycode[i];
        if(proc_hotkey(key) == 0)
        {
            index=key/8;
            bit=key%8;
            app_key_buf.tk_bits[index]|=1<<bit;
        }
    }
    //get normal kb pipe  key bits  and all kb pipe key bits
    int cnt=0;
    tmemset((unsigned char *)&app_key_buf.nk[2], 0, 6);

    for( i=0;i<ALL_KEY_BUF_SIZE;i++)
    {
        for(int k=0;k<8;k++)
        {
            
            if(app_key_buf.tk_bits[i]&(1<<k))
            {
                if(app_key_buf.nk_bits[i]&(1<<k))
                {
                    
                }
                else if(app_key_buf.ak_bits[i]&(1<<k))
                {
                    
                }
                else if(nk_cnt<6)
                {
                    app_key_buf.nk_bits[i]|=(1<<k);
                    nk_cnt++;
                }
                else 
                {
                    app_key_buf.ak_bits[i]|=(1<<k);
                }
            }
            else
            {
                if(app_key_buf.nk_bits[i]&(1<<k))
                {
                    nk_cnt--;
                    app_key_buf.nk_bits[i]&=0xff-(1<<k);
                }
                else if(app_key_buf.ak_bits[i]&(1<<k))
                {
                    app_key_buf.ak_bits[i]&=0xff-(1<<k);
                }
            }

            if(cnt<6)
            {
                if(app_key_buf.nk_bits[i]&(1<<k))
                {
                    app_key_buf.nk[2+cnt]=i*8+k;
                    cnt++;
                }
            }
        
        }
    }

    nk_cnt=cnt;

    for(i=0;i<8;i++)
    {
        if(nk_last[i] != app_key_buf.nk[i])
        {
            for (int j = 0; j < 8; j++) {
                if (app_key_buf.nk[j] != 0) {
                    nk_flg = true;
                }
            }
            tmemcpy(nk_last,app_key_buf.nk,8);
            unsigned int r = core_interrupt_disable();
            tpsll_send_keyboard_data(&app_key_buf.nk[0], 8, NORMAL_KB_DATA_CMD, NULL, NULL);
            // tlkapi_send_string_data(APP_LOG_EN, "n-kb", (unsigned char *)&app_key_buf.nk[0], 8);
            core_restore_interrupt(r);
            break;  
        }
    }

    for(i=0;i<ALL_KEY_BUF_SIZE;i++)
    {
        if(ak_last[i] != app_key_buf.ak_bits[i])
        {
            for (int j = 0; j < ALL_KEY_BUF_SIZE; j++) {
                if (app_key_buf.ak_bits[j] != 0) {
                    ak_flg = true;
                }
            }
            tmemcpy(ak_last,app_key_buf.ak_bits,ALL_KEY_BUF_SIZE);
            unsigned int r = core_interrupt_disable();
            tpsll_send_keyboard_data(&app_key_buf.ak_bits[0], ALL_KEY_BUF_SIZE, ALL_KB_DATA_CMD, NULL, NULL);
            // tlkapi_send_string_data(APP_LOG_EN, "akb", (unsigned char *)&app_key_buf.ak_bits[0], ALL_KEY_BUF_SIZE);
            core_restore_interrupt(r);
            break;
        }
    }

    if(ck_last != app_key_buf.ck)
    {
        ck_last=app_key_buf.ck;
        unsigned short temp=0;
        if(ck_last!=0)
        {
            temp=consumer_list[ck_last-C_INDEX_START];
        }
        else
        {
            temp=0;
        }
        //has_new_report|=HAS_CONSUMER_REPORT;
        unsigned int r = core_interrupt_disable();
        tpsll_send_keyboard_data((unsigned char *)&temp, 2, CONSUME_KB_DATA_CMD, NULL, NULL);
        // tlkapi_send_string_data(APP_LOG_EN, "ckb", (unsigned char *)&temp, 2);
        core_restore_interrupt(r);
    }

    if(sk_last != app_key_buf.sk)
    {
        sk_last = app_key_buf.sk;
       // has_new_report |= HAS_SYSYTEM_REPORT;
        unsigned char temp=1<<(sk_last-S_SLEEP);
        unsigned int r = core_interrupt_disable();
        tpsll_send_keyboard_data((unsigned char *)&temp, 1, SYSTEM_KB_DATA_CMD, NULL, NULL);
        // tlkapi_send_string_data(APP_LOG_EN, "skb", (unsigned char *)&temp, 1);
        core_restore_interrupt(r);
    }

    if(app_ctx.report_rate & REPORT_RATE_8K)
    {
        //Only one piece of data is retained in the FIFO.
        unsigned int r = core_interrupt_disable();
        while(pp_fifo_get_num(&d25fKbTxFifo) > 1)
        {
            if ((nk_flg == false) || (ak_flg == false))
            {
                break;
            }
            pp_fifo_pop(&d25fKbTxFifo);
        }
        core_restore_interrupt(r);
    }

}
#if ALG_KEYSCAN_APP_FUN_ENABLE

#define TOTAL_ROW                       8
#define TOTAL_COL                      16

#if NEW_HW_KEYBOARD_EN
unsigned char map_window[TOTAL_COL][TOTAL_ROW] = {\
       /*R00*/      /*R01*/     /*R02*/     /*R03*/     /*R04*/     /*R05*/      /*R06*/     /*R07*/
/*C00*/{0,          0,          0,          0,          KB_F6,      KB_6,        KB_Y,       KB_H},\
/*C01*/{KB_F5,      KB_5,       KB_T,       KB_G,       KB_F4,      KB_4,        KB_R,       KB_F},\
/*C02*/{KB_F3,      KB_3,       KB_E,       KB_D,       KB_F2,      KB_2,        KB_W,       KB_S},\
/*C03*/{KB_F1,      KB_1,       KB_Q,       KB_A,       KB_Esc,     KB_dunhao,   KB_Tab,     KB_Caps},\
/*C04*/{KB_Delete,  KB_Num,     KP_7,       KP_5,       KB_Home,    KB_Back,     KB_xiegang, KP_4},\
/*C05*/{KB_F12,     KB_denghao, KB_Rguohao, KB_Enter,   KB_F11,     KB_jianhao,  KB_Lguohao, KB_yinhao},\
/*C06*/{KB_F10,     KB_0,       KB_P,       KB_fenhao,  KB_F9,      KB_9,        KB_O,       KB_L},\
/*C07*/{KB_F8,      KB_8,       KB_I,       KB_K,       KB_F7,      KB_7,        KB_U,       KB_J},\
 
/*C08*/{0,          0,          0,          0,          KB_N,       KB_RCtrl,    KB_PgUp,    0},\
/*C09*/{KB_B,       T_FN,       KB_Insert,  0,          KB_V,       KB_RAlt,     KB_PgDown,  0},\
/*C10*/{KB_C,       KB_Space,   KP_jianhao, 0,          KB_X,       KB_LAlt,     KP_jiahao,  0},\
/*C11*/{KB_Z,       KB_LWin,    KP_9,       0,          KB_LShift,  KB_LCtrl,    KP_8,       0},\
/*C12*/{KP_2,       KP_6,       KP_chuhao,  0,          KP_1,       KP_3,        KP_chenghao,0},\
/*C13*/{KB_Up,      KP_enter,   0,          0,          KB_RShift,  KP_Del,      0,          0},\
/*C14*/{KB_wenhao,  KP_0,       0,          0,          KB_juhao,   KB_Right,    0,          0},\
/*C15*/{KB_douhao,  KB_Down,    0,          0,          KB_M,       KB_Left,     0,          0},\
};
#else
unsigned char map_window[TOTAL_COL][TOTAL_ROW] = {\
       /*R00*/     /*R01*/     /*R02*/     /*R03*/    /*R04*/     /*R05*/      /*R06*/     /*R07*/
/*C00*/{0,         0,          0,          0,         KB_Delete,  KB_Insert,   KB_PgUp,    KB_PgDown},\
/*C01*/{KB_Home,   KB_xiegang, KB_Back,    KB_Num,    KB_F12,     KB_Rguohao,  KB_denghao, KB_Enter},\
/*C02*/{KB_F11,    KB_Lguohao, KB_jianhao, KB_yinhao, KB_F10,     KB_P,        KB_0,       KB_fenhao},\
/*C03*/{KB_F9,     KB_O,       KB_9,       KB_L,      KB_F8,      KB_I,        KB_8,       KB_K},\
/*C04*/{KB_Esc,    KB_Tab,     KB_dunhao,  KB_Caps,   KB_F1,      KB_Q,        KB_1,       KB_A},\
/*C05*/{KB_F2,     KB_W,       KB_2,       KB_S,      KB_F3,      KB_E,        KB_3,       KB_D},\
/*C06*/{KB_F4,     KB_R,       KB_4,       KB_F,      KB_F5,      KB_T,        KB_5,       KB_G},\
/*C07*/{KB_F6,     KB_Y,       KB_6,       KB_H,      KB_F7,      KB_U,        KB_7,       KB_J},\
 
/*C08*/{0,         0,          0,          0,         KB_M,       KP_6,        KB_Right,   0},\
/*C09*/{KP_4,      KP_enter,   KB_Down,    0,         KB_Up,      KP_3,        KB_Left,    0},\
/*C10*/{KB_RShift, KP_2,       KB_RCtrl,   0,         KB_wenhao,  KP_1,        T_FN,       0},\
/*C11*/{KB_juhao,  KP_Del,     KB_RAlt,    0,         KB_douhao,  KP_0,        KB_Space,   0},\
/*C12*/{KB_B,      KP_chuhao,  KB_LAlt,    0,         KB_V,       KP_chenghao, KB_LWin,    0},\
/*C13*/{KB_C,      KP_jianhao, 0,          0,         KB_X,       KP_jiahao,   0,          0},\
/*C14*/{KB_Z,      KP_9,       0,          0,         KB_LShift,  KP_8,        0,          0},\
/*C15*/{KB_LCtrl,  KP_7,       0,          0,         KB_N,       KP_5,        0,          0},\
};
#endif

unsigned char hw_now_bits[TOTAL_COL];  //hardware bits
unsigned char hw_last_bits[TOTAL_COL];  //hardware bits

_attribute_ram_code_sec_ unsigned char get_key_value( unsigned short *buf,unsigned short release_threshold,unsigned short press_threshold)
{
    if(buf[0] < press_threshold)
    {
        return 1;
    }
    else if(buf[0] > release_threshold)
    {
        return 0;
    }
    else
    {
        return 2;
    }

    return 1;
}

_attribute_ram_code_sec_ unsigned char key_scan(void)
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
                press_flag= get_key_value((unsigned short *)&adc_buffer[8*col+i], ks_ana_threshold.release_threshold, ks_ana_threshold.press_threshold);
           }
             if(press_flag==1)
             {
                hw_now_bits[col]|=(1<<i);
                app_key_buf.press_cnt++;
                key_fifo(map_window[col][i]);
                // tlkapi_printf(APP_LOG_EN, "col=%d,row=%d,adc_buf=0x%0x",col,i,adc_buffer[8*col+i]);
             }
             else if(press_flag==0)
             {
               hw_now_bits[col]&=0xff-(1<<i);
             }
             #if 0
             if((hw_last_bits[col]&(1<<i))!=(hw_now_bits[col]&(1<<i)))
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
     /*
    printk("RALT=%d space=%d LALT=%d  WIN=%d 2=%d 1=%d ESC=%d F1=%d /=%d *=%d\r\n", \
        adc_buffer[90],adc_buffer[94],adc_buffer[98], \
        adc_buffer[102],adc_buffer[42],adc_buffer[38], \
        adc_buffer[32],adc_buffer[36],adc_buffer[97],adc_buffer[101]);
    */
     if(has_new_key_event)
     {
        key_data_handle();
        special_key_event_handle();
     }


    return has_new_key_event;
}
#endif
