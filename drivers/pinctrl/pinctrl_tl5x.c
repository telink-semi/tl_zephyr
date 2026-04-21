/*
 * Copyright (c) 2025 Telink Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/pinctrl.h>
#include <zephyr/dt-bindings/pinctrl/tl523x-pinctrl.h>

#define DT_DRV_COMPAT telink_tl5x_pinctrl

static int pinctrl_tl5x_init(void)
{
	return 0;
}

SYS_INIT(pinctrl_tl5x_init, PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);

static int pinctrl_configure_pin(const pinctrl_soc_pin_t *pinctrl)
{
	ARG_UNUSED(pinctrl);

	return 0;
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