/*
 * Copyright (c) 2026 Telink Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_TL521X_PINCTRL_COMMON_H_
#define ZEPHYR_TL521X_PINCTRL_COMMON_H_

/* IDs for TL521X GPIO functions */
#define TL521X_FUNC_DEFAULT            0
#define TL521X_FUNC_PWM0               1
#define TL521X_FUNC_PWM1               2
#define TL521X_FUNC_PWM2               3
#define TL521X_FUNC_PWM3               4
#define TL521X_FUNC_PWM4               5
#define TL521X_FUNC_PWM5               6
#define TL521X_FUNC_PWM0_N             7
#define TL521X_FUNC_PWM1_N             8
#define TL521X_FUNC_PWM2_N             9
#define TL521X_FUNC_PWM3_N             10
#define TL521X_FUNC_PWM4_N             11
#define TL521X_FUNC_PWM5_N             12
#define TL521X_FUNC_I2C_SCL_IO         13
#define TL521X_FUNC_I2C_SDA_IO         14
#define TL521X_FUNC_DMIC0_CLK          15
#define TL521X_FUNC_DMIC0_DAT_I        16
#define TL521X_FUNC_SDM0_P             17
#define TL521X_FUNC_SDM0_N             18
#define TL521X_FUNC_SDM1_P             19
#define TL521X_FUNC_SDM1_N             20
#define TL521X_FUNC_UART0_CTS_I        21
#define TL521X_FUNC_UART0_RTS          22
#define TL521X_FUNC_UART0_TX           23
#define TL521X_FUNC_UART0_RTX_IO       24
#define TL521X_FUNC_UART1_CTS_I        25
#define TL521X_FUNC_UART1_RTS          26
#define TL521X_FUNC_UART1_TX           27
#define TL521X_FUNC_UART1_RTX_IO       28
#define TL521X_FUNC_UART2_CTS_I        29
#define TL521X_FUNC_UART2_RTS          30
#define TL521X_FUNC_UART2_TX           31
#define TL521X_FUNC_UART2_RTX_IO       32
#define TL521X_FUNC_UART3_CTS_I        33
#define TL521X_FUNC_UART3_RTS          34
#define TL521X_FUNC_UART3_TX           35
#define TL521X_FUNC_UART3_RTX_IO       36
#define TL521X_FUNC_UART4_CTS_I        37
#define TL521X_FUNC_UART4_RTS          38
#define TL521X_FUNC_UART4_TX           39
#define TL521X_FUNC_UART4_RTX_IO       40
#define TL521X_FUNC_I2S2_BCK_IO        41
#define TL521X_FUNC_I2S2_LR0_IO        42
#define TL521X_FUNC_I2S2_DAT0_IO       43
#define TL521X_FUNC_I2S2_LR1_IO        44
#define TL521X_FUNC_I2S2_DAT1_IO       45
#define TL521X_FUNC_I2S2_CLK           46
#define TL521X_FUNC_IR_LEARN_I         47
#define TL521X_FUNC_KEYS12_IO          48
#define TL521X_FUNC_CLK_7816           49
#define TL521X_FUNC_TDI_I              50
#define TL521X_FUNC_TDO_IO             50
#define TL521X_FUNC_TMS_IO             50
#define TL521X_FUNC_TCK_I              50
#define TL521X_FUNC_SSPI_CN_I          51
#define TL521X_FUNC_SSPI_CK_I          52
#define TL521X_FUNC_SSPI_SI_IO         53
#define TL521X_FUNC_SSPI_SO_IO         54
#define TL521X_FUNC_RZ_TX              55
#define TL521X_FUNC_SWM_IO             56
#define TL521X_FUNC_TX_CYC2PA          57
#define TL521X_FUNC_WIFI_DENY_I        58
#define TL521X_FUNC_BT_ACTIVITY        59
#define TL521X_FUNC_BT_STATUS          60
#define TL521X_FUNC_ATSEL_0            61
#define TL521X_FUNC_ATSEL_1            62
#define TL521X_FUNC_ATSEL_2            63
#define TL521X_FUNC_ATSEL_3            64
#define TL521X_FUNC_ATSEL_4            65
#define TL521X_FUNC_ATSEL_5            66
#define TL521X_FUNC_RX_CYC2LNA         67
#define TL521X_FUNC_DBG_PROBE_CLK      68
#define TL521X_FUNC_DBG_BB0            69
#define TL521X_FUNC_CAN0_RX_I          72
#define TL521X_FUNC_CAN0_TX            73
#define TL521X_FUNC_I3C0_SDA_PULLUP_EN 74
#define TL521X_FUNC_I3C0_SDA_IO        75
#define TL521X_FUNC_I3C0_SCL_IO        76
#define TL521X_FUNC_GSPI_CN_IO         77
#define TL521X_FUNC_GSPI_CN0_IO        77
#define TL521X_FUNC_GSPI_IO3_IO        78
#define TL521X_FUNC_GSPI_IO2_IO        79
#define TL521X_FUNC_GSPI_MISO_IO       80
#define TL521X_FUNC_GSPI_MOSI_IO       81
#define TL521X_FUNC_GSPI_CK_IO         82
#define TL521X_FUNC_GSPI1_CN_IO        83
#define TL521X_FUNC_GSPI1_IO3_IO       84
#define TL521X_FUNC_GSPI1_IO2_IO       85
#define TL521X_FUNC_GSPI1_MISO_IO      86
#define TL521X_FUNC_GSPI1_MOSI_IO      87
#define TL521X_FUNC_GSPI1_CK_IO        88

/* IDs for GPIO Ports  */

#define TLX_PORT_A 0x00
#define TLX_PORT_B 0x01
#define TLX_PORT_C 0x02
#define TLX_PORT_D 0x03
#define TLX_PORT_E 0x04
#define TLX_PORT_F 0x05

/* IDs for GPIO Pins */

#define TLX_PIN_0 0x01
#define TLX_PIN_1 0x02
#define TLX_PIN_2 0x04
#define TLX_PIN_3 0x08
#define TLX_PIN_4 0x10
#define TLX_PIN_5 0x20
#define TLX_PIN_6 0x40
#define TLX_PIN_7 0x80

/* TLx pinctrl pull-up/down */

#define TLX_PULL_NONE 0
#define TLX_PULL_DOWN 2
#define TLX_PULL_UP   3

/* Pin function positions */

#define TL521X_PIN_FUNC_POS 0xFF

/* Pin pull up positions */

#define TLX_PIN_0_PULL_UP_EN_POS 0x00
#define TLX_PIN_1_PULL_UP_EN_POS 0x02
#define TLX_PIN_2_PULL_UP_EN_POS 0x04
#define TLX_PIN_3_PULL_UP_EN_POS 0x06
#define TLX_PIN_4_PULL_UP_EN_POS 0x00
#define TLX_PIN_5_PULL_UP_EN_POS 0x02
#define TLX_PIN_6_PULL_UP_EN_POS 0x04
#define TLX_PIN_7_PULL_UP_EN_POS 0x06

/* TL521X pin configuration bit field positions and masks */

#define TLX_PULL_POS    24
#define TLX_PULL_MSK    0x3
#define TLX_FUNC_POS    16
#define TL521X_FUNC_MSK 0xFF
#define TLX_PORT_POS    8
#define TLX_PORT_MSK    0xFF

#define TLX_PIN_POS    0
#define TLX_PIN_MSK    0xFFFF
#define TLX_PIN_ID_MSK 0xFF

/* Setters and getters */

#define TLX_PINMUX_SET(port, pin, func)                                                            \
	((func << TLX_FUNC_POS) | (port << TLX_PORT_POS) | (pin << TLX_PIN_POS))
#define TLX_PINMUX_GET_PULL(pinmux)   ((pinmux >> TLX_PULL_POS) & TLX_PULL_MSK)
#define TLX_PINMUX_GET_FUNC(pinmux)   ((pinmux >> TLX_FUNC_POS) & TL521X_FUNC_MSK)
#define TLX_PINMUX_GET_PIN(pinmux)    ((pinmux >> TLX_PIN_POS) & TLX_PIN_MSK)
#define TLX_PINMUX_GET_PIN_ID(pinmux) ((pinmux >> TLX_PIN_POS) & TLX_PIN_ID_MSK)

#endif /* ZEPHYR_TL521X_PINCTRL_COMMON_H_ */