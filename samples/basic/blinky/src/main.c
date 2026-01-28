/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(adc_test, LOG_LEVEL_INF);

#include <adc.h>
#include <zephyr/drivers/adc.h>

static inline int32_t adc_div_pow2(int32_t val, uint8_t n)
{
	int32_t bias = (val >> 31) & ((1 << n) - 1);

	return (val + bias) >> n;
}

static int adc_get_sample(int32_t *sample, uint8_t oversampling)
{
	int result = -EINVAL;
	size_t samples_num = (size_t)1 << oversampling;

	if (samples_num && samples_num <= INT16_MAX) {
		*sample = 0;
		adc_power_on();
		for (size_t i = 0; i < samples_num; i++) {
			adc_start_sample_nodma();
			while (!adc_get_rxfifo_cnt());
			*sample +=  (int16_t)adc_get_raw_code();
			adc_stop_sample_nodma();
		}
		adc_power_off();
		*sample = adc_div_pow2(*sample, oversampling);
		result = 0;
	}
	return result;
}

int main(void)
{
	LOG_INF("main started");

	adc_init(NDMA_M_CHN);

	for (;;) {
		adc_gpio_cfg_t adc_gpio_cfg_m = {
				.v_ref			=	ADC_VREF_1P2V,
				.pre_scale		=	ADC_PRESCALE_1F4,
				.sample_freq	=	ADC_SAMPLE_FREQ_48K,
				.pin			=	ADC_GPIO_PB1
		};
		/* do not call when adc_power_on */
		adc_gpio_sample_init(ADC_M_CHANNEL, adc_gpio_cfg_m);

		int32_t adc_code;

		if (!adc_get_sample(&adc_code, 7)) {
			LOG_INF("adc %d", adc_code);
			/* 11 bits since upper bit is sign on HW */
			(void) adc_raw_to_millivolts(1260, ADC_GAIN_1_4, 11, &adc_code);
			LOG_INF("adc %d mV", adc_code);
		} else {
			LOG_ERR("adc sample error");
		}
		k_msleep(1000);
	}
	return 0;
}
