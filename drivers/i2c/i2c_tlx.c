/*
 * Copyright (c) 2021-2023 Telink Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT telink_tlx_i2c

#include "i2c.h"
#include "clock.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(i2c_telink, CONFIG_I2C_LOG_LEVEL);

#include <zephyr/drivers/i2c.h>
#include "i2c-priv.h"
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/pm/device.h>

/* I2C configuration structure */
struct i2c_tlx_cfg {
	uint32_t bitrate;
	const struct pinctrl_dev_config *pcfg;
};

/* I2C data structure */
struct i2c_tlx_data {
	struct k_mutex mutex;
	uint32_t dev_config;
};

/* API implementation: configure */
static int i2c_tlx_configure(const struct device *dev, uint32_t dev_config)
{
	struct i2c_tlx_data *data = dev->data;
	uint32_t i2c_speed = 0u;

	/* check address size */
	if (dev_config & I2C_ADDR_10_BITS) {
		LOG_ERR("10-bits address is not supported");
		return -ENOTSUP;
	}

	/* check I2C Master/Slave configuration */
	if (!(dev_config & I2C_MODE_CONTROLLER)) {
		LOG_ERR("I2C slave is not implemented");
		return -ENOTSUP;
	}

	/* check i2c speed */
	switch (I2C_SPEED_GET(dev_config)) {
	case I2C_SPEED_STANDARD:
		i2c_speed = 100000u;
		break;

	case I2C_SPEED_FAST:
		i2c_speed = 400000U;
		break;

	case I2C_SPEED_FAST_PLUS:
	case I2C_SPEED_HIGH:
	case I2C_SPEED_ULTRA:
	default:
		LOG_ERR("Unsupported I2C speed requested");
		return -ENOTSUP;
	}

	k_mutex_lock(&data->mutex, K_FOREVER);
#if CONFIG_SOC_RISCV_TELINK_TL322X || CONFIG_SOC_RISCV_TELINK_TL323X
	i2c_master_init(I2C0);
	i2c_set_master_clk(I2C0, (unsigned char)(sys_clk.pclk * 1000 * 1000 / (4 * i2c_speed)));
#else
	i2c_master_init();
	i2c_set_master_clk((unsigned char)(sys_clk.pclk * 1000 * 1000 / (4 * i2c_speed)));
#endif
	data->dev_config = dev_config;
	k_mutex_unlock(&data->mutex);

	return 0;
}

/* API implementation: transfer */
static int i2c_tlx_transfer(const struct device *dev, struct i2c_msg *msgs, uint8_t num_msgs,
			    uint16_t addr)
{
	int status = 0;
	uint8_t send_stop = 0;
	struct i2c_tlx_data *data = dev->data;

	k_mutex_lock(&data->mutex, K_FOREVER);

	/* loop through all messages */
	for (int i = 0; i < num_msgs; i++) {
		/* check addr size */
		if (msgs[i].flags & I2C_MSG_ADDR_10_BITS) {
			LOG_ERR("10-bits address is not supported");
			k_mutex_unlock(&data->mutex);
			return -ENOTSUP;
		}

		/* config stop bit */
		send_stop = msgs[i].flags & I2C_MSG_STOP ? 1 : 0;
#if CONFIG_SOC_RISCV_TELINK_TL322X || CONFIG_SOC_RISCV_TELINK_TL323X
		i2c_master_send_stop(I2C0, send_stop);
#else
		i2c_master_send_stop(send_stop);

#endif
		/* transfer data */
		if (msgs[i].flags & I2C_MSG_READ) {
#if CONFIG_SOC_RISCV_TELINK_TL322X || CONFIG_SOC_RISCV_TELINK_TL323X
			status = i2c_master_read(I2C0, addr << 1, msgs[i].buf, msgs[i].len);
#else
			status = i2c_master_read(addr << 1, msgs[i].buf, msgs[i].len);

#endif
		} else {
#if CONFIG_SOC_RISCV_TELINK_TL322X || CONFIG_SOC_RISCV_TELINK_TL323X
			status = i2c_master_write(I2C0, addr << 1, msgs[i].buf, msgs[i].len);
#else
			status = i2c_master_write(addr << 1, msgs[i].buf, msgs[i].len);
#endif
		}

		/* check status */
		if (status != 1) {
			LOG_DBG("Failed to transfer I2C messages");
			k_mutex_unlock(&data->mutex);
			return -EIO;
		}
	}

	k_mutex_unlock(&data->mutex);

	return 0;
};

/* API implementation: init */
static int i2c_tlx_init(const struct device *dev)
{
	int status = 0;
	const struct i2c_tlx_cfg *cfg = dev->config;
	struct i2c_tlx_data *data = dev->data;

	data->dev_config = (I2C_MODE_CONTROLLER | i2c_map_dt_bitrate(cfg->bitrate));
	k_mutex_init(&data->mutex);

	/* config i2c on startup */
	status = i2c_tlx_configure(dev, data->dev_config);
	if (status != 0) {
		LOG_ERR("Failed to configure I2C on init");
		return status;
	}
	/* configure pins */
	status = pinctrl_apply_state(cfg->pcfg, PINCTRL_STATE_DEFAULT);
	if (status < 0) {
		LOG_ERR("Failed to configure I2C pins");
		return status;
	}

#ifdef CONFIG_PM_DEVICE
	for (uint8_t i = 0; i < cfg->pcfg->state_cnt; ++i) {
		if (cfg->pcfg->states[i].id == PINCTRL_STATE_DEFAULT) {
			for (uint8_t j = 0; j < cfg->pcfg->states[i].pin_cnt; ++j) {
				const pinctrl_soc_pin_t pin =
					TLX_PINMUX_GET_PIN(cfg->pcfg->states[i].pins[j]);

				gpio_input_en(pin);
			}
		}
	}
#endif

	return 0;
}

#ifdef CONFIG_PM_DEVICE
__GENERIC_SECTION(.ram_code)
static int i2c_tlx_pm_action(const struct device *dev, enum pm_device_action action)
{
	const struct i2c_tlx_cfg *cfg = dev->config;
	struct i2c_tlx_data *data = dev->data;

	switch (action) {
	case PM_DEVICE_ACTION_RESUME:
#if CONFIG_SOC_SERIES_RISCV_TELINK_TLX_RETENTION
		extern bool pm_has_resumed_from_deep_sleep_retention(void);

		if (pm_has_resumed_from_deep_sleep_retention()) {
			i2c_tlx_configure(dev, data->dev_config);
			pinctrl_apply_state(cfg->pcfg, PINCTRL_STATE_DEFAULT);
		}
		for (uint8_t i = 0; i < cfg->pcfg->state_cnt; ++i) {
			if (cfg->pcfg->states[i].id == PINCTRL_STATE_DEFAULT) {
				for (uint8_t j = 0; j < cfg->pcfg->states[i].pin_cnt; ++j) {
					const pinctrl_soc_pin_t pin =
						TLX_PINMUX_GET_PIN(cfg->pcfg->states[i].pins[j]);

					gpio_input_en(pin);
				}
			}
		}
#endif /* CONFIG_SOC_SERIES_RISCV_TELINK_TLX_RETENTION */
		break;

	case PM_DEVICE_ACTION_SUSPEND:
#if CONFIG_SOC_SERIES_RISCV_TELINK_TLX_RETENTION
		for (uint8_t i = 0; i < cfg->pcfg->state_cnt; ++i) {
			if (cfg->pcfg->states[i].id == PINCTRL_STATE_DEFAULT) {
				for (uint8_t j = 0; j < cfg->pcfg->states[i].pin_cnt; ++j) {
					const pinctrl_soc_pin_t pin =
						TLX_PINMUX_GET_PIN(cfg->pcfg->states[i].pins[j]);

					gpio_shutdown(pin);
				}
			}
		}
#endif /* CONFIG_SOC_SERIES_RISCV_TELINK_TLX_RETENTION */
		break;

	default:
		return -ENOTSUP;
	}

	return 0;
}
#endif /* CONFIG_PM_DEVICE */

/* I2C driver APIs structure */
static const struct i2c_driver_api i2c_tlx_api = {
	.configure = i2c_tlx_configure,
	.transfer = i2c_tlx_transfer,
};

BUILD_ASSERT(DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) <= 1, "unsupported I2C instance");

/* I2C driver registration */
#define I2C_TLX_INIT(inst)                                                                         \
                                                                                                   \
	PM_DEVICE_DT_INST_DEFINE(inst, i2c_tlx_pm_action);                                         \
                                                                                                   \
	PINCTRL_DT_INST_DEFINE(inst);                                                              \
                                                                                                   \
	static struct i2c_tlx_data i2c_tlx_data_##inst;                                            \
                                                                                                   \
	static struct i2c_tlx_cfg i2c_tlx_cfg_##inst = {                                           \
		.bitrate = DT_INST_PROP(inst, clock_frequency),                                    \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(inst),                                      \
	};                                                                                         \
                                                                                                   \
	I2C_DEVICE_DT_INST_DEFINE(inst, i2c_tlx_init, PM_DEVICE_DT_INST_GET(inst),                 \
				  &i2c_tlx_data_##inst, &i2c_tlx_cfg_##inst, POST_KERNEL,          \
				  CONFIG_I2C_INIT_PRIORITY, &i2c_tlx_api);

DT_INST_FOREACH_STATUS_OKAY(I2C_TLX_INIT)
