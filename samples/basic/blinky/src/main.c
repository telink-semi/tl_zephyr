/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include "soc_flash.c"

static void printk_buf(const char *comment, void *buf, size_t len)
{
    printk("%s[%zu]:", comment, len);
    for (size_t i = 0; i < len; ++i) {
        printk(" %02x ", ((uint8_t *)buf)[i]);
    }
    printk("\n");
}

int main(void)
{
	printk("main started\n");

	static uint8_t data[256];
	const uintptr_t addr = 0x201fec78;

	soc_flash_read(NULL, addr, data, sizeof(data));

	printk_buf("data", data, sizeof(data));

	return 0;
}
