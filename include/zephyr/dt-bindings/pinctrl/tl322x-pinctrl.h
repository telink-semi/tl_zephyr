/*
 * Copyright (c) 2024 Telink Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_TL322X_PINCTRL_COMMON_H_
#define ZEPHYR_TL322X_PINCTRL_COMMON_H_

/* IDs for TL322X GPIO functions */

#define TL322X_FUNC_DEFAULT            0
#define TL322X_FUNC_PWM0               1
#define TL322X_FUNC_PWM4               2
#define TL322X_FUNC_PWM8               3
#define TL322X_FUNC_PWM12              4
#define TL322X_FUNC_PWM16              5
#define TL322X_FUNC_PWM20              6
#define TL322X_FUNC_PWM0_N             7
#define TL322X_FUNC_PWM4_N             8
#define TL322X_FUNC_PWM8_N             9
#define TL322X_FUNC_PWM12_N            10
#define TL322X_FUNC_PWM16_N            11
#define TL322X_FUNC_PWM20_N            12
#define TL322X_FUNC_MSPI_CN2           13
#define TL322X_FUNC_MSPI_CN3           14
#define TL322X_FUNC_MSPI_CN1           15
#define TL322X_FUNC_I2C_SCL_IO         16
#define TL322X_FUNC_I2C_SDA_IO         17
#define TL322X_FUNC_I2C1_SDA_IO        18
#define TL322X_FUNC_I2C1_SCL_IO        19
#define TL322X_FUNC_UART0_CTS_I        20
#define TL322X_FUNC_UART0_RTS          21
#define TL322X_FUNC_UART0_TX           22
#define TL322X_FUNC_UART0_RTX_IO       23
#define TL322X_FUNC_UART1_CTS_I        24
#define TL322X_FUNC_UART1_RTS          25
#define TL322X_FUNC_UART1_TX           26
#define TL322X_FUNC_UART1_RTX_IO       27
#define TL322X_FUNC_UART2_CTS_I        28
#define TL322X_FUNC_UART2_RTS          29
#define TL322X_FUNC_UART2_TX           30
#define TL322X_FUNC_UART2_RTX_IO       31
#define TL322X_FUNC_UART3_CTS_I        32
#define TL322X_FUNC_UART3_RTS          33
#define TL322X_FUNC_UART3_TX           34
#define TL322X_FUNC_UART3_RTX_IO       35
#define TL322X_FUNC_UART4_CTS_I        36
#define TL322X_FUNC_UART4_RTS          37
#define TL322X_FUNC_UART4_TX           38
#define TL322X_FUNC_UART4_RTX_IO       39
#define TL322X_FUNC_CLK_7816           40
#define TL322X_FUNC_I2S0_CLK           41
#define TL322X_FUNC_I2S2_BCK_IO        42
#define TL322X_FUNC_I2S2_LR0_IO        43
#define TL322X_FUNC_I2S2_DAT0_IO       44
#define TL322X_FUNC_I2S2_LR1_IO        45
#define TL322X_FUNC_I2S2_DAT1_IO       46
#define TL322X_FUNC_I2S2_CLK           47
#define TL322X_FUNC_DMIC0_CLK          48
#define TL322X_FUNC_DMIC0_DAT_I        49
#define TL322X_FUNC_SDM0_P             50
#define TL322X_FUNC_SDM0_N             51
#define TL322X_FUNC_SDM1_P             52
#define TL322X_FUNC_SDM1_N             53
#define TL322X_FUNC_IR_LEARN_I         54
#define TL322X_FUNC_SSPI_CN_I          55
#define TL322X_FUNC_SSPI_CK_I          56
#define TL322X_FUNC_SSPI_SI_IO         57
#define TL322X_FUNC_SSPI_SO_IO         58
#define TL322X_FUNC_KEYS0_IO           59
#define TL322X_FUNC_PWM_SYNC_I         60
#define TL322X_FUNC_RZ_TX              61
#define TL322X_FUNC_SWM_IO             62
#define TL322X_FUNC_TX_CYC2PA          63
#define TL322X_FUNC_WIFI_DENY_I        64
#define TL322X_FUNC_BT_ACTIVITY        65
#define TL322X_FUNC_BT_STATUS          66
#define TL322X_FUNC_ATSEL_0            67
#define TL322X_FUNC_ATSEL_1            68
#define TL322X_FUNC_ATSEL_2            69
#define TL322X_FUNC_ATSEL_3            70
#define TL322X_FUNC_ATSEL_4            71
#define TL322X_FUNC_ATSEL_5            72
#define TL322X_FUNC_RX_CYC2LNA         73
#define TL322X_FUNC_DBG_PROBE_CLK      74
#define TL322X_FUNC_DBG_BB0            75
#define TL322X_FUNC_DBG_ADC_I_DAT0     76
#define TL322X_FUNC_LIN0_RX_I          77
#define TL322X_FUNC_LIN0_TX            78
#define TL322X_FUNC_LIN1_RX_I          79
#define TL322X_FUNC_LIN1_TX            80
#define TL322X_FUNC_CAN0_RX_I          81
#define TL322X_FUNC_CAN0_TX            82
#define TL322X_FUNC_CAN1_RX_I          83
#define TL322X_FUNC_CAN1_TX            84
#define TL322X_FUNC_I3C0_SDA_PULLUP_EN 85
#define TL322X_FUNC_I3C0_SDA_IO        86
#define TL322X_FUNC_I3C0_SCL_IO        87
#define TL322X_FUNC_I3C1_SDA_PULLUP_EN 88
#define TL322X_FUNC_I3C1_SDA_IO        89
#define TL322X_FUNC_I3C1_SCL_IO        90
#define TL322X_FUNC_GSPI_CN_IO         91
#define TL322X_FUNC_GSPI_IO3_IO        92
#define TL322X_FUNC_GSPI_IO2_IO        93
#define TL322X_FUNC_GSPI_MISO_IO       94
#define TL322X_FUNC_GSPI_MOSI_IO       95
#define TL322X_FUNC_GSPI_CK_IO         96
#define TL322X_FUNC_GSPI1_CN_IO        97
#define TL322X_FUNC_GSPI1_IO3_IO       98
#define TL322X_FUNC_GSPI1_IO2_IO       99
#define TL322X_FUNC_GSPI1_MISO_IO      100
#define TL322X_FUNC_GSPI1_MOSI_IO      101
#define TL322X_FUNC_GSPI1_CK_IO        102
#define TL322X_FUNC_GSPI2_CN_IO        103
#define TL322X_FUNC_GSPI2_IO3_IO       104
#define TL322X_FUNC_GSPI2_IO2_IO       105
#define TL322X_FUNC_GSPI2_MISO_IO      106
#define TL322X_FUNC_GSPI2_MOSI_IO      107
#define TL322X_FUNC_GSPI2_CK_IO        108
#define TL322X_FUNC_GSPI3_CN_IO        109
#define TL322X_FUNC_GSPI3_IO3_IO       110
#define TL322X_FUNC_GSPI3_IO2_IO       111
#define TL322X_FUNC_GSPI3_MISO_IO      112
#define TL322X_FUNC_GSPI3_MOSI_IO      113
#define TL322X_FUNC_GSPI3_CK_IO        114
#define TL322X_FUNC_GSPI4_CN_IO        115
#define TL322X_FUNC_GSPI4_IO3_IO       116
#define TL322X_FUNC_GSPI4_IO2_IO       117
#define TL322X_FUNC_GSPI4_MISO_IO      118
#define TL322X_FUNC_GSPI4_MOSI_IO      119
#define TL322X_FUNC_GSPI4_CK_IO        120
#define TL322X_FUNC_LSPI_CN_IO         121
#define TL322X_FUNC_LSPI_IO3_IO        122
#define TL322X_FUNC_LSPI_IO2_IO        123
#define TL322X_FUNC_LSPI_MISO_IO       124
#define TL322X_FUNC_LSPI_MOSI_IO       125
#define TL322X_FUNC_LSPI_CK_IO         126

/* Some special aliases (with value 0) */
#define TL322X_FUNC_PA5_USB0_DM_IO     0
#define TL322X_FUNC_PA6_USB0_DP_IO     0
#define TL322X_FUNC_PA7_SWS_IO         0
#define TL322X_FUNC_PC4_TDI_I          0
#define TL322X_FUNC_PC5_TDO_IO         0
#define TL322X_FUNC_PC6_TMS_IO         0
#define TL322X_FUNC_PC7_TCK_I          0
#define TL322X_FUNC_PI0_MSPI_MOSI_IO   0
#define TL322X_FUNC_PI1_MSPI_CK_IO     0
#define TL322X_FUNC_PI2_MSPI_IO3_IO    0
#define TL322X_FUNC_PI3_MSPI_CN_IO     0
#define TL322X_FUNC_PI4_MSPI_MISO_IO   0
#define TL322X_FUNC_PI5_MSPI_IO2_IO    0

/* IDs for GPIO Ports  */

#define TLX_PORT_A       0x00
#define TLX_PORT_B       0x01
#define TLX_PORT_C       0x02
#define TLX_PORT_D       0x03
#define TLX_PORT_E       0x04
#define TLX_PORT_F       0x05
#define TLX_PORT_G       0x06
#define TLX_PORT_H       0x07
#define TLX_PORT_I       0x08

/* IDs for GPIO Pins */

#define TLX_PIN_0        0x01
#define TLX_PIN_1        0x02
#define TLX_PIN_2        0x04
#define TLX_PIN_3        0x08
#define TLX_PIN_4        0x10
#define TLX_PIN_5        0x20
#define TLX_PIN_6        0x40
#define TLX_PIN_7        0x80

/* TLX pinctrl pull-up/down */

#define TLX_PULL_NONE    0
#define TLX_PULL_DOWN    2
#define TLX_PULL_UP      3

/* Pin function positions */

#define TL322X_PIN_FUNC_POS    0xFF

/* Pin pull up positions */

#define TLX_PIN_0_PULL_UP_EN_POS    0x00
#define TLX_PIN_1_PULL_UP_EN_POS    0x02
#define TLX_PIN_2_PULL_UP_EN_POS    0x04
#define TLX_PIN_3_PULL_UP_EN_POS    0x06
#define TLX_PIN_4_PULL_UP_EN_POS    0x00
#define TLX_PIN_5_PULL_UP_EN_POS    0x02
#define TLX_PIN_6_PULL_UP_EN_POS    0x04
#define TLX_PIN_7_PULL_UP_EN_POS    0x06

/* TL322X pin configuration bit field positions and masks */

#define TLX_PULL_POS     24
#define TLX_PULL_MSK     0x3
#define TLX_FUNC_POS     16
#define TL322X_FUNC_MSK  0xFF
#define TLX_PORT_POS     8
#define TLX_PORT_MSK     0xFF

#define TLX_PIN_POS      0
#define TLX_PIN_MSK      0xFFFF
#define TLX_PIN_ID_MSK   0xFF

/* Setters and getters */

#define TLX_PINMUX_SET(port, pin, func)   ((func << TLX_FUNC_POS) | \
					   (port << TLX_PORT_POS) | \
					   (pin << TLX_PIN_POS))
#define TLX_PINMUX_GET_PULL(pinmux)       ((pinmux >> TLX_PULL_POS) & TLX_PULL_MSK)
#define TLX_PINMUX_GET_FUNC(pinmux)       ((pinmux >> TLX_FUNC_POS) & TL322X_FUNC_MSK)
#define TLX_PINMUX_GET_PIN(pinmux)        ((pinmux >> TLX_PIN_POS) & TLX_PIN_MSK)
#define TLX_PINMUX_GET_PIN_ID(pinmux)     ((pinmux >> TLX_PIN_POS) & TLX_PIN_ID_MSK)

#endif  /* ZEPHYR_TL322X_PINCTRL_COMMON_H_ */