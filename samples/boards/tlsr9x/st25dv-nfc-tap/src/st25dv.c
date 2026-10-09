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
#define ST25DV_MBOX_LENGTH_MAX  255

#define ST25DV_REG_SYS_IC_REF      0x0017
#define ST25DV_REG_SYS_IC_REV      0x0020
#define ST25DV_REG_USR_MB_CTRL_DYN 0x2006
#define ST25DV_REG_USR_GPO_DYN     0x2000
#define ST25DV_REG_USR_MB_DATA_DYN 0x2008
#define ST25DV_REG_SYS_I2C_PWD     0x0900
#define ST25DV_REG_SYS_MB_MODE     0x000d
#define ST25DV_REG_SYS_CONFIG      0x0018

static const char *st25dv_module_name = STRINGIFY(MODULE_NAME);
static const uint8_t st25dv_chip_ref[] = {0x51, 0x50};

static void st25dv_isr_handler(const struct device *port, struct gpio_callback *cb,
			       gpio_port_pins_t pins);
static void st25dv_work_handler(struct k_work *item);

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
		result = -ENODEV;
		for (size_t i = 0; i < ARRAY_SIZE(st25dv_chip_ref); ++i) {
			if (chip_ref == st25dv_chip_ref[i]) {
				result = 0;
				break;
			}
		}
		if (result) {
			LOG_ERR("%s invalid chip id %02x", st25dv_module_name, chip_ref);
			break;
		}
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
		LOG_INF("%s chip id %02x rev %02x", st25dv_module_name, chip_ref, chip_rev);
	} while (0);

	return result;
}

int st25dv_unlock(struct st25dv_data *data)
{
	int result;

	do {
		LOG_DBG("%s %s", st25dv_module_name, __func__);

		/* TODO: Set static regs: GPO, MB */
		result = 0;
	} while (0);

	return result;
}

int st25dv_reset(struct st25dv_data *data)
{
	int result;

	do {
		LOG_DBG("%s %s", st25dv_module_name, __func__);
		result = i2c_write(data->i2c_bus,
				   (uint8_t[]){
					   (uint8_t)(ST25DV_REG_USR_MB_CTRL_DYN >> 8),
					   (uint8_t)ST25DV_REG_USR_MB_CTRL_DYN,
					   0x00, /* bit 0: MB_EN = 1 */
				   },
				   3, ST25DV_USER_MEMORY_ADDR);
		if (result) {
			LOG_ERR("%s mailbox switching off failed %d", st25dv_module_name, result);
			break;
		}
		result = i2c_write(data->i2c_bus,
				   (uint8_t[]){
					   (uint8_t)(ST25DV_REG_USR_GPO_DYN >> 8),
					   (uint8_t)ST25DV_REG_USR_GPO_DYN,
					   0x00, /* bit 0: GPO_Enable */
				   },
				   3, ST25DV_USER_MEMORY_ADDR);
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
		result = st25dv_unlock(data);
		if (result) {
			LOG_ERR("%s unlock failed", st25dv_module_name);
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
		result = i2c_write(data->i2c_bus,
				   (uint8_t[]){
					   (uint8_t)(ST25DV_REG_USR_GPO_DYN >> 8),
					   (uint8_t)ST25DV_REG_USR_GPO_DYN,
					   0x01, /* bit 0: GPO_Enable */
				   },
				   3, ST25DV_USER_MEMORY_ADDR);
		if (result) {
			LOG_ERR("%s gpo register config failed %d", st25dv_module_name, result);
			break;
		}
		result = i2c_write(data->i2c_bus,
				   (uint8_t[]){
					   (uint8_t)(ST25DV_REG_USR_MB_CTRL_DYN >> 8),
					   (uint8_t)ST25DV_REG_USR_MB_CTRL_DYN,
					   0x01, /* bit 0: MB_EN = 1 */
				   },
				   3, ST25DV_USER_MEMORY_ADDR);
		if (result) {
			LOG_ERR("%s enable mailbox failed %d", st25dv_module_name, result);
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

int st25dv_mailbox_send(struct st25dv_data *data, const void *buf, size_t buf_len)
{
	int result = -EINVAL;

	do {
		LOG_DBG("%s %s", st25dv_module_name, __func__);
		if (!buf || !buf_len || buf_len > ST25DV_MBOX_LENGTH_MAX) {
			LOG_ERR("%s invalid data send", st25dv_module_name);
			break;
		}
		if (!data->inited) {
			result = -ENODEV;
			LOG_ERR("%s not inited", st25dv_module_name);
			break;
		}

		uint8_t ctrl_reg;

		result = i2c_write_read(data->i2c_bus, ST25DV_USER_MEMORY_ADDR,
					(uint8_t[]){
						(uint8_t)(ST25DV_REG_USR_MB_CTRL_DYN >> 8),
						(uint8_t)ST25DV_REG_USR_MB_CTRL_DYN,
					},
					2, &ctrl_reg, sizeof(ctrl_reg));
		if (result) {
			LOG_ERR("%s mailbox status failed %d", st25dv_module_name, result);
			break;
		}
		if (!(ctrl_reg & 0x01)) { /* bit 0: MB_EN = 1 */
			result = -EIO;
			LOG_ERR("%s mailbox disabled", st25dv_module_name);
			break;
		}
		if (ctrl_reg & 0x06) { /* bit 1: HOST_PUT_MSG = 1, bit 2: RF_PUT_MSG = 1 */
			result = -EBUSY;
			LOG_ERR("%s mailbox busy %x", st25dv_module_name, ctrl_reg & 0x06);
			break;
		}

		uint8_t mbox_data[2 + buf_len];

		mbox_data[0] = (uint8_t)(ST25DV_REG_USR_MB_DATA_DYN >> 8);
		mbox_data[1] = (uint8_t)ST25DV_REG_USR_MB_DATA_DYN;
		memcpy(&mbox_data[2], buf, buf_len);
		result = i2c_write(data->i2c_bus, mbox_data, sizeof(mbox_data),
				   ST25DV_USER_MEMORY_ADDR);
		if (result) {
			LOG_ERR("%s mailbox send failed %d", st25dv_module_name, result);
			break;
		}
		LOG_DBG("%s mailbox send ok", st25dv_module_name);
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
