/** @file app_config.h
 *  @brief
 */

/*
 * Copyright (c) 2025 Telink Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifdef __cplusplus
extern "C" {
#endif
///////////////////////// BOARD TYPE SELECT ////////////////////////////////
#define HW_ALG_KEYBOARD                     1 //8*16  analog keyboard

#define HW_BOARD_TYPE                       HW_ALG_KEYBOARD

#define DBG_WITH_EVK_EN             0

#define NEW_HW_KEYBOARD_EN          0

/////////////////////////////////////////////////////////////////////////////
#if (HW_BOARD_TYPE == HW_ALG_KEYBOARD)
    #define ALLOW_SWITCH_BLE_2P4G_MODE      1
    #define ALG_KEYSCAN_APP_FUN_ENABLE      1
    #define USB_APP_FUN_ENABLE              1

    #define CAP_LED_PIN   GPIO_NONE_PIN//GPIO_PB4
    #define NUM_LED_PIN   GPIO_NONE_PIN//GPIO_PB5
    #define MODE_LED_PIN  GPIO_NONE_PIN//GPIO_PH1
    #define PAIR_LED_PIN  GPIO_NONE_PIN//GPIO_PG7

    #define APP_VBUS_CHECK_DE_JT_CNT        2
#endif

#define SPP_TEST_EN                 0

#define REPORT_RATE_TEST_EN         0

#define APP_WDT_ENABLE              0

#ifdef __cplusplus
}
#endif
