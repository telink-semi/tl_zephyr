/*
 * Copyright (c) 2026 Telink Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "st25dv.h"
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/gpio.h>

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
static const uint8_t st25dv_chip_ref[] = {0x51, 0x50, 0x50, 0x27, 0x25};

static void st25dv_isr_handler(const struct device *port, struct gpio_callback *cb,
			       gpio_port_pins_t pins);
static void st25dv_work_handler(struct k_work *item);

inline static int st25dv_i2c_reg_read(const struct device *i2c_dev, uint8_t i2c_addr,
				      uint16_t reg_addr, void *data, size_t data_len)
{
	struct i2c_msg msgs[2] = {
		{
			.buf = (uint8_t[]){(uint8_t)(reg_addr >> 8), (uint8_t)reg_addr},
			.len = 2,
			.flags = I2C_MSG_WRITE,
		},
		{
			.buf = data,
			.len = data_len,
			.flags = I2C_MSG_RESTART | I2C_MSG_READ | I2C_MSG_STOP,
		}};

	return i2c_transfer(i2c_dev, msgs, ARRAY_SIZE(msgs), i2c_addr);
}

inline static int st25dv_i2c_reg_write(const struct device *i2c_dev, uint8_t i2c_addr,
				       uint16_t reg_addr, const void *data, size_t data_len)
{
	struct i2c_msg msgs[2] = {
		{
			.buf = (uint8_t[]){(uint8_t)(reg_addr >> 8), (uint8_t)reg_addr},
			.len = 2,
			.flags = I2C_MSG_WRITE,
		},
		{
			.buf = (void *)data,
			.len = data_len,
			.flags = I2C_MSG_RESTART | I2C_MSG_WRITE | I2C_MSG_STOP,
		}};

	return i2c_transfer(i2c_dev, msgs, ARRAY_SIZE(msgs), i2c_addr);
}

#define st25dv_i2c_read_object(i2c_bus, i2c_addr, obj_addr, obj)                                   \
	st25dv_i2c_reg_read(i2c_bus, i2c_addr, obj_addr, &obj, sizeof(obj))

#define st25dv_i2c_write_object(i2c_bus, i2c_addr, obj_addr, obj)                                  \
	st25dv_i2c_reg_write(i2c_bus, i2c_addr, obj_addr, &obj, sizeof(obj))

int st25dv_probe(struct st25dv_data *data)
{
	int result;

	do {
		LOG_DBG("%s %s", st25dv_module_name, __func__);

		uint8_t chip_ref, chip_rev;

		result = st25dv_i2c_read_object(data->i2c_bus, ST25DV_SYSTEM_AREA_ADDR,
						ST25DV_REG_SYS_IC_REF, chip_ref);
		if (result) {
			LOG_ERR("%s read chip id failed %d", st25dv_module_name, result);
			break;
		}
		for (size_t i = 0, result = -ENODEV; i < ARRAY_SIZE(st25dv_chip_ref); ++i) {
			if (chip_ref == st25dv_chip_ref[i]) {
				result = 0;
				break;
			}
		}
		if (result) {
			LOG_ERR("%s invalid chip id %02x", st25dv_module_name, chip_ref);
			break;
		}
		result = st25dv_i2c_read_object(data->i2c_bus, ST25DV_SYSTEM_AREA_ADDR,
						ST25DV_REG_SYS_IC_REV, chip_rev);
		if (result) {
			LOG_ERR("%s read chip id failed %d", st25dv_module_name, result);
			break;
		}
		LOG_INF("%s chip id %02x rev %02x", st25dv_module_name, chip_ref, chip_rev);
	} while (0);

	return result;
}

int st25dv_reset(struct st25dv_data *data)
{
	int result;

	do {
		LOG_DBG("%s %s", st25dv_module_name, __func__);

		uint8_t zero = 0;

		result = st25dv_i2c_write_object(data->i2c_bus, ST25DV_USER_MEMORY_ADDR,
						 ST25DV_REG_USR_MB_CTRL_DYN, zero);
		if (result) {
			LOG_ERR("%s mailbox switching off failed %d", st25dv_module_name, result);
			break;
		}
		result = st25dv_i2c_write_object(data->i2c_bus, ST25DV_USER_MEMORY_ADDR,
						 ST25DV_REG_USR_GPO_DYN, zero);
		if (result) {
			LOG_ERR("%s clear isr config failed %d", st25dv_module_name, result);
			break;
		}
	} while (0);

	return result;
}

int st25dv_init(struct st25dv_data *data)
{
	int result = -EALREADY;

	do {
		LOG_DBG("%s %s", st25dv_module_name, __func__);
		if (data->inited) {
			break;
		}
		result = -ENODEV;
		if (!device_is_ready(data->i2c_bus)) {
			LOG_ERR("%s device %s not ready", st25dv_module_name, data->i2c_bus->name);
			break;
		}
		if (!device_is_ready(data->isr_port)) {
			LOG_ERR("%s device %s not ready", st25dv_module_name, data->isr_port->name);
			break;
		}
		result = st25dv_probe(data);
		if (result) {
			LOG_ERR("%s not detected on i2c bus", st25dv_module_name);
			break;
		}
		result = st25dv_reset(data);
		if (result) {
			LOG_ERR("%s reset failed", st25dv_module_name);
			break;
		}
		k_work_init(&data->work, st25dv_work_handler);
		result = gpio_pin_configure(data->isr_port, data->isr_port_pin,
					    GPIO_INPUT | GPIO_PULL_UP | GPIO_ACTIVE_LOW);
		if (result) {
			LOG_ERR("%s gpio setup failed %d", st25dv_module_name, result);
			break;
		}
		gpio_init_callback(&data->gpio_cb, st25dv_isr_handler, BIT(data->isr_port_pin));
		result = gpio_add_callback(data->isr_port, &data->gpio_cb);
		if (result) {
			LOG_ERR("%s add gpio callback failed %d", st25dv_module_name, result);
			break;
		}
		result = gpio_pin_interrupt_configure(data->isr_port, data->isr_port_pin,
						      GPIO_INT_EDGE_TO_ACTIVE);
		if (result) {
			LOG_ERR("%s gpio isr setup failed %d", st25dv_module_name, result);
			break;
		}

		uint8_t gpo_dyn_reg = 0x11,  /* bit 4: MB_PUT_MSG, bit 0: GPO_Enable */
			mbox_ctl_reg = 0x01; /* bit 0: MB_EN = 1 */

		result = st25dv_i2c_write_object(data->i2c_bus, ST25DV_USER_MEMORY_ADDR,
						 ST25DV_REG_USR_GPO_DYN, gpo_dyn_reg);
		if (result) {
			LOG_ERR("%s gpo register config failed %d", st25dv_module_name, result);
			break;
		}
		result = st25dv_i2c_write_object(data->i2c_bus, ST25DV_USER_MEMORY_ADDR,
						 ST25DV_REG_USR_MB_CTRL_DYN, mbox_ctl_reg);
		if (result) {
			LOG_ERR("%s gpo register config failed %d", st25dv_module_name, result);
			break;
		}
		data->inited = true;
		LOG_DBG("%s init ok", st25dv_module_name);
	} while (0);

	return result;
}

int st25dv_deinit(struct st25dv_data *data)
{
	int result = -EALREADY;

	do {
		LOG_DBG("%s %s", st25dv_module_name, __func__);
		if (!data->inited) {
			break;
		}
		result = st25dv_reset(data);
		if (result) {
			LOG_ERR("%s reset failed", st25dv_module_name);
			break;
		}
		result = gpio_pin_interrupt_configure(data->isr_port, data->isr_port_pin,
						      GPIO_INT_DISABLE);
		if (result) {
			LOG_ERR("%s gpio isr disable failed", st25dv_module_name);
			break;
		}
		result = gpio_remove_callback(data->isr_port, &data->gpio_cb);
		if (result) {
			LOG_ERR("%s rm gpio callback failed", st25dv_module_name);
			break;
		}
		result = gpio_pin_configure(data->isr_port, data->isr_port_pin, GPIO_DISCONNECTED);
		if (result) {
			LOG_ERR("%s gpio disable failed", st25dv_module_name);
			break;
		}

		struct k_work_sync sync_handle;

		(void)k_work_cancel_sync(&data->work, &sync_handle);
		data->inited = false;
		LOG_DBG("%s deinit ok", st25dv_module_name);
	} while (0);

	return result;
}

static void st25dv_isr_handler(const struct device *port, struct gpio_callback *cb,
			       gpio_port_pins_t pins)
{
	struct st25dv_data *data = CONTAINER_OF(cb, struct st25dv_data, gpio_cb);

	/* LOG_DBG("%s isr %s", st25dv_module_name, data->i2c_bus->name); */
	k_work_submit(&data->work);
}

static void st25dv_work_handler(struct k_work *item)
{
	struct st25dv_data *data = CONTAINER_OF(item, struct st25dv_data, work);

	LOG_DBG("%s work %s", st25dv_module_name, data->i2c_bus->name);
	/* TODO: read input mailbox */
}
