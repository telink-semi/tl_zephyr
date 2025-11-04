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
    #define HARDWARE_MODULE_SCAN_ENABLE     1
#endif

#define APP_PM_ENABLE                       1

#define BATT_CHECK_ENABLE                   0

#define TOGGLE_DEBUG_IO_ENABLE				1
#define USE_K_TIMER_LOOP                    1
#define USER_K_TIMER_SCAN_LOOP_INTERVAL_MS  50      //50ms

#ifdef __cplusplus
}
#endif
