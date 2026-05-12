/*
 * Copyright (c) 2025 Telink Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "analog.h"
#include "gpio.h"
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/dt-bindings/pinctrl/tl523x-pinctrl.h>

#define DT_DRV_COMPAT telink_tl5x_pinctrl

/**
 *      GPIO Function Enable Register
 *      ADDR                 PINS
 *      gpio_func + 0*0x10:    PORT_A[0-7]
 *      gpio_func + 1*0x10:    PORT_B[0-7]
 *      gpio_func + 2*0x10:    PORT_C[0-7]
 *      gpio_func + 3*0x10:    PORT_D[0-7]
 *      gpio_func + 4*0x10:    PORT_E[0-7]
 */
#define pinctrl_reg_gpio_func(pin)                                                                 \
	(*(volatile uint8_t *)((uint32_t)DT_INST_REG_ADDR_BY_NAME(0, gpio_func) +                  \
			       ((pin >> 8) * 0x10)))

/**
 *      Pull Up resistors enable
 *          ADDR               PINS
 *      pull_up_en:         PORT_A[0-3]
 *      pull_up_en + 1:     PORT_A[4-7]
 *      pull_up_en + 2:     PORT_B[0-3]
 *      pull_up_en + 3:     PORT_B[4-7]
 *      pull_up_en + 4:     PORT_C[0-3]
 *      pull_up_en + 5:     PORT_C[4-7]
 *      pull_up_en + 6:     PORT_D[0-3]
 *      pull_up_en + 7:     PORT_D[4-7]
 *      pull_up_en + 8:     PORT_E[0-3]
 *      pull_up_en + 9:     PORT_E[4-7]
 */
#define pinctrl_reg_pull_up_en(pin)                                                                \
	((uint8_t)(DT_INST_REG_ADDR_BY_NAME(0, pull_up_en) + ((pin >> 8) * 2) +                    \
		   ((pin & 0xf0) ? 1 : 0)))

static int pinctrl_tl5x_init(void)
{
	return 0;
}

SYS_INIT(pinctrl_tl5x_init, PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);

static inline void pinctrl_tl5x_gpio_function_disable(uint32_t pin)
{
	uint8_t bit = pin & 0xff;

	pinctrl_reg_gpio_func(pin) &= ~bit;
}

static inline int pinctrl_tl5x_get_offset(uint32_t pin, uint8_t *offset)
{
	switch (TL5X_PINMUX_GET_PIN_ID(pin)) {
	case TL5X_PIN_0:
		*offset = TL5X_PIN_0_PULL_UP_EN_POS;
		break;
	case TL5X_PIN_1:
		*offset = TL5X_PIN_1_PULL_UP_EN_POS;
		break;
	case TL5X_PIN_2:
		*offset = TL5X_PIN_2_PULL_UP_EN_POS;
		break;
	case TL5X_PIN_3:
		*offset = TL5X_PIN_3_PULL_UP_EN_POS;
		break;
	case TL5X_PIN_4:
		*offset = TL5X_PIN_4_PULL_UP_EN_POS;
		break;
	case TL5X_PIN_5:
		*offset = TL5X_PIN_5_PULL_UP_EN_POS;
		break;
	case TL5X_PIN_6:
		*offset = TL5X_PIN_6_PULL_UP_EN_POS;
		break;
	case TL5X_PIN_7:
		*offset = TL5X_PIN_7_PULL_UP_EN_POS;
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int pinctrl_configure_pin(const pinctrl_soc_pin_t *pinctrl)
{
	int status;
	uint8_t mask;
	uint8_t offset = 0;
	uint8_t pull = TL5X_PINMUX_GET_PULL(*pinctrl);
	uint8_t func = TL5X_PINMUX_GET_FUNC(*pinctrl);
	uint32_t pin = TL5X_PINMUX_GET_PIN(*pinctrl);
	uint8_t pull_up_en_addr = pinctrl_reg_pull_up_en(pin);

	gpio_input_en(pin);

	status = pinctrl_tl5x_get_offset(pin, &offset);
	if (status != 0) {
		return status;
	}

	mask = (uint8_t)~(BIT(offset) | BIT(offset + 1));

	reg_gpio_func_mux(pin) = func;

	pinctrl_tl5x_gpio_function_disable(pin);

	pull = pull << offset;
	analog_write_reg8(pull_up_en_addr, (analog_read_reg8(pull_up_en_addr) & mask) | pull);

	return status;
}

int pinctrl_configure_pins(const pinctrl_soc_pin_t *pins, uint8_t pin_cnt, uintptr_t reg)
{
	ARG_UNUSED(reg);

	int status = 0;

	for (uint8_t i = 0; i < pin_cnt; i++) {
		status = pinctrl_configure_pin(pins++);
		if (status < 0) {
			break;
		}
	}

	return status;
}
