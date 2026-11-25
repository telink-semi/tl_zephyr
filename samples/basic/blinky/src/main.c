/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>

#include <i2c.h>

/* uncomment this to have correct response */
// #define I2C_READY_BY_DELAY

static void telink_i2c_print(const char *comment, void *buf, size_t buf_len)
{
	printk("%s[%zu]:", comment, buf_len);
	for (size_t i = 0; i < buf_len; ++i) {
		printk(" %02x", ((uint8_t *)buf)[i]);
	}
	printk("\n");
}

static bool telink_i2c_is_ready(uint8_t addr, uint32_t trials)
{
#ifndef I2C_READY_BY_DELAY
	bool result = true;

	for (uint32_t i = 0; i <= trials; ++i) {
		if (i2c_master_write(addr, NULL, 0) == 1) {
			result = true;
			break;
		}
		k_msleep(1);
	}
	return result;
#else
	k_msleep(trials * 5);
	return true;
#endif
}

static void telink_i2c_test(void)
{
	printk("test started\n");
	/*
	 * pins for TL321, connect SDA & SCL pins of ANT7-T-M24SR64 module
	 */
	i2c_set_pin(GPIO_PC2, GPIO_PC3);
	i2c_master_init();
	i2c_set_master_clk((unsigned char)(sys_clk.pclk * 1000 * 1000 / (4 * 1000000)));
	/*
	 * usual start of M24SR init sequence
	 */
	i2c_master_write(0xac, (uint8_t []){0x52}, 1);

	printk("is ready a %u\n", telink_i2c_is_ready(0xac, 1));

	i2c_master_write(0xac, (uint8_t []){
						    0x02, 0x00, 0xa4, 0x04, 0x00, 0x07, 0xd2, 0x76,
						    0x00, 0x00, 0x85, 0x01, 0x01, 0x00, 0x35, 0xc0
					   }, 16);

	printk("is ready b %u\n", telink_i2c_is_ready(0xac, 1));

	uint8_t buf[5];

	i2c_master_read(0xac, buf, sizeof(buf));

	telink_i2c_print("rx" ,buf, sizeof(buf));
}

int main(void)
{
	telink_i2c_test();
	return 0;
}
