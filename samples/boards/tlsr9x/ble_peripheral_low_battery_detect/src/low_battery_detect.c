/* low_battery_detetc.c - VBAT ADC initialization and low battery check */

/*
 * Copyright (c) 2026 Telink Semiconductor (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log_ctrl.h>
#include <zephyr/sys/printk.h>
#include <zephyr/drivers/gpio.h>

#include "drivers.h"

#if defined(CONFIG_BATT_CHECK_ENABLE)

/* The devicetree node identifier for the "led0" alias. */
#define LED_RED_NODE DT_NODELABEL(led_red)

/*
 * A build error on this line means your board is unsupported.
 * See the sample documentation for information on how to fix this.
 */
static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED_RED_NODE, gpios);

#if !DT_NODE_EXISTS(DT_PATH(zephyr_user)) || !DT_NODE_HAS_PROP(DT_PATH(zephyr_user), io_channels)
#error "No suitable devicetree overlay specified"
#endif

/*
 * Battery detection is performed through the ADC io-channel defined in the
 * board overlay (boards/<board>.overlay): a "zephyr,user" node with one
 * io-channel of the ADC controller configured for the internal VBAT input,
 * same pattern as the official adc_dt sample.
 */
static const struct adc_dt_spec batt_adc = ADC_DT_SPEC_GET(DT_PATH(zephyr_user));

/* Sample buffer filled by the ADC driver */
static int16_t batt_sample;

/* ADC sequence: channels/resolution are set from DT by adc_sequence_init_dt() */
static struct adc_sequence batt_seq = {
	.buffer = &batt_sample,
	/* Buffer size in bytes, not number of samples */
	.buffer_size = sizeof(batt_sample),
};

/*
 * The TL521X and TL323X ADC drivers perform the conversion internally and
 * report the measured voltage in mV directly in the sample buffer. All the
 * other Telink ADC drivers (B9x, TL321X, TL322X, TL721X) return raw codes
 * that need to be converted with adc_raw_to_millivolts_dt().
 */
#if DT_NODE_HAS_COMPAT(DT_IO_CHANNELS_CTLR_BY_IDX(DT_PATH(zephyr_user), 0), telink_tl521x_adc) ||  \
	DT_NODE_HAS_COMPAT(DT_IO_CHANNELS_CTLR_BY_IDX(DT_PATH(zephyr_user), 0), telink_tl323x_adc)
#define BATT_RAW_IS_MV 1
#else
#define BATT_RAW_IS_MV 0
#endif

/*
 * Compile-time conversion of a devicetree GPIO spec to the SDK gpio_pin_e
 * encoding (GPIO_GROUPx | BIT(n)). Derived from the TL521X SDK gpio.h:
 * GPIO_GROUPA = 0x000, GPIO_GROUPB = 0x100, ..., GPIO_GROUPF = 0x500, and
 * from the Zephyr GPIO driver port mapping: consecutive port registers,
 * 0x10 apart, starting from gpioa.
 */
#define SDK_GPIO_PIN_BY_IDX(node_id, prop, idx)                                                    \
	(((((uint32_t)DT_REG_ADDR(DT_PHANDLE_BY_IDX(node_id, prop, idx)) -                         \
	    (uint32_t)DT_REG_ADDR(DT_NODELABEL(gpioa))) /                                          \
	   (uint32_t)DT_REG_SIZE(DT_NODELABEL(gpioa)))                                             \
	  << 8) |                                                                                  \
	 BIT(DT_PHA_BY_IDX(node_id, prop, idx, pin)))

/*
 * Deep sleep wakeup pin: GPIO_PC7, driven directly (no key_pool).
 * The pad is biased with the internal pull-down (idles low), so the
 * high-level wake condition is never asserted by a floating pad.
 */
#define BATT_WAKEUP_SDK_PIN GPIO_PC7
#define BATT_WAKEUP_PORT    DEVICE_DT_GET(DT_NODELABEL(gpioc))
#define BATT_WAKEUP_PIN     7

/**
 * @brief Initialize the ADC for battery voltage detection.
 *
 * Configures the ADC io-channel from the devicetree, mirroring the SDK
 * adc_bat_detect_init(). Must be called once before
 * user_battery_power_check().
 */
void adc_bat_detect_init(void)
{
	int ret;

	if (!gpio_is_ready_dt(&led)) {
		printk("GPIO device %s not ready\n", led.port->name);
		return;
	}

	ret = gpio_pin_configure_dt(&led, GPIO_OUTPUT_ACTIVE);
	if (ret < 0) {
		printk("GPIO pin configuration failed (err %d)\n", ret);
		return;
	}

	ret = gpio_pin_set_dt(&led, 0);
	if (ret < 0) {
		printk("GPIO pin set failed (err %d)\n", ret);
		return;
	}

	/*
	 * Bias the wakeup pad: input with internal pull-down so it idles
	 * low (matches the WAKEUP_LEVEL_HIGH wake condition polarity: only
	 * a driven-high pad asserts it).
	 */
	if (!device_is_ready(BATT_WAKEUP_PORT)) {
		printk("GPIO device %s not ready\n", BATT_WAKEUP_PORT->name);
		return;
	}

	ret = gpio_pin_configure(BATT_WAKEUP_PORT, BATT_WAKEUP_PIN, GPIO_INPUT | GPIO_PULL_DOWN);
	if (ret < 0) {
		printk("wakeup pad configuration failed (err %d)\n", ret);
		return;
	}

	/* DEBUG: physical pad level right after pull-down config.
	 * 1 with the pull-down on means the pad is driven high externally
	 * (wiring) and will spuriously wake.
	 */
	printk("boot PC7 raw=%d\n", gpio_pin_get_raw(BATT_WAKEUP_PORT, BATT_WAKEUP_PIN));

	if (!adc_is_ready_dt(&batt_adc)) {
		printk("ADC device %s not ready\n", batt_adc.dev->name);
		return;
	}

	ret = adc_channel_setup_dt(&batt_adc);
	if (ret) {
		printk("ADC VBAT channel setup failed (err %d)\n", ret);
		return;
	}

	ret = adc_sequence_init_dt(&batt_adc, &batt_seq);
	if (ret) {
		printk("ADC channel configuration missing in DT (err %d)\n", ret);
	}
}

/* uint8_t, can save 8 bit info when deep sleep */
#define USED_DEEP_ANA_REG PM_ANA_REG_POWER_ON_CLR_BUF1
#define LOW_BATT_FLG      BIT(0)

/**
 * @brief Check the battery voltage against the alarm threshold.
 *
 * Mirrors the SDK app_battery_power_check(): measures the VBAT voltage
 * through the ADC and compares it with the alarm threshold.
 *
 * @param[in] alarm_vol_mv Low battery alarm threshold in mV
 *
 * @return 0 Battery voltage is below the alarm threshold
 * @return 1 Battery voltage is above the alarm threshold (or read failed)
 */
int user_battery_power_check(uint16_t alarm_vol_mv)
{
	int32_t batt_vol_mv;
	int ret = 0;
	int bat_check_flag = 0;

	ret = adc_read_dt(&batt_adc, &batt_seq);
	if (ret) {
		printk("ADC read failed (err %d)\n", ret);
		return 1;
	}

#if BATT_RAW_IS_MV
	/* The driver reports the measured voltage in mV directly */
	batt_vol_mv = batt_sample;
#else
	batt_vol_mv = batt_sample;

	ret = adc_raw_to_millivolts_dt(&batt_adc, &batt_vol_mv);
	if (ret) {
		printk("Voltage conversion failed (err %d)\n", ret);
		return 1;
	}
#endif

	if (analog_read(USED_DEEP_ANA_REG) & LOW_BATT_FLG) {
		if (batt_vol_mv > alarm_vol_mv + 200) {
			bat_check_flag = 1;
		}
	} else {
		if (batt_vol_mv > alarm_vol_mv) {
			bat_check_flag = 1;
		}
	}

	printk("VBAT: %d mV\n", batt_vol_mv);

	if (bat_check_flag) {
		analog_write_reg8(USED_DEEP_ANA_REG,
				  analog_read_reg8(USED_DEEP_ANA_REG) & (~LOW_BATT_FLG));
	} else {
		for (int i = 0; i < 3; i++) {
			gpio_pin_set_dt(&led, 0);
			k_msleep(200);
			gpio_pin_set_dt(&led, 1);
			k_msleep(200);
		}

		analog_write_reg8(USED_DEEP_ANA_REG,
				  analog_read_reg8(USED_DEEP_ANA_REG) | LOW_BATT_FLG);

		/* Arm PC7 as the deep sleep wakeup pad (high level wakes) */
		cpu_set_gpio_wakeup(BATT_WAKEUP_SDK_PIN, WAKEUP_LEVEL_HIGH, 1);
		/* DEBUG: physical pad level right before deep sleep entry.
		 * A pad reading 1 here asserts the high-level wake condition.
		 */
		printk("sleep PC7 raw=%d\n", gpio_pin_get_raw(BATT_WAKEUP_PORT, BATT_WAKEUP_PIN));
		/* Flush buffered log messages over UART before the reset-bound
		 * deep sleep entry swallows them (deferred log mode)
		 */
		log_flush();
		while (log_process()) {
		}
		cpu_sleep_wakeup_32k_rc(DEEPSLEEP_MODE, PM_WAKEUP_PAD, 0);
	}

	return (batt_vol_mv < alarm_vol_mv) ? 0 : 1;
}

#endif /* CONFIG_BATT_CHECK_ENABLE */
