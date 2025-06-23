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

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/gatt.h>

#include "app_public.h"

#include "keyscan_ana.h"

#define LOG_LEVEL LOG_LEVEL_DBG
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(main);


int main(void)
{
	gpio_function_en(GPIO_PH1);
	gpio_output_en(GPIO_PH1);
	gpio_input_dis(GPIO_PH1);

	gpio_function_en(GPIO_PG7);
	gpio_output_en(GPIO_PG7);
	gpio_input_dis(GPIO_PG7);

	gpio_function_en(GPIO_PB4);
	gpio_output_en(GPIO_PB4);
	gpio_input_dis(GPIO_PB4);

	gpio_function_en(GPIO_PB5);
	gpio_output_en(GPIO_PB5);
	gpio_input_dis(GPIO_PB5);


    gpio_set_level(GPIO_PB5, 1);
    gpio_set_level(GPIO_PG7, 1);

	keyboard_comm_init();

	while(1) {

	    public_loop();
		k_sleep(K_MSEC(3));
	}

	return 0;
}
