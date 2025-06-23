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
#include <inttypes.h>
#include <zephyr/drivers/interrupt_controller/riscv_plic.h>

#include <zephyr/usb/usb_device.h>
#include <zephyr/usb/class/usb_hid.h>

#include "app_public.h"
#include "compiler.h"

//#include "keyscan.h"
//#include "keyscan.c"
#include "timer.c"

#define LOG_LEVEL LOG_LEVEL_DBG
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(kb_matrix);


#if DIGIT_KEYSCAN_FUN_ENABL

#define   ROW_CNT                                   8
#define   COL_CNT                                   18
#if 0
unsigned char map_digit[COL_CNT][ROW_CNT] = {\
/*C00*/{KB_Esc,     KB_dunhao,      KB_Tab,         KB_Caps,        KB_LShift,  KB_LCtrl    },\
/*C01*/{KB_F1,      KB_1,           KB_Q,           KB_A,           0,          KB_LAlt     },\
/*C02*/{KB_F2,      KB_2,           KB_W,           KB_S,           KB_Z,       KB_LWin     },\
/*C03*/{KB_F3,      KB_3,           KB_E,           KB_D,           KB_X,       0           },\
/*C04*/{KB_F4,      KB_4,           KB_R,           KB_F,           KB_C,       0           },\
/*C05*/{KB_F5,      KB_5,           KB_T,           KB_G,           KB_V,       0           },\
/*C06*/{KB_F6,      KB_6,           KB_Y,           KB_H,           KB_B,       KB_Space    },\
/*C07*/{KB_F7,      KB_7,           KB_U,           KB_J,           KB_N,       0           },\
/*C08*/{KB_F8,      KB_8,           KB_I,           KB_K,           KB_M,       0           },\
/*C09*/{KB_F9,      KB_9,           KB_O,           KB_L,           KB_douhao,  0           },\
/*C10*/{KB_F10,     KB_0,           KB_P,           KB_fenhao,      KB_juhao,   KB_RWin     },\
/*C11*/{KB_F11,     KB_jianhao,     KB_Lguohao,     KB_yinhao,      KB_wenhao,  KB_RAlt     },\
/*C12*/{KB_F12,     KB_denghao,     KB_Rguohao,     KB_Enter,       KB_RShift,  T_FN        },\
/*C13*/{KB_F13,     KB_Back,        KB_xiegang,     0,              0,          KB_RCtrl    },\
/*C14*/{KB_F14,     KB_Insert,      KB_Delete,      KP_jiahao,      0,          KB_Left     },\
/*C15*/{KB_F15,     KB_Home,        KB_End,         KP_jianhao,     KB_Up,      KB_Down     },\
/*C16*/{KB_F16,     KB_PgUp,        KB_PgDown,      KB_F20,         0,          KB_Right    },\
/*C17*/{KB_F17,     KB_Num,         KP_7,           KP_4,           KP_1,       KP_0        },\
/*C18*/{KB_F18,     KP_chuhao,      KP_8,           KP_5,           KP_2,       KP_enter    },\
/*C19*/{KB_F19,     KP_chenghao,    KP_9,           KP_6,           KP_3,       KP_Del      },\
       /*R0*/
};
#endif

uint8_t const map_digit[COL_CNT][ROW_CNT] =
{
    {KB_Pause,	S_POWER,	0,	        S_SLEEP,	KB_RCtrl,	KB_RWin,	KB_LCtrl,	KB_F5},
    {KB_Q,		KB_Tab,		KB_A,	    KB_Esc,		KB_Z,		KB_LShift,	KB_dunhao,	KB_1},
    {KB_W,		KB_Caps,	KB_S,		KB_K45,		KB_X,		KB_RShift,	KB_F1,	    KB_2},
    {KB_E,	    KB_F3,		KB_D,		KB_F4,		KB_C,		KB_LWin,	KB_F2,		KB_3},
    {KB_R,		KB_T,		KB_F,		KB_G,		KB_V,		KB_B,	    KB_5,	    KB_4},
    {KB_U,      KB_Y,	    KB_J,		KB_H,	    KB_M,		KB_N,		KB_6,	    KB_7},
    {KB_I,		KB_Rguohao,	KB_K,		KB_F6,      KB_douhao,	KB_K56,		KB_denghao,	KB_8},
    {KB_O,      KB_F7,	    KB_L,		0,	        KB_juhao,	KB_APP,	    KB_F8,	    KB_9},
    {KB_P,		KB_Lguohao,	KB_fenhao,	KB_yinhao,	KB_K42,	    KB_wenhao,	KB_jianhao,	KB_0},
    {KB_Scroll,	KB_K150,	T_FN,		KB_LAlt,	0,	        KB_RAlt,	KB_K151,	KB_PrtSc},
    {KB_K14,	KB_Back,	KB_xiegang,	KB_F11,	    KB_Enter,	KB_F12,		KB_F9,	    KB_F10},
    {KP_7,	    KP_4,		KP_1,		KB_Space,	KB_Num,		KB_Down,    KB_Delete,  S_POWER},
    {KP_8,	    KP_5,		KP_2,		KP_0,	    KP_chuhao,	KB_Right,	KB_Insert,	S_SLEEP},
    {KP_9,		KP_6,		KP_3,		KP_Del,     KP_chenghao,KP_jianhao,	KB_PgUp,    KB_PgDown},
    {KP_jiahao,	KB_K107,    KP_enter,	KB_Up,	    C_Play,	    KB_Left,	KB_Home,	KB_End},
    {0,	        KB_LShift,	KB_RShift,	C_volDown,	C_volUP,	C_nextTrace,C_prevTrace,C_Media},
    {0,	        KB_LWin,	C_wForward,	C_wStop,	C_wBack,	C_wRefresh,	0,			C_wSearch},
    {0,	        0,		    0,		    0,	        C_mute,	    C_computer,	C_wHome,	KB_K151},
};

uint8_t const map_digit_fn[COL_CNT][ROW_CNT] =
{
    {KB_Pause,	S_POWER,	0,	        S_SLEEP,	KB_RCtrl,	KB_RWin,	KB_LCtrl,	C_wRefresh},
    {KB_Q,		KB_Tab,		KB_A,	    KB_Esc,		KB_Z,		KB_LShift,	KB_dunhao,	KB_1},
    {KB_W,		KB_Caps,	KB_S,		KB_K45,		KB_X,		KB_RShift,	C_Media,	KB_2},
    {KB_E,	    C_volUP,	KB_D,		C_mute,		KB_C,		KB_LWin,	C_volDown,	KB_3},
    {KB_R,		KB_T,		KB_F,		KB_G,		KB_V,		KB_B,	    KB_5,	    KB_4},
    {KB_U,      KB_Y,	    KB_J,		KB_H,	    KB_M,		KB_N,		KB_6,	    KB_7},
    {KB_I,		KB_Rguohao,	KB_K,		C_prevTrace,KB_douhao,	KB_K56,		KB_denghao,	KB_8},
    {KB_O,      C_Play,	    KB_L,		0,	        KB_juhao,	KB_APP,	    C_nextTrace,KB_9},
    {KB_P,		KB_Lguohao,	KB_fenhao,	KB_yinhao,	KB_K42,	    KB_wenhao,	KB_jianhao,	KB_0},
    {KB_Scroll,	KB_K150,	T_FN,		KB_LAlt,	0,	        KB_RAlt,	KB_K151,	KB_PrtSc},
    {KB_K14,	KB_Back,	KB_xiegang,	C_calculator,KB_Enter,	KB_F12,		C_email,	C_wHome},
    {KP_7,	    KP_4,		KP_1,		KB_Space,	KB_Num,		KB_Down,    KB_Delete,  S_POWER},
    {KP_8,	    KB_5,		KP_2,		KP_0,	    KP_chuhao,	KB_Right,	KB_Insert,	S_SLEEP},
    {KP_9,		KB_6,		KP_3,		KP_Del,     KP_chenghao,KP_jianhao,	KB_PgUp,    KB_PgDown},
    {KP_jiahao,	KB_K107,    KP_enter,	KB_Up,	    C_Play,	    KB_Left,	KB_Home,	KB_End},
    {0,	        KB_LShift,	KB_RShift,  C_volDown,	C_volUP,	C_nextTrace,C_prevTrace,C_Media},
    {0,	        KB_LWin,	C_wForward,	C_wStop,	C_wBack,	C_wRefresh,	0,			C_wSearch},
    {0,	        0,		    0,		    0,	        C_mute,	    C_computer,	C_wHome,	KB_K151},
};


static  unsigned int last_result[ROW_CNT]={0};
static  unsigned int debug_result[ROW_CNT]={0};

static KEY_MATRIX_DEFINE(key_matrix);

// 全局变量
static struct k_timer scan_timer;
static bool scanning_active = false;


static unsigned int  last_scan_result = 0;
static unsigned int flag_count = 0;
int debounce_cnt = 0;
static uint32_t key_change_tick = 0;
static unsigned char need_debounce_flag=0;

void digit_keyscan_init(void)
{
    // 初始化按键扫描
    if (matrix_keypad_init() != 0) {
        LOG_ERR("Failed to initialize keypad\n");
        return;
    }

#if (USE_K_TIMER_SCAN_MATRIX)
    k_timer_start(&scan_timer, K_MSEC(SCAN_INTERVAL_MS),
                  K_MSEC(SCAN_INTERVAL_MS));
#else
    //user_timer_init();
#endif

     LOG_INF("Matrix keyscan init\n");
}

_attribute_ram_code_sec_noinline_ void digit_new_key_handle(void)
{
    unsigned int now_bit = 0;
    app_key_buf.press_cnt = 0;

    app_key_buf.cnt = 0;
    app_key_buf.special_key_press_f = 0;

    for(int i = 0; i < ROW_CNT; i++)
    {
        for(int k = 0; k < COL_CNT; k++)
        {
            now_bit = last_result[i]&(1<<k);

            if(now_bit)
            {
                app_key_buf.press_cnt++;
                //printk("x %d, y %d\r\n", k, i);
                key_fifo(map_digit[k][i]);
            }
        }
        debug_result[i]=last_result[i];
    }

    key_data_handle();
    special_key_event_handle();
}

_attribute_ram_code_sec_noinline_ void digit_keyscan_handle(void)
{

    if(need_debounce_flag == 1)
    {
        if ((k_uptime_get_32() - key_change_tick) >= 1)
        {
            need_debounce_flag = 0;
        }
        else
        {
            return;
        }
    }

    unsigned int has_new_evnt_flag = digit_key_soft_scan();

    if(has_new_evnt_flag)
    {
        key_change_tick = k_uptime_get_32();
        if(need_debounce_flag == 0)
        {
            debounce_cnt = 2;
        }
        else
        {
            debounce_cnt = 1;
        }
        need_debounce_flag = 1 - need_debounce_flag;
    }

    if(debounce_cnt)
    {
        debounce_cnt++;
        if(debounce_cnt >= 4)
        {
             debounce_cnt = 0;
             digit_new_key_handle();
        }
    }
}

_attribute_ram_code_sec_noinline_ unsigned int get_scan_gpio_value(void)
{
    unsigned int value = 0;

    for(int col = 0; col < COL_CNT; col++)
    {
        if(gpio_pin_get_dt(&key_matrix.col[col]) == 0)
        {
            BIT_SET(value, col);
        }
    }
    return value;
}

_attribute_ram_code_sec_noinline_ unsigned int digit_key_soft_scan(void)
{
    unsigned int scan_result = 0;
    unsigned int has_key_change = 0;

    scan_result = get_scan_gpio_value();//50us

    flag_count++;
    if (last_scan_result != scan_result)
    {
        last_scan_result = scan_result;
        flag_count=0;
    }
    if ((flag_count > 2) && (scan_result == 0))
    {
        flag_count = 0X3F;
        return 0;
    }

    for(int i = 0; i < ROW_CNT; i++)
    {
        gpio_pin_set_dt(&key_matrix.row[i], 1);
    }

    for(int i = 0; i < ROW_CNT; i++)
    {
        gpio_pin_set_dt(&key_matrix.row[i], 0);
        k_busy_wait(2);
        scan_result = get_scan_gpio_value();

        gpio_pin_set_dt(&key_matrix.row[i], 1);
        if(last_result[i] != scan_result)
        {
            last_result[i] = scan_result;
            has_key_change = 1;
        }
    }

    for(int i = 0; i < ROW_CNT; i++)
    {
        gpio_pin_set_dt(&key_matrix.row[i], 0);
    }
    return has_key_change;
}


// 初始化按键扫描
int matrix_keypad_init(void)
{
    int ret;

    // 检查GPIO设备是否就绪
    for (int i = 0; i < ROW_CNT; i++) {
        if (!gpio_is_ready_dt(&key_matrix.row[i])) {
            LOG_ERR("Row GPIO %d not ready", i);
            return -ENODEV;
        }
    }

    for (int i = 0; i < COL_CNT; i++) {
        if (!gpio_is_ready_dt(&key_matrix.col[i])) {
            LOG_ERR("Col GPIO %d not ready", i);
            return -ENODEV;
        }
    }

    // 配置行GPIO为输出
    for (int i = 0; i < ROW_CNT; i++) {
        ret = gpio_pin_configure_dt(&key_matrix.row[i], GPIO_OUTPUT_ACTIVE);
        if (ret < 0) {
            LOG_ERR("Failed to configure row GPIO %d: %d", i, ret);
            return ret;
        }
        gpio_pin_set_dt(&key_matrix.row[i], 0);
    }

    // 配置列GPIO为输入，带上拉
    for (int i = 0; i < COL_CNT; i++) {
        ret = gpio_pin_configure_dt(&key_matrix.col[i],
                                   GPIO_INPUT | GPIO_PULL_UP);
        if (ret < 0) {
            LOG_ERR("Failed to configure col GPIO %d: %d", i, ret);
            return ret;
        }
        gpio_pin_set_dt(&key_matrix.row[i], 1);
    }

#if (USE_K_TIMER_SCAN_MATRIX)
    // 初始化定时器
    k_timer_init(&scan_timer, keyscan_loop, NULL);
#endif

    LOG_INF("Matrix keypad initialized");
    return 0;
}




/* Timer0 interrupt handler */
_attribute_ram_code_sec_ void timer0_isr(void)
{
    if (timer_get_irq_status(FLD_TMR0_MODE_IRQ)){
		timer_clr_irq_status(FLD_TMR0_MODE_IRQ); //Clear IRQ status

        #if (USE_K_TIMER_SCAN_MATRIX)
        keyscan_loop(NULL);
        #else
        keyscan_loop();
        #endif
	}
}

void user_timer_init(void)
{
     /* Timer0 configuration */
    timer_set_init_tick(TIMER0, 0);
    timer_set_cap_tick(TIMER0, 1000 * 10 * sys_clk.pclk * 1);	//10MS
    timer_set_mode(TIMER0, TIMER_MODE_SYSCLK);
    timer_set_irq_mask(FLD_TMR0_MODE_IRQ);
    IRQ_CONNECT(CONFIG_2ND_LVL_ISR_TBL_OFFSET + IRQ_TIMER0, 2, timer0_isr, 0, 0);
    riscv_plic_set_priority(IRQ_TIMER0, 2);
    riscv_plic_irq_enable(IRQ_TIMER0);

     /* Start timers */
	timer_start(TIMER0);
}
#endif
