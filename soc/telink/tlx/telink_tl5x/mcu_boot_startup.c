/*
 * Copyright (c) 2024 Telink Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/uart.h>
#include <bootutil/bootutil_log.h>
#include <zephyr/sys/crc.h>


BOOT_LOG_MODULE_REGISTER(telink_tlx_mcuboot);

static bool telink_tlx_mcu_boot_startup(void);

void __wrap_main(void)
{
	if (telink_tlx_mcu_boot_startup()) {
		extern void __real_main(void);

		__real_main();
	}
}

/* Vendor specific code during MCUBoot startup */
static bool telink_tlx_mcu_boot_startup(void)
{
	bool result = true; /* run MCUBoot main */

	return result;
}

void print_buffer(uint8_t *buffer, size_t size)
{
	for (size_t i = 0; i < size; i++) {
		printk("%c", buffer[i]);
	}
}
