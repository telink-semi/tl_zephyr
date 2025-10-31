/*
 * Copyright (c) 2024 Telink Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_TL323X_PINCTRL_COMMON_H_
#define ZEPHYR_TL323X_PINCTRL_COMMON_H_

/* IDs for TL323X GPIO functions */

#define TL323X_FUNC_DEFAULT            0
#define TL323X_FUNC_PWM0               1
#define TL323X_FUNC_PWM1               2
#define TL323X_FUNC_PWM2               3
#define TL323X_FUNC_PWM3               4
#define TL323X_FUNC_PWM4               5
#define TL323X_FUNC_PWM5               6
#define TL323X_FUNC_PWM0_N             7
#define TL323X_FUNC_PWM1_N             8
#define TL323X_FUNC_PWM2_N             9
#define TL323X_FUNC_PWM3_N             10
#define TL323X_FUNC_PWM4_N             11
#define TL323X_FUNC_PWM5_N             12

/* I2C */
#define TL323X_FUNC_I2C_SCL_IO         13
#define TL323X_FUNC_I2C_SDA_IO         14
#define TL323X_FUNC_I2C1_SDA_IO        15
#define TL323X_FUNC_I2C1_SCL_IO        16

/* UART0 */
#define TL323X_FUNC_UART0_CTS_I        17
#define TL323X_FUNC_UART0_RTS          18
#define TL323X_FUNC_UART0_TX           19
#define TL323X_FUNC_UART0_RTX_IO       20

/* UART1 */
#define TL323X_FUNC_UART1_CTS_I        21
#define TL323X_FUNC_UART1_RTS          22
#define TL323X_FUNC_UART1_TX           23
#define TL323X_FUNC_UART1_RTX_IO       24

/* UART2 */
#define TL323X_FUNC_UART2_CTS_I        25
#define TL323X_FUNC_UART2_RTS          26
#define TL323X_FUNC_UART2_TX           27
#define TL323X_FUNC_UART2_RTX_IO       28

/* UART3 */
#define TL323X_FUNC_UART3_CTS_I        29
#define TL323X_FUNC_UART3_RTS          30
#define TL323X_FUNC_UART3_TX           31
#define TL323X_FUNC_UART3_RTX_IO       32

/* UART4 */
#define TL323X_FUNC_UART4_CTS_I        33
#define TL323X_FUNC_UART4_RTS          34
#define TL323X_FUNC_UART4_TX           35
#define TL323X_FUNC_UART4_RTX_IO       36

/* Misc clock / debug / SPI */
#define TL323X_FUNC_CLK_7816           37
#define TL323X_FUNC_TDI_I              38
#define TL323X_FUNC_TDO_IO             38
#define TL323X_FUNC_TMS_IO             38
#define TL323X_FUNC_TCK_I              38

/* SSPI */
#define TL323X_FUNC_SSPI_CN_I          39
#define TL323X_FUNC_SSPI_CK_I          40
#define TL323X_FUNC_SSPI_SI_IO         41
#define TL323X_FUNC_SSPI_SO_IO         42

/* Misc IOs */
#define TL323X_FUNC_RZ_TX              43
#define TL323X_FUNC_SWM_IO             44
#define TL323X_FUNC_TX_CYC2PA          45
#define TL323X_FUNC_WIFI_DENY_I        46
#define TL323X_FUNC_BT_ACTIVITY        47
#define TL323X_FUNC_BT_STATUS          48

/* ATSEL */
#define TL323X_FUNC_ATSEL_0            49
#define TL323X_FUNC_ATSEL_1            50
#define TL323X_FUNC_ATSEL_2            51
#define TL323X_FUNC_ATSEL_3            52
#define TL323X_FUNC_ATSEL_4            53
#define TL323X_FUNC_ATSEL_5            54

#define TL323X_FUNC_RX_CYC2LNA         55
#define TL323X_FUNC_DBG_PROBE_CLK      56
#define TL323X_FUNC_DBG_BB0            57
#define TL323X_FUNC_DBG_ADC_I_DAT0     58
#define TL323X_FUNC_DBG_TRNG0          59

/* GSPI */
#define TL323X_FUNC_GSPI_CN_IO         60
#define TL323X_FUNC_GSPI_CN0_IO        60
#define TL323X_FUNC_GSPI_IO3_IO        61
#define TL323X_FUNC_GSPI_IO2_IO        62
#define TL323X_FUNC_GSPI_MISO_IO       63
#define TL323X_FUNC_GSPI_MOSI_IO       64
#define TL323X_FUNC_GSPI_CK_IO         65

/* IDs for GPIO Ports  */

#define TLX_PORT_A       0x00
#define TLX_PORT_B       0x01
#define TLX_PORT_C       0x02
#define TLX_PORT_D       0x03
#define TLX_PORT_E       0x04
#define TLX_PORT_F       0x05

/* IDs for GPIO Pins */

#define TLX_PIN_0        0x01
#define TLX_PIN_1        0x02
#define TLX_PIN_2        0x04
#define TLX_PIN_3        0x08
#define TLX_PIN_4        0x10
#define TLX_PIN_5        0x20
#define TLX_PIN_6        0x40
#define TLX_PIN_7        0x80

/* TLx pinctrl pull-up/down */

#define TLX_PULL_NONE    0
#define TLX_PULL_DOWN    2
#define TLX_PULL_UP      3

/* Pin function positions */

#define TL323X_PIN_FUNC_POS    0xFF

/* Pin pull up positions */

#define TLX_PIN_0_PULL_UP_EN_POS    0x00
#define TLX_PIN_1_PULL_UP_EN_POS    0x02
#define TLX_PIN_2_PULL_UP_EN_POS    0x04
#define TLX_PIN_3_PULL_UP_EN_POS    0x06
#define TLX_PIN_4_PULL_UP_EN_POS    0x00
#define TLX_PIN_5_PULL_UP_EN_POS    0x02
#define TLX_PIN_6_PULL_UP_EN_POS    0x04
#define TLX_PIN_7_PULL_UP_EN_POS    0x06

/* TL323X pin configuration bit field positions and masks */

#define TLX_PULL_POS     24
#define TLX_PULL_MSK     0x3
#define TLX_FUNC_POS     16
#define TL323X_FUNC_MSK  0xFF
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
#define TLX_PINMUX_GET_FUNC(pinmux)       ((pinmux >> TLX_FUNC_POS) & TL323X_FUNC_MSK)
#define TLX_PINMUX_GET_PIN(pinmux)        ((pinmux >> TLX_PIN_POS) & TLX_PIN_MSK)
#define TLX_PINMUX_GET_PIN_ID(pinmux)     ((pinmux >> TLX_PIN_POS) & TLX_PIN_ID_MSK)

/* ========== Common Pinmux Decode Helpers for UART ========== */

#define TLX_PINMUX_GET_PORT(pinmux)  (((pinmux) >> TLX_PORT_POS) & TLX_PORT_MSK)
#define GPIO_FC_PORT_SHIFT 8
#define GPIO_FC_SET(port, pin_mask)  (((port) << GPIO_FC_PORT_SHIFT) | (pin_mask))
#define TLX_UART_PINMUX_EXTRACT(uart_node_label, tx_pinmux_var, rx_pinmux_var) \
    const uint32_t tx_pinmux_var = DT_PROP(DT_PHANDLE_BY_IDX(DT_NODELABEL(uart_node_label), pinctrl_0, 0), pinmux); \
    const uint32_t rx_pinmux_var = DT_PROP(DT_PHANDLE_BY_IDX(DT_NODELABEL(uart_node_label), pinctrl_0, 1), pinmux);
#define TLX_PINMUX_TO_GPIO(pinmux_val) \
    GPIO_FC_SET(TLX_PINMUX_GET_PORT(pinmux_val), TLX_PINMUX_GET_PIN_ID(pinmux_val))

#endif  /* ZEPHYR_TL323X_PINCTRL_COMMON_H_ */
