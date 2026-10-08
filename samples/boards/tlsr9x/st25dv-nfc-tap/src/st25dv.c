/*
 * Copyright (c) 2026 Telink Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "st25dv.h"
#include <zephyr/drivers/i2c.h>

#include <zephyr/logging/log.h>

#define MODULE_NAME st25dv
LOG_MODULE_REGISTER(MODULE_NAME, CONFIG_ST25DV_LOG_LEVEL);

#define ST25DV_USER_MEMORY_ADDR 0x53
#define ST25DV_SYSTEM_AREA_ADDR 0x57

#define ST25DV_REG_SYS_IC_REF      0x0017
#define ST25DV_REG_SYS_IC_REV      0x0020
#define ST25DV_REG_USR_MB_CTRL_DYN 0x2006
#define ST25DV_REG_USR_GPO_DYN     0x2000

static const char *st25dv_module_name = STRINGIFY(MODULE_NAME);

static int st25dv_i2c_write_dyn_reg_safe(const struct device *i2c_dev, uint16_t reg_addr,
					 uint8_t value)
{
	struct i2c_msg msgs[2] = {
		{
			.buf = (uint8_t[]){(uint8_t)(reg_addr >> 8), (uint8_t)reg_addr},
			.len = 2,
			.flags = I2C_MSG_WRITE,
		},
		{
			.buf = (uint8_t[]){value},
			.len = 1,
			.flags = I2C_MSG_WRITE | I2C_MSG_STOP,
		}};
	return i2c_transfer(i2c_dev, msgs, ARRAY_SIZE(msgs), ST25DV_USER_MEMORY_ADDR);
}

int st25dv_probe(struct st25dv_data *data)
{
	int result;

	do {
		LOG_DBG("%s %s", st25dv_module_name, __func__);

		uint8_t chip_ref, chip_rev;

		result = i2c_write_read(data->i2c_bus, ST25DV_SYSTEM_AREA_ADDR,
					(uint8_t[]){
						(uint8_t)(ST25DV_REG_SYS_IC_REF >> 8),
						(uint8_t)ST25DV_REG_SYS_IC_REF,
					},
					2, &chip_ref, sizeof(chip_ref));
		if (result) {
			LOG_ERR("%s read chip id failed %d", st25dv_module_name, result);
			break;
		}
		LOG_INF("%s chip id %02x", st25dv_module_name, chip_ref);
		result = i2c_write_read(data->i2c_bus, ST25DV_SYSTEM_AREA_ADDR,
					(uint8_t[]){
						(uint8_t)(ST25DV_REG_SYS_IC_REV >> 8),
						(uint8_t)ST25DV_REG_SYS_IC_REV,
					},
					2, &chip_rev, sizeof(chip_rev));
		if (result) {
			LOG_ERR("%s read chip id failed %d", st25dv_module_name, result);
			break;
		}
		LOG_INF("%s chip id %02x", st25dv_module_name, chip_rev);
	} while (0);

	return result;
}

int st25dv_reset(struct st25dv_data *data)
{
	int result;

	do {
		LOG_DBG("%s %s", st25dv_module_name, __func__);

		result =
			st25dv_i2c_write_dyn_reg_safe(data->i2c_bus, ST25DV_REG_USR_MB_CTRL_DYN, 0);
		if (result) {
			LOG_ERR("%s mailbox switching off failed %d", st25dv_module_name, result);
			break;
		}
		result = st25dv_i2c_write_dyn_reg_safe(data->i2c_bus, ST25DV_REG_USR_GPO_DYN, 0);
		if (result) {
			LOG_ERR("%s clear isr config failed %d", st25dv_module_name, result);
			break;
		}
	} while (0);

	return result;
}
