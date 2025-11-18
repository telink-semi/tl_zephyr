/** @file
 *  @brief HoG Service sample
 */

/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifdef __cplusplus
extern "C" {
#endif

///////////////////////// BOARD TYPE SELECT ////////////////////////////////
#define HW_PRJ_KEYBOARD                     1 //8*16  analog keyboard
#define HW_DIGIT_KEYBOARD                   2  //digit keyboard

#define HW_BOARD_TYPE                       HW_DIGIT_KEYBOARD

#define HW_EVK_BOARD   0

#if (HW_BOARD_TYPE == HW_PRJ_KEYBOARD )
    #define  ALLOW_SWITCH_BLE_2P4G_MODE     1
    #define  DIGIT_KEYSCAN_FUN_ENABL        0
    #define  ALG_KEYSCAN_APP_FUN_ENABLE     1
    #define USB_APP_FUN_ENABLE              1
#elif(HW_BOARD_TYPE== HW_DIGIT_KEYBOARD)
    #define ALLOW_SWITCH_BLE_2P4G_MODE      0
    #define DIGIT_KEYSCAN_FUN_ENABL         1
    #define ALG_KEYSCAN_APP_FUN_ENABLE      0
    #define USB_APP_FUN_ENABLE              1
    #define HARDWARE_MODULE_SCAN_ENABLE     0

    #define VBUS_5V_CHECK_PIN  GPIO_PC1
    #define VBUS_5V_CHECK_PIN_USB_IN_LEVEL  1
#endif

#define APP_PM_ENABLE                       0

#define BATT_CHECK_ENABLE                   0

#define TOGGLE_DEBUG_IO_ENABLE				0
#define USE_K_TIMER_LOOP                    1
#define USER_K_TIMER_SCAN_LOOP_INTERVAL_MS  50      //50ms

#define D24G_PAIR_TIMER_OUT 			350 //unit 1us
#define D24G_COMMUNICATION_TIMER_OUT	350//240 //unit 1us

#define RF_2P4G_POWER_NORMAL                RF_POWER_INDEX_P5p93dBm
#define RF_2P4G_POWER_PAIR                  RF_POWER_INDEX_P0p08dBm
/////////////////// DEEP SAVE FLG //////////////////////////////////
#define USED_DEEP_ANA_REG                           PM_ANA_REG_POWER_ON_CLR_BUF1 //u8,can save 8 bit info when deep
#define LOW_BATT_FLG                                BIT(0) //if 1: low battery
#define CONN_DEEP_FLG                               BIT(1) //if 1: conn deep, 0: adv deep
#define USED_PAIR_ANA_REG                           PM_ANA_REG_POWER_ON_CLR_BUF2

#ifdef __cplusplus
}
#endif
