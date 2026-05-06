/*
 * Copyright (c) 2024 Telink Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_TL523X_PINCTRL_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_TL523X_PINCTRL_H_

/* IDs for TL523X GPIO functions */
#define TL523X_FUNC_SWS_IO            0
#define TL523X_FUNC_DM_IO             0
#define TL523X_FUNC_DP_IO             0
#define TL523X_FUNC_MOSI              0
#define TL523X_FUNC_MCLK              0
#define TL523X_FUNC_MSCN              0
#define TL523X_FUNC_MISO              0
#define TL523X_FUNC_PA_KS0_IO         1
#define TL523X_FUNC_DBG0_IO           2
#define TL523X_FUNC_PWM0              3
#define TL523X_FUNC_PWM1              4
#define TL523X_FUNC_PWM2              5
#define TL523X_FUNC_PWM3              6
#define TL523X_FUNC_PWM4              7
#define TL523X_FUNC_PWM5              8
#define TL523X_FUNC_UART0_CTS_I       9
#define TL523X_FUNC_UART0_RTS         10
#define TL523X_FUNC_CLK_7816          11
#define TL523X_FUNC_UART0_RX_I        12
#define TL523X_FUNC_UART0_RTX_IO      13
#define TL523X_FUNC_UART1_CTS_I       14
#define TL523X_FUNC_UART1_RTS         15
#define TL523X_FUNC_UART1_RX_I        16
#define TL523X_FUNC_UART1_RTX_IO      17
#define TL523X_FUNC_I2C_SCL_IO        18
#define TL523X_FUNC_I2C_SDA_IO        19
#define TL523X_FUNC_RX_CYC2LNA        20
#define TL523X_FUNC_TX_CYC2PA         21
#define TL523X_FUNC_SPI_MISO_IO       22
#define TL523X_FUNC_SPI_MOSI_IO       23
#define TL523X_FUNC_SPI_CK_IO         24
#define TL523X_FUNC_SPI_CN_IO         25
#define TL523X_FUNC_DBG_PROBE_CLK     26
#define TL523X_FUNC_WIFI_DENY_I       27
#define TL523X_FUNC_BLE_STATUS        27
#define TL523X_FUNC_BLE_ACTIVITY      27
#define TL523X_FUNC_SWM_IO            28
#define TL523X_FUNC_TDI_I             29
#define TL523X_FUNC_TDO_IO            29
#define TL523X_FUNC_TMS_IO            29
#define TL523X_FUNC_TCK_I             29

/* IDs for GPIO Ports  */
#define TL5X_PORT_A       0x00
#define TL5X_PORT_B       0x01
#define TL5X_PORT_C       0x02
#define TL5X_PORT_D       0x03
#define TL5X_PORT_E       0x04

/* IDs for GPIO Pins */
#define TL5X_PIN_0        0x01
#define TL5X_PIN_1        0x02
#define TL5X_PIN_2        0x04
#define TL5X_PIN_3        0x08
#define TL5X_PIN_4        0x10
#define TL5X_PIN_5        0x20
#define TL5X_PIN_6        0x40
#define TL5X_PIN_7        0x80

/* TL5x pinctrl pull-up/down */
#define TL5X_PULL_NONE    0
/* #define TLX_PULLUP_1M    1 */
#define TL5X_PULL_DOWN    2
#define TL5X_PULL_UP      3

/* Pin function positions， TODO */
#define TL5X_PIN_FUNC_POS    0xFF

/* Pin pull up positions */
#define TL5X_PIN_0_PULL_UP_EN_POS    0x00
#define TL5X_PIN_1_PULL_UP_EN_POS    0x02
#define TL5X_PIN_2_PULL_UP_EN_POS    0x04
#define TL5X_PIN_3_PULL_UP_EN_POS    0x06
#define TL5X_PIN_4_PULL_UP_EN_POS    0x00
#define TL5X_PIN_5_PULL_UP_EN_POS    0x02
#define TL5X_PIN_6_PULL_UP_EN_POS    0x04
#define TL5X_PIN_7_PULL_UP_EN_POS    0x06

/* TL523X pin configuration bit field positions and masks， TODO */
#define TL5X_PULL_POS     24
#define TL5X_PULL_MSK     0x3
#define TL5X_FUNC_POS     16
#define TL5X_FUNC_MSK     0xFF
#define TL5X_PORT_POS     8
#define TL5X_PORT_MSK     0xFF

#define TL5X_PIN_POS      0
#define TL5X_PIN_MSK      0xFFFF
#define TL5X_PIN_ID_MSK   0xFF

/* Setters and getters， TODO */
#define TL5X_PINMUX_SET(port, pin, func)   ((func << TL5X_FUNC_POS) | \
					    (port << TL5X_PORT_POS) | \
					    (pin << TL5X_PIN_POS))
#define TL5X_PINMUX_GET_PULL(pinmux)       ((pinmux >> TL5X_PULL_POS) & TL5X_PULL_MSK)
#define TL5X_PINMUX_GET_FUNC(pinmux)       ((pinmux >> TL5X_FUNC_POS) & TL5X_FUNC_MSK)
#define TL5X_PINMUX_GET_PIN(pinmux)        ((pinmux >> TL5X_PIN_POS) & TL5X_PIN_MSK)
#define TL5X_PINMUX_GET_PIN_ID(pinmux)     ((pinmux >> TL5X_PIN_POS) & TL5X_PIN_ID_MSK)

#endif  /* ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_TL523X_PINCTRL_H_ */
