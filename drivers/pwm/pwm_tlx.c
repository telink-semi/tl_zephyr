/*
 * Copyright (c) 2024 Telink Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT telink_tlx_pwm

#include <pwm.h>
#include <clock.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/pm/device.h>

struct pwm_tlx_channel_state {
	uint32_t period;
	uint32_t pulse;
	pwm_flags_t flags;
};

struct pwm_tlx_config {
	const pinctrl_soc_pin_t *pins;
	uint32_t clock_frequency;
	uint8_t channels;
	uint8_t clk32k_ch_enable;
};

struct pwm_tlx_data {
	uint8_t out_pin_ch_connected;
#ifdef CONFIG_PM_DEVICE
	struct pwm_tlx_channel_state ch[CONFIG_PWM_TELINK_TLX_MAX_CHANNELS];
#endif /* CONFIG_PM_DEVICE */
};

/* API implementation: init */
static int pwm_tlx_init(const struct device *dev)
{
	const struct pwm_tlx_config *config = dev->config;
	uint32_t pwm_clk_div;

	/* Check maximum number of PWM channels */
	if (config->channels > CONFIG_PWM_TELINK_TLX_MAX_CHANNELS) {
		return -EINVAL;
	}

	/* Calculate and check PWM clock divider */
	pwm_clk_div = sys_clk.pclk * 1000 * 1000 / config->clock_frequency - 1;
	if (pwm_clk_div > 255) {
		return -EINVAL;
	}

	/* Set PWM Peripheral clock */
	pwm_set_clk((unsigned char)(pwm_clk_div & 0xFF));

	return 0;
}

/* API implementation: set_cycles */
static int pwm_tlx_set_cycles(const struct device *dev, uint32_t channel,
			      uint32_t period_cycles, uint32_t pulse_cycles,
			      pwm_flags_t flags)
{
	struct pwm_tlx_data *data = dev->data;
	const struct pwm_tlx_config *config = dev->config;

	/* check pwm channel */
	if (channel >= config->channels) {
		return -EINVAL;
	}

	/* check size of pulse and period (2 bytes) */
	if ((period_cycles > 0xFFFFu) ||
	    (pulse_cycles  > 0xFFFFu)) {
		return -EINVAL;
	}

	/* set polarity */
	if (flags & PWM_POLARITY_INVERTED) {
		pwm_set_polarity_en(channel);
	} else {
		pwm_set_polarity_dis(channel);
	}

	/* set pulse and period */
	pwm_set_tcmp(channel, pulse_cycles);
	pwm_set_tmax(channel, period_cycles);

	/* start pwm */
	pwm_start((channel == 0) ? FLD_PWM0_EN : BIT(channel));

	/* switch to 32K */
	if (config->clk32k_ch_enable & BIT(channel)) {
		pwm_32k_chn_en(BIT(channel));
	}

	/* connect output */
	if (config->pins[channel] != UINT32_MAX) {
		const struct pinctrl_state pinctrl_state = {
			.pins = &config->pins[channel],
			.pin_cnt = 1,
			.id = PINCTRL_STATE_DEFAULT,
		};
		const struct pinctrl_dev_config pinctrl = {
			.states = &pinctrl_state,
			.state_cnt = 1,
		};

		if (!pinctrl_apply_state(&pinctrl, PINCTRL_STATE_DEFAULT)) {
			data->out_pin_ch_connected |= BIT(channel);
		} else {
			return -EIO;
		}
	}

#ifdef CONFIG_PM_DEVICE
	data->ch[channel].period = period_cycles;
	data->ch[channel].pulse = pulse_cycles;
	data->ch[channel].flags = flags;
#endif /* CONFIG_PM_DEVICE */

	return 0;
}

/* API implementation: get_cycles_per_sec */
static int pwm_tlx_get_cycles_per_sec(const struct device *dev,
				      uint32_t channel, uint64_t *cycles)
{
	const struct pwm_tlx_config *config = dev->config;

	/* check pwm channel */
	if (channel >= config->channels) {
		return -EINVAL;
	}

	if ((config->clk32k_ch_enable & BIT(channel)) != 0U) {
		*cycles = 32000u;
	} else {
		*cycles = sys_clk.pclk * 1000 * 1000 / (reg_pwm_clkdiv + 1);
	}

	return 0;
}

#ifdef CONFIG_PM_DEVICE
static int pwm_tlx_pm_action(const struct device *dev, enum pm_device_action action)
{
	const struct pwm_tlx_config *config = dev->config;
	struct pwm_tlx_data *data = dev->data;

	extern bool pm_has_resumed_from_deep_sleep_retention(void);
	switch (action) {
	case PM_DEVICE_ACTION_RESUME:
#if CONFIG_SOC_SERIES_RISCV_TELINK_TLX_RETENTION
		if (pm_has_resumed_from_deep_sleep_retention()) {
			pwm_tlx_init(dev);
		}

		for (uint8_t channel = 0; channel < config->channels; channel++) {
			if (data->out_pin_ch_connected & BIT(channel)) {
				pwm_tlx_set_cycles(dev, channel, data->ch[channel].period,
						   data->ch[channel].pulse,
						   data->ch[channel].flags);
			}
		}
	}
#endif /* CONFIG_SOC_SERIES_RISCV_TELINK_TLX_RETENTION */
	break;

	case PM_DEVICE_ACTION_SUSPEND:
#if CONFIG_SOC_SERIES_RISCV_TELINK_TLX_RETENTION
	{
		for (uint8_t channel = 0; channel < config->channels; channel++) {
			if (data->out_pin_ch_connected & BIT(channel)) {
				uint32_t pin = TLX_PINMUX_GET_PIN(config->pins[channel]);

				pwm_stop((channel == 0) ? FLD_PWM0_EN : BIT(channel));
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

/* PWM driver APIs structure */
static const struct pwm_driver_api pwm_tlx_driver_api = {
	.set_cycles = pwm_tlx_set_cycles,
	.get_cycles_per_sec = pwm_tlx_get_cycles_per_sec,
};

/* PWM driver registration */
#define PWM_tlx_INIT(n)                                                 \
                                                                        \
	PM_DEVICE_DT_INST_DEFINE(n, pwm_tlx_pm_action);                     \
                                                                        \
	static const pinctrl_soc_pin_t pwm_tlx_pins##n[] = {                \
		COND_CODE_1(DT_NODE_HAS_PROP(DT_DRV_INST(n), pinctrl_ch0),      \
		(Z_PINCTRL_STATE_PIN_INIT(DT_DRV_INST(n), pinctrl_ch0, 0)),     \
		(UINT32_MAX,))                                                  \
		COND_CODE_1(DT_NODE_HAS_PROP(DT_DRV_INST(n), pinctrl_ch1),      \
		(Z_PINCTRL_STATE_PIN_INIT(DT_DRV_INST(n), pinctrl_ch1, 0)),     \
		(UINT32_MAX,))                                                  \
		COND_CODE_1(DT_NODE_HAS_PROP(DT_DRV_INST(n), pinctrl_ch2),      \
		(Z_PINCTRL_STATE_PIN_INIT(DT_DRV_INST(n), pinctrl_ch2, 0)),     \
		(UINT32_MAX,))                                                  \
		COND_CODE_1(DT_NODE_HAS_PROP(DT_DRV_INST(n), pinctrl_ch3),      \
		(Z_PINCTRL_STATE_PIN_INIT(DT_DRV_INST(n), pinctrl_ch3, 0)),     \
		(UINT32_MAX,))                                                  \
		COND_CODE_1(DT_NODE_HAS_PROP(DT_DRV_INST(n), pinctrl_ch4),      \
		(Z_PINCTRL_STATE_PIN_INIT(DT_DRV_INST(n), pinctrl_ch4, 0)),     \
		(UINT32_MAX,))                                                  \
		COND_CODE_1(DT_NODE_HAS_PROP(DT_DRV_INST(n), pinctrl_ch5),      \
		(Z_PINCTRL_STATE_PIN_INIT(DT_DRV_INST(n), pinctrl_ch5, 0)),     \
		(UINT32_MAX,))                                                  \
	};                                                                  \
                                                                        \
	static const struct pwm_tlx_config config##n = {                    \
		.pins = pwm_tlx_pins##n,                                        \
		.clock_frequency = DT_INST_PROP(n, clock_frequency),            \
		.channels = DT_INST_PROP(n, channels),                          \
		.clk32k_ch_enable = (                                           \
			(DT_INST_PROP(n, clk32k_ch0_enable) << 0U) |                \
			(DT_INST_PROP(n, clk32k_ch1_enable) << 1U) |                \
			(DT_INST_PROP(n, clk32k_ch2_enable) << 2U) |                \
			(DT_INST_PROP(n, clk32k_ch3_enable) << 3U) |                \
			(DT_INST_PROP(n, clk32k_ch4_enable) << 4U) |                \
			(DT_INST_PROP(n, clk32k_ch5_enable) << 5U)                  \
		),                                                              \
	};                                                                  \
                                                                        \
	static struct pwm_tlx_data data##n;                                 \
                                                                        \
	DEVICE_DT_INST_DEFINE(n, pwm_tlx_init,                              \
		PM_DEVICE_DT_INST_GET(n), &data##n, &config##n,                 \
		POST_KERNEL, CONFIG_PWM_INIT_PRIORITY,                          \
		&pwm_tlx_driver_api);

DT_INST_FOREACH_STATUS_OKAY(PWM_tlx_INIT);
