/*
 * Copyright (c) 2022-2023 Telink Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT telink_tlx_adc

/* Local driver headers */
#define ADC_CONTEXT_USES_KERNEL_TIMER
#include "adc_context.h"

/* Zephyr Device Tree headers */
#if CONFIG_SOC_RISCV_TELINK_TL721X || CONFIG_SOC_RISCV_TELINK_TL321X
#include <zephyr/dt-bindings/adc/tlx-adc.h>
#elif CONFIG_SOC_RISCV_TELINK_TL322X
#include <zephyr/dt-bindings/adc/tl322x-adc.h>
#endif

/* Zephyr Logging headers */
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(adc_tlx, CONFIG_ADC_LOG_LEVEL);

/* Telink HAL headers */
#include <adc.h>
#include <zephyr/drivers/pinctrl.h>

#if CONFIG_SOC_RISCV_TELINK_TL721X || CONFIG_SOC_RISCV_TELINK_TL321X
/* Set ADC resolution value */
static inline void adc_set_resolution(adc_res_e res)
{
	analog_write_reg8(areg_adc_res_m, (analog_read_reg8
		(areg_adc_res_m)&(~FLD_ADC_RES_M)) | res);
}
#elif CONFIG_SOC_RISCV_TELINK_TL322X
static inline void adc_set_resolution(adc_num_e sar_adc_num,adc_res_e res)
{
    analog_write_reg8(areg_adc_res_m(sar_adc_num), (analog_read_reg8(areg_adc_res_m(sar_adc_num) )&(~FLD_ADC_RES_M)) | res);
}
#endif

/* ADC tlx defines */
#define SIGN_BIT_POSITION          (13)
#define AREG_ADC_DATA_STATUS       (0xf6)
#define ADC_DATA_READY             BIT(0)

/* tlx ADC driver data */
struct tlx_adc_data {
	struct adc_context ctx;
	int16_t *buffer;
	int16_t *repeat_buffer;
	uint8_t differential;
	uint8_t resolution_divider;
	struct k_sem acq_sem;
	struct k_thread thread;

	K_KERNEL_STACK_MEMBER(stack, CONFIG_ADC_TLX_ACQUISITION_THREAD_STACK_SIZE);
};

struct tlx_adc_cfg {
	uint32_t sample_freq;
	uint16_t vref_internal_mv;
	const struct pinctrl_dev_config *pcfg;
};

/* Validate ADC data buffer size */
static int adc_tlx_validate_buffer_size(const struct adc_sequence *sequence)
{
	size_t needed = sizeof(int16_t);

	if (sequence->options) {
		needed *= (1 + sequence->options->extra_samplings);
	}

	if (sequence->buffer_size < needed) {
		return -ENOMEM;
	}

	return 0;
}

/* Validate ADC read API input parameters */
static int adc_tlx_validate_sequence(const struct adc_sequence *sequence)
{
	int status;

	if (sequence->channels != BIT(0)) {
		LOG_ERR("Only channel 0 is supported.");
		return -ENOTSUP;
	}

	if (sequence->oversampling) {
		LOG_ERR("Oversampling is not supported.");
		return -ENOTSUP;
	}

	status = adc_tlx_validate_buffer_size(sequence);
	if (status) {
		LOG_ERR("Buffer size too small.");
		return status;
	}

	return 0;
}

#if CONFIG_SOC_RISCV_TELINK_TL322X
static uint16_t adc_tlx_get_pin(uint16_t dt_pin, bool positive)
{
    if (positive) {
        /* adc_input_pch_e */
        switch (dt_pin) {
        case DT_ADC_GPIO_PC0: return ADC0_GPIO_PC0P;
        case DT_ADC_GPIO_PC1: return ADC0_GPIO_PC1P;
        case DT_ADC_GPIO_PC2: return ADC0_GPIO_PC2P;
        case DT_ADC_GPIO_PC3: return ADC0_GPIO_PC3P;
        case DT_ADC_GPIO_PC4: return ADC0_GPIO_PC4P;
        case DT_ADC_GPIO_PC5: return ADC0_GPIO_PC5P;
        case DT_ADC_GPIO_PC6: return ADC0_GPIO_PC6P;
        case DT_ADC_GPIO_PC7: return ADC0_GPIO_PC7P;
        case DT_ADC_VBAT:     return ADC_VBAT_P;  
        default:              return 0;
        }
    } else {
        /* adc_input_nch_e */
        switch (dt_pin) {
        case DT_ADC_GPIO_PC0: return ADC0_GPIO_PC0N;
        case DT_ADC_GPIO_PC1: return ADC0_GPIO_PC1N;
        case DT_ADC_GPIO_PC2: return ADC0_GPIO_PC2N;
        case DT_ADC_GPIO_PC3: return ADC0_GPIO_PC3N;
        case DT_ADC_GPIO_PC4: return ADC0_GPIO_PC4N;
        case DT_ADC_GPIO_PC5: return ADC0_GPIO_PC5N;
        case DT_ADC_GPIO_PC6: return ADC0_GPIO_PC6N;
        case DT_ADC_GPIO_PC7: return ADC0_GPIO_PC7N;
        case DT_ADC_VBAT:     return ADC_GND_N; 
        default:              return 0;
        }
    }
}
#elif CONFIG_SOC_RISCV_TELINK_TL721X || CONFIG_SOC_RISCV_TELINK_TL321X
/* Convert dts pin to tlx SDK pin */
static adc_input_pin_def_e adc_tlx_get_pin(uint8_t dt_pin)
{
	adc_input_pin_def_e adc_pin;

	switch (dt_pin) {
#if CONFIG_SOC_RISCV_TELINK_TL321X
	case DT_ADC_GPIO_PB0:
		adc_pin = ADC_GPIO_PB0;
		break;
#endif
	case DT_ADC_GPIO_PB1:
		adc_pin = ADC_GPIO_PB1;
		break;
	case DT_ADC_GPIO_PB2:
		adc_pin = ADC_GPIO_PB2;
		break;
	case DT_ADC_GPIO_PB3:
		adc_pin = ADC_GPIO_PB3;
		break;
	case DT_ADC_GPIO_PB4:
		adc_pin = ADC_GPIO_PB4;
		break;
	case DT_ADC_GPIO_PB5:
		adc_pin = ADC_GPIO_PB5;
		break;
	case DT_ADC_GPIO_PB6:
		adc_pin = ADC_GPIO_PB6;
		break;
	case DT_ADC_GPIO_PB7:
		adc_pin = ADC_GPIO_PB7;
		break;
#if CONFIG_SOC_RISCV_TELINK_TL321X
	case DT_ADC_GPIO_PD0:
		adc_pin = ADC_GPIO_PD0;
		break;
	case DT_ADC_GPIO_PD1:
		adc_pin = ADC_GPIO_PD1;
		break;
#endif
	case DT_ADC_VBAT:
		adc_pin = ADC_VBAT;
		break;

	default:
		adc_pin = NOINPUTN;
		break;
	}

	return adc_pin;
}
#endif

#if CONFIG_SOC_RISCV_TELINK_TL721X || CONFIG_SOC_RISCV_TELINK_TL321X
/* Get ADC value */
static signed short adc_tlx_get_code(void)
{
	signed short adc_code;

#if CONFIG_SOC_RISCV_TELINK_TL321X
	adc_code = adc_get_code();
#elif CONFIG_SOC_RISCV_TELINK_TL721X
	adc_code = adc_get_raw_code();
#endif
	return adc_code;
}
#elif CONFIG_SOC_RISCV_TELINK_TL322X
/* Get ADC value */
static signed short adc_tlx_get_code(adc_num_e sar_adc_num)
{
	signed short adc_code;
	adc_code = adc_get_raw_code(sar_adc_num);
	return adc_code;
}
#endif

/* ADC Context API implementation: start sampling */
static void adc_context_start_sampling(struct adc_context *ctx)
{
	struct tlx_adc_data *data =
		CONTAINER_OF(ctx, struct tlx_adc_data, ctx);

	data->repeat_buffer = data->buffer;

#if CONFIG_SOC_RISCV_TELINK_TL721X || CONFIG_SOC_RISCV_TELINK_TL321X
	adc_power_on();
#elif CONFIG_SOC_RISCV_TELINK_TL322X
	adc_power_on(ADC0);
#endif

	k_sem_give(&data->acq_sem);
}

/* ADC Context API implementation: buffer pointer */
static void adc_context_update_buffer_pointer(struct adc_context *ctx, bool repeat_sampling)
{
	struct tlx_adc_data *data =
		CONTAINER_OF(ctx, struct tlx_adc_data, ctx);

	if (repeat_sampling) {
		data->buffer = data->repeat_buffer;
	}
}

/* Start ADC measurements */
static int adc_tlx_adc_start_read(const struct device *dev, const struct adc_sequence *sequence)
{
	int status;
	struct tlx_adc_data *data = dev->data;

	/* Validate input parameters */
	status = adc_tlx_validate_sequence(sequence);
	if (status != 0) {
		return status;
	}

	/* Set resolution */
	switch (sequence->resolution) {
#if  CONFIG_SOC_RISCV_TELINK_TL321X || CONFIG_SOC_RISCV_TELINK_TL721X
	case 12:
		adc_set_resolution(ADC_RES12);
		data->resolution_divider = 4;
		break;
	case 10:
		adc_set_resolution(ADC_RES10);
		data->resolution_divider = 16;
		break;
	case 8:
		adc_set_resolution(ADC_RES8);
		data->resolution_divider = 64;
		break;
	default:
		LOG_ERR("Selected ADC resolution is not supported.");
		return -EINVAL;
#elif CONFIG_SOC_RISCV_TELINK_TL322X
	case 12:
		adc_set_resolution(ADC0, ADC_RES12);
		data->resolution_divider = 4;
		break;
	case 10:
		adc_set_resolution(ADC0, ADC_RES10);
		data->resolution_divider = 16;
		break;
	case 8:
		adc_set_resolution(ADC0, ADC_RES8);
		data->resolution_divider = 64;
		break;
	default:
		LOG_ERR("Selected ADC resolution is not supported.");
		return -EINVAL;
#endif		
	}

	/* Save buffer */
	data->buffer = sequence->buffer;

	/* Start ADC conversion */
	adc_context_start_read(&data->ctx, sequence);

	return adc_context_wait_for_completion(&data->ctx);
}

/* Main ADC Acquisition thread */
static void adc_tlx_acquisition_thread(const struct device *dev)
{
	int16_t adc_code = 0;
	struct tlx_adc_data *data = dev->data;

	while (true) {
		/* Wait for Acquisition semaphore */
		k_sem_take(&data->acq_sem, K_FOREVER);
#if CONFIG_SOC_RISCV_TELINK_TL321X || CONFIG_SOC_RISCV_TELINK_TL721X
		adc_start_sample_nodma();
		while (((reg_adc_rxfifo_trig_num & FLD_BUF_CNT) >> 4)
				== 0){
		}
#elif CONFIG_SOC_RISCV_TELINK_TL322X
		adc_start_sample_nodma(ADC0);
		while (((reg_adc_rxfifo_trig_num(ADC0) & FLD_BUF_CNT) >> 4)
				== 0){
		}
#endif

#if  CONFIG_SOC_RISCV_TELINK_TL321X || CONFIG_SOC_RISCV_TELINK_TL721X
		adc_code = adc_tlx_get_code();
#elif CONFIG_SOC_RISCV_TELINK_TL322X
		adc_code = adc_tlx_get_code(ADC0);
#endif
		if (!data->differential) {
			/* Sign bit is not used in case of single-ended configuration */
			adc_code = adc_code * 8;

			/* Do not return negative value for single-ended configuration */
			if (adc_code < 0) {
				adc_code = 0;
			}
		}
		*data->buffer++ = adc_code;

#if CONFIG_SOC_RISCV_TELINK_TL721X || CONFIG_SOC_RISCV_TELINK_TL321X
		/* Power off ADC */
		adc_power_off();
#elif CONFIG_SOC_RISCV_TELINK_TL322X
		adc_power_off(ADC0);
#endif
		/* Release ADC context */
		adc_context_on_sampling_done(&data->ctx, dev);
	}
}

/* ADC Driver initialization */
static int adc_tlx_init(const struct device *dev)
{
	struct tlx_adc_data *data = dev->data;
	const struct tlx_adc_cfg *config = dev->config;
	int err;

	/* Configure dt provided device signals when available */
	err = pinctrl_apply_state(config->pcfg, PINCTRL_STATE_DEFAULT);
	if (err < 0) {
		LOG_ERR("ADC pinctrl setup failed (%d)", err);
		return err;
	}

	k_sem_init(&data->acq_sem, 0, 1);

	k_thread_create(&data->thread, data->stack,
			CONFIG_ADC_TLX_ACQUISITION_THREAD_STACK_SIZE,
			(k_thread_entry_t)adc_tlx_acquisition_thread,
			(void *)dev, NULL, NULL,
			CONFIG_ADC_TLX_ACQUISITION_THREAD_PRIO,
			0, K_NO_WAIT);

	adc_context_unlock_unconditionally(&data->ctx);

	return 0;
}

/* API implementation: read */
static int adc_tlx_read(const struct device *dev,
			const struct adc_sequence *sequence)
{
	int status;
	struct tlx_adc_data *data = dev->data;

	adc_context_lock(&data->ctx, false, NULL);
	status = adc_tlx_adc_start_read(dev, sequence);
	adc_context_release(&data->ctx, status);

	return status;
}

#if CONFIG_SOC_RISCV_TELINK_TL321X || CONFIG_SOC_RISCV_TELINK_TL721X
/* API implementation: channel_setup */
static int adc_tlx_channel_setup(const struct device *dev,
				 const struct adc_channel_cfg *channel_cfg)
{
	adc_ref_vol_e vref_internal_mv;
	adc_sample_freq_e sample_freq;
	adc_pre_scale_e pre_scale;
	adc_sample_cycle_e sample_cycl;
	adc_input_pin_def_e input_positive;
	adc_input_pin_def_e input_negative;
	struct tlx_adc_data *data = dev->data;
	const struct tlx_adc_cfg *config = dev->config;

	/* Check reference */
	if (channel_cfg->reference != ADC_REF_INTERNAL) {
		LOG_ERR("Selected ADC reference is not supported.");
		return -EINVAL;
	}

	/* Check internal reference */
	switch (config->vref_internal_mv) {
#if CONFIG_SOC_RISCV_TELINK_TL321X
	case 900:
		vref_internal_mv = ADC_VREF_0P9V;
		break;
	case 1200:
		vref_internal_mv = ADC_VREF_1P2V;
		break;
#elif CONFIG_SOC_RISCV_TELINK_TL721X
	case 1200:
		vref_internal_mv = ADC_VREF_ANTI_AGING;
		break;
#endif
	default:
		LOG_ERR("Selected reference voltage is not supported.");
		return -EINVAL;
	}

	/* Check sample frequency */
	switch (config->sample_freq) {
	case 23000:
		sample_freq = ADC_SAMPLE_FREQ_23K;
		break;
	case 48000:
		sample_freq = ADC_SAMPLE_FREQ_48K;
		break;
	case 96000:
		sample_freq = ADC_SAMPLE_FREQ_96K;
		break;
	default:
		LOG_ERR("Selected sample frequency is not supported.");
		return -EINVAL;
	}

	/* Check gain */
	switch (channel_cfg->gain) {
	case ADC_GAIN_1:
		pre_scale = ADC_PRESCALE_1;
		break;
	#if CONFIG_SOC_RISCV_TELINK_TL321X || CONFIG_SOC_RISCV_TELINK_TL322X
	case ADC_GAIN_1_4:
		pre_scale = ADC_PRESCALE_1F4;
		break;
	#endif

	default:
		LOG_ERR("Selected ADC gain is not supported.");
		return -EINVAL;
	}

	/* Check acquisition time */
	switch (channel_cfg->acquisition_time) {
	case ADC_ACQ_TIME(ADC_ACQ_TIME_TICKS, 3):
		sample_cycl = ADC_SAMPLE_CYC_3;
		break;
	case ADC_ACQ_TIME_DEFAULT:
	case ADC_ACQ_TIME(ADC_ACQ_TIME_TICKS, 6):
		sample_cycl = ADC_SAMPLE_CYC_6;
		break;
	case ADC_ACQ_TIME(ADC_ACQ_TIME_TICKS, 9):
		sample_cycl = ADC_SAMPLE_CYC_9;
		break;
	case ADC_ACQ_TIME(ADC_ACQ_TIME_TICKS, 12):
		sample_cycl = ADC_SAMPLE_CYC_12;
		break;
	case ADC_ACQ_TIME(ADC_ACQ_TIME_TICKS, 18):
		sample_cycl = ADC_SAMPLE_CYC_18;
		break;
	case ADC_ACQ_TIME(ADC_ACQ_TIME_TICKS, 24):
		sample_cycl = ADC_SAMPLE_CYC_24;
		break;
	case ADC_ACQ_TIME(ADC_ACQ_TIME_TICKS, 36):
		sample_cycl = ADC_SAMPLE_CYC_36;
		break;
	case ADC_ACQ_TIME(ADC_ACQ_TIME_TICKS, 48):
		sample_cycl = ADC_SAMPLE_CYC_48;
		break;

	default:
		LOG_ERR("Selected ADC acquisition time is not supported.");
		return -EINVAL;
	}

	/* Check for valid pins configuration */
	input_positive = adc_tlx_get_pin(channel_cfg->input_positive);
	input_negative = adc_tlx_get_pin(channel_cfg->input_negative);
	if ((input_positive == (uint8_t)ADC_VBAT || input_negative == (uint8_t)ADC_VBAT) &&
		channel_cfg->differential) {
		LOG_ERR("VBAT pin is not available for differential mode.");
		return -EINVAL;
	} else if (channel_cfg->differential && (input_negative == (uint8_t)NOINPUTN)) {
		LOG_ERR("Negative input is not selected.");
		return -EINVAL;
	}

	adc_init(NDMA_M_CHN);

	data->differential = channel_cfg->differential;

	if (channel_cfg->differential) {
		/* Differential pins configuration */
		adc_set_diff_pin(ADC_M_CHANNEL, input_positive, input_negative);
	} else if (input_positive == (uint8_t)ADC_VBAT) {
		/* VBAT pin configuration */
		adc_vbat_sample_init(ADC_M_CHANNEL);
	} else {
		/* Single-ended GPIO pin configuration */
		adc_gpio_cfg_t adc_gpio_cfg_m = {
				.v_ref			=	vref_internal_mv,
				.pre_scale		=	pre_scale,
				.sample_freq		=	sample_freq,
				.pin			=	input_positive,
		};
		adc_gpio_sample_init(ADC_M_CHANNEL, adc_gpio_cfg_m);
	}

	return 0;
}

#elif CONFIG_SOC_RISCV_TELINK_TL322X
/* API implementation: channel_setup */
static int adc_tlx_channel_setup(const struct device *dev,
				 const struct adc_channel_cfg *channel_cfg)
{
	adc_ref_vol_e vref_internal_mv;
	adc_sample_freq_e sample_freq;
	adc_pre_scale_e pre_scale;
	adc_sample_cycle_e sample_cycl;

	adc_input_pch_e input_positive;
	adc_input_nch_e input_negative;
	struct tlx_adc_data *data = dev->data;
	const struct tlx_adc_cfg *config = dev->config;

	/* Check reference */
	if (channel_cfg->reference != ADC_REF_INTERNAL) {
		LOG_ERR("Selected ADC reference is not supported.");
		return -EINVAL;
	}

	/* Check internal reference */
	switch (config->vref_internal_mv) {
	case 1200:
		vref_internal_mv = ADC_VREF_1P2V;
		break;

	default:
		LOG_ERR("Selected reference voltage is not supported.");
		return -EINVAL;
	}

	/* Check sample frequency */
	switch (config->sample_freq) {
	case 23000:
		sample_freq = ADC_SAMPLE_FREQ_23K;
		break;
	case 48000:
		sample_freq = ADC_SAMPLE_FREQ_48K;
		break;
	case 96000:
		sample_freq = ADC_SAMPLE_FREQ_96K;
		break;
	default:
		LOG_ERR("Selected sample frequency is not supported.");
		return -EINVAL;
	}

	/* Check gain */
	switch (channel_cfg->gain) {
	case ADC_GAIN_1:
		pre_scale = ADC_PRESCALE_1;
		break;
	case ADC_GAIN_1_4:
		pre_scale = ADC_PRESCALE_1F4;
		break;

	default:
		LOG_ERR("Selected ADC gain is not supported.");
		return -EINVAL;
	}
	/* Check acquisition time */
	switch (channel_cfg->acquisition_time) {
	case ADC_ACQ_TIME(ADC_ACQ_TIME_TICKS, 4):
		sample_cycl = ADC_SAMPLE_CYC_4;
		break;
	case ADC_ACQ_TIME_DEFAULT:
	case ADC_ACQ_TIME(ADC_ACQ_TIME_TICKS, 6):
		sample_cycl = ADC_SAMPLE_CYC_6;
		break;
	case ADC_ACQ_TIME(ADC_ACQ_TIME_TICKS, 8):
		sample_cycl = ADC_SAMPLE_CYC_8;
		break;
	case ADC_ACQ_TIME(ADC_ACQ_TIME_TICKS, 10):
		sample_cycl = ADC_SAMPLE_CYC_10;
		break;
	case ADC_ACQ_TIME(ADC_ACQ_TIME_TICKS, 12):
		sample_cycl = ADC_SAMPLE_CYC_12;
		break;
	case ADC_ACQ_TIME(ADC_ACQ_TIME_TICKS, 14):
		sample_cycl = ADC_SAMPLE_CYC_14;
		break;
	case ADC_ACQ_TIME(ADC_ACQ_TIME_TICKS, 16):
		sample_cycl = ADC_SAMPLE_CYC_16;
		break;
	case ADC_ACQ_TIME(ADC_ACQ_TIME_TICKS, 18):
		sample_cycl = ADC_SAMPLE_CYC_18;
		break;
	case ADC_ACQ_TIME(ADC_ACQ_TIME_TICKS, 20):
		sample_cycl = ADC_SAMPLE_CYC_20;
		break;
	case ADC_ACQ_TIME(ADC_ACQ_TIME_TICKS, 22):
		sample_cycl = ADC_SAMPLE_CYC_22;
		break;
	case ADC_ACQ_TIME(ADC_ACQ_TIME_TICKS, 24):
		sample_cycl = ADC_SAMPLE_CYC_24;
		break;
	case ADC_ACQ_TIME(ADC_ACQ_TIME_TICKS, 26):
		sample_cycl = ADC_SAMPLE_CYC_26;
		break;
	case ADC_ACQ_TIME(ADC_ACQ_TIME_TICKS, 28):
		sample_cycl = ADC_SAMPLE_CYC_28;
		break;
	case ADC_ACQ_TIME(ADC_ACQ_TIME_TICKS, 30):
		sample_cycl = ADC_SAMPLE_CYC_30;
		break;
	case ADC_ACQ_TIME(ADC_ACQ_TIME_TICKS, 32):
		sample_cycl = ADC_SAMPLE_CYC_32;
		break;
	case ADC_ACQ_TIME(ADC_ACQ_TIME_TICKS, 34):
		sample_cycl = ADC_SAMPLE_CYC_34;
		break;

	default:
		LOG_ERR("Selected ADC acquisition time is not supported.");
		return -EINVAL;
	}

    input_positive = (adc_input_pch_e)adc_tlx_get_pin(channel_cfg->input_positive, true);
    input_negative = (adc_input_nch_e)adc_tlx_get_pin(channel_cfg->input_negative, false);
    if ((input_positive == (uint16_t)ADC_VBAT_P || input_negative == (uint16_t)ADC_GND_N) &&
        channel_cfg->differential) {
        LOG_ERR("VBAT or GND is not available for differential mode.");
        return -EINVAL;
    } else if (channel_cfg->differential && (input_negative == (uint16_t)0)) {
        LOG_ERR("Negative input is not selected.");
        return -EINVAL;
    }

	adc_init(ADC0, NDMA_M_CHN);

	data->differential = channel_cfg->differential;

	if (channel_cfg->differential) {
		/* Differential pins configuration */
		/* The adc_channel_sample_init function has been configured and will not be configured here. */
		// adc_pin_config(input_positive);	
    	// adc_pin_config(input_negative);
		adc_set_diff_input(ADC0, ADC_M_CHANNEL, input_positive, input_negative);
	} else if (input_positive == (uint16_t)ADC_VBAT_P) {
		/* VBAT pin configuration */
		adc_chn_cfg_t adc_vbat_cfg_m = {
				.pre_scale		=	pre_scale,
				.sample_freq		=	sample_freq,
				.input_p	=	input_positive,
				.input_n	=	ADC_GND_N,
		};
		adc_channel_sample_init(ADC0, ADC_VBAT_MODE, ADC_M_CHANNEL, &adc_vbat_cfg_m);
	} else {
		/* Single-ended GPIO pin configuration */
		adc_chn_cfg_t adc_gpio_cfg_m = {
				.pre_scale		=	pre_scale,
				.sample_freq		=	sample_freq,
				.input_p	=	input_positive,
				.input_n	=	ADC_GND_N,
		};
		adc_channel_sample_init(ADC0, ADC_GPIO_MODE, ADC_M_CHANNEL, &adc_gpio_cfg_m);
	}
	return 0;
}
#endif

#ifdef CONFIG_ADC_ASYNC
/* API implementation: read_async */
static int adc_tlx_read_async(const struct device *dev,
			      const struct adc_sequence *sequence,
			      struct k_poll_signal *async)
{
	int status;
	struct tlx_adc_data *data = dev->data;

	adc_context_lock(&data->ctx, true, async);
	status = adc_tlx_adc_start_read(dev, sequence);
	adc_context_release(&data->ctx, status);

	return status;
}
#endif /* CONFIG_ADC_ASYNC */

static struct tlx_adc_data data_0 = {
	ADC_CONTEXT_INIT_TIMER(data_0, ctx),
	ADC_CONTEXT_INIT_LOCK(data_0, ctx),
	ADC_CONTEXT_INIT_SYNC(data_0, ctx),
};

PINCTRL_DT_INST_DEFINE(0);

static const struct tlx_adc_cfg cfg_0 = {
	.sample_freq = DT_INST_PROP(0, sample_freq),
	.vref_internal_mv = DT_INST_PROP(0, vref_internal_mv),
	.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(0),
};

static const struct adc_driver_api adc_tlx_driver_api = {
	.channel_setup = adc_tlx_channel_setup,
	.read = adc_tlx_read,
#ifdef CONFIG_ADC_ASYNC
	.read_async = adc_tlx_read_async,
#endif
	.ref_internal = cfg_0.vref_internal_mv,
};

DEVICE_DT_INST_DEFINE(0, adc_tlx_init, NULL,
		      &data_0,  &cfg_0,
		      POST_KERNEL,
		      CONFIG_ADC_INIT_PRIORITY,
		      &adc_tlx_driver_api);
