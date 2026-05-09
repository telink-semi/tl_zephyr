/*
 * Copyright (c) 2025-2026 Telink Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT telink_tl323x_adc

/* Local driver headers */
#define ADC_CONTEXT_USES_KERNEL_TIMER
#include "adc_context.h"

/* Zephyr Device Tree headers */
#include <zephyr/dt-bindings/adc/tl323x-adc.h>

/* Zephyr Logging headers */
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(adc_tl323x, CONFIG_ADC_LOG_LEVEL);

/* Telink HAL headers */
#include "lpc.h"
#include <sd_adc.h>
#include <zephyr/drivers/pinctrl.h>
#ifdef CONFIG_PM_DEVICE
#include <zephyr/pm/device.h>
#endif /* CONFIG_PM_DEVICE */

/* Driver context structure */
struct tl323x_adc_data {
	struct adc_context ctx;
	int16_t sample;
	int16_t *buffer;
	int16_t *repeat_buffer;
	struct k_sem acq_sem;
	struct k_thread thread;
    uint8_t mode;

	/* To save resources, the ADC thread will not be created because the user is not using */
	/* K_KERNEL_STACK_MEMBER(stack, CONFIG_ADC_TL323X_ACQUISITION_THREAD_STACK_SIZE); */
};

/* Driver configuration structure */
struct tl323x_adc_cfg {
	uint8_t resolution;
	uint32_t sample_freq;
	uint32_t downsample_rate;
	uint8_t vbat_divider;
    const struct pinctrl_dev_config *pcfg;
};

#ifdef CONFIG_PM_DEVICE
struct adc_channel_cfg tl323x_channel_cfg;
#endif /* CONFIG_PM_DEVICE */

#define SD_ADC_SAMPLE_CNT 16  // Number of samples used to calculate the average.

/* Convert dts pin to tl323x SDK pin */
static sd_adc_p_input_pin_def_e adc_tl323x_get_p_pin(uint8_t dt_pin)
{
    sd_adc_p_input_pin_def_e adc_pin;

    switch (dt_pin) {
    case DT_ADC_GPIO_PB0:
        adc_pin = SD_ADC_GPIO_PB0P;
        break;
    case DT_ADC_GPIO_PB1:
        adc_pin = SD_ADC_GPIO_PB1P;
        break;
    case DT_ADC_GPIO_PB2:
        adc_pin = SD_ADC_GPIO_PB2P;
        break;
    case DT_ADC_GPIO_PB3:
        adc_pin = SD_ADC_GPIO_PB3P;
        break;
    case DT_ADC_GPIO_PB4:
        adc_pin = SD_ADC_GPIO_PB4P;
        break;
    case DT_ADC_GPIO_PB5:
        adc_pin = SD_ADC_GPIO_PB5P;
        break;
    case DT_ADC_GPIO_PB6:
        adc_pin = SD_ADC_GPIO_PB6P;
        break;
    case DT_ADC_GPIO_PB7:
        adc_pin = SD_ADC_GPIO_PB7P;
        break;
    case DT_ADC_GPIO_PD0:
        adc_pin = SD_ADC_GPIO_PD0P;
        break;
    case DT_ADC_GPIO_PD1:
        adc_pin = SD_ADC_GPIO_PD1P;
        break;
    default:
        adc_pin = SD_ADC_GPIO_PB4P;
        break;
    }

    return adc_pin;
}

/* Convert dts pin to tl323x SDK negative pin */
static sd_adc_n_input_pin_def_e adc_tl323x_get_n_pin(uint8_t dt_pin)
{
    sd_adc_n_input_pin_def_e adc_pin;

    switch (dt_pin) {
    case DT_ADC_GPIO_PB0:
        adc_pin = SD_ADC_GPIO_PB0N;
        break;
    case DT_ADC_GPIO_PB1:
        adc_pin = SD_ADC_GPIO_PB1N;
        break;
    case DT_ADC_GPIO_PB2:
        adc_pin = SD_ADC_GPIO_PB2N;
        break;
    case DT_ADC_GPIO_PB3:
        adc_pin = SD_ADC_GPIO_PB3N;
        break;
    case DT_ADC_GPIO_PB4:
        adc_pin = SD_ADC_GPIO_PB4N;
        break;
    case DT_ADC_GPIO_PB5:
        adc_pin = SD_ADC_GPIO_PB5N;
        break;
    case DT_ADC_GPIO_PB6:
        adc_pin = SD_ADC_GPIO_PB6N;
        break;
    case DT_ADC_GPIO_PB7:
        adc_pin = SD_ADC_GPIO_PB7N;
        break;
    case DT_ADC_GPIO_PD0:
        adc_pin = SD_ADC_GPIO_PD0N;
        break;
    case DT_ADC_GPIO_PD1:
        adc_pin = SD_ADC_GPIO_PD1N;
        break;
    default:
        adc_pin = SD_ADC_GNDN;
        break;
    }

    return adc_pin;
}

/* Helper function to set up sampling frequency */
static int adc_tl323x_setup_sample_freq(uint32_t sample_freq, sd_adc_sample_clk_freq_e *sample_clk)
{
	switch (sample_freq) {
	case 1000000:
		*sample_clk = SD_ADC_SAPMPLE_CLK_1M;
		return 0;
	case 2000000:
		*sample_clk = SD_ADC_SAPMPLE_CLK_2M;
		return 0;
	default:
		LOG_ERR("Selected sample frequency is not supported.");
		return -EINVAL;
	}
}

/* Helper function to set up downsample rate */
static int adc_tl323x_setup_downsample_rate(uint32_t downsample_rate, sd_adc_downsample_rate_e *rate)
{
	switch (downsample_rate) {
	case 64:
		*rate = SD_ADC_DOWNSAMPLE_RATE_64;
		return 0;
	case 128:
		*rate = SD_ADC_DOWNSAMPLE_RATE_128;
		return 0;
	case 256:
		*rate = SD_ADC_DOWNSAMPLE_RATE_256;
		return 0;
	default:
		LOG_ERR("Selected downsample rate is not supported.");
		return -EINVAL;
	}
}

/**
 * @brief Collect samples and calculate average values
 *
 * This function performs the following operations:
 * 1. Power on the SD ADC and begin sampling
 * 2. Wait for sampling to stabilize
 * 3. Discard the first 4 invalid samples
 * 4. Collect a specified number of samples
 * 5. Sort the samples using insertion sort
 * 6. Calculate the average value (excluding the highest and lowest 1/4 data)
 * 7. Stop sampling and power off the ADC
 *
 * @return The calculated average voltage value (unit: mV)
 */
static int sd_adc_collect_and_calculate_average(struct tl323x_adc_data *data)
{
	signed int sd_adc_sample_buffer[SD_ADC_SAMPLE_CNT] __attribute__((aligned(4))) = {0};
	signed int code_average = 0;

	// Enable ADC and start sampling
	sd_adc_power_on(SD_ADC_SAMPLE_MODE);

    /* Wait for ADC to stabilize */
    k_busy_wait(200);

    /* Start sampling */
	sd_adc_sample_start();

    // Discard the first 4 invalid samples
	for (int i = 0; i < 4; i++) {
		sd_adc_get_raw_code();
	}

	// Collect multiple samples
	int cnt = 0;
	while (cnt < SD_ADC_SAMPLE_CNT) {
		int sample_cnt = sd_adc_get_rxfifo_cnt();
		if (sample_cnt > 0) {
			sd_adc_sample_buffer[cnt] = sd_adc_get_raw_code();
			cnt++;
		}
	}

	// Sort samples using insertion sort
	for(int i = 1; i < SD_ADC_SAMPLE_CNT; i++) {
		if(sd_adc_sample_buffer[i] < sd_adc_sample_buffer[i-1]) {
			signed int temp = sd_adc_sample_buffer[i];
			sd_adc_sample_buffer[i] = sd_adc_sample_buffer[i-1];
			int j;
			for(j = i-1; j >= 0 && sd_adc_sample_buffer[j] > temp; j--) {
				sd_adc_sample_buffer[j+1] = sd_adc_sample_buffer[j];
			}
			sd_adc_sample_buffer[j+1] = temp;
		}
	}

	// Calculate average (remove the highest and lowest 1/4 data)
	for (int i = SD_ADC_SAMPLE_CNT >> 2; i < (SD_ADC_SAMPLE_CNT - (SD_ADC_SAMPLE_CNT >> 2)); i++) {
		code_average += sd_adc_sample_buffer[i] / (SD_ADC_SAMPLE_CNT >> 1);
	}

    signed int sd_adc_vol_10x = 0;
    signed int sd_adc_vol = 0;
    sd_adc_vol_10x = sd_adc_calculate_voltage(code_average, SD_ADC_VOLTAGE_10X_MV);
    sd_adc_vol = sd_adc_vol_10x / 10;
    return sd_adc_vol;
}

/* ADC Context API implementation: start sampling */
static void adc_context_start_sampling(struct adc_context *ctx)
{
	struct tl323x_adc_data *data = CONTAINER_OF(ctx, struct tl323x_adc_data, ctx);

	data->repeat_buffer = data->buffer;

	k_sem_give(&data->acq_sem);
}

static void adc_context_update_buffer_pointer(struct adc_context *ctx, bool repeat_sampling)
{
	struct tl323x_adc_data *data = CONTAINER_OF(ctx, struct tl323x_adc_data, ctx);

	if (repeat_sampling) {
		data->buffer = data->repeat_buffer;
	}
}

/* Main ADC Acquisition thread */
static void adc_tl323x_acquisition_thread(const struct device *dev)
{
	struct tl323x_adc_data *data = dev->data;

	while (true) {
		/* Wait for Acquisition semaphore */
		k_sem_take(&data->acq_sem, K_FOREVER);

		/* Collect samples and calculate average voltage value */
		int average_voltage = sd_adc_collect_and_calculate_average(data);

		data->sample = average_voltage;
		*data->buffer++ = data->sample;

        // Stop sampling and power off
        sd_adc_sample_stop();
        sd_adc_power_off(SD_ADC_SAMPLE_MODE);

		/* Release ADC context */
		adc_context_on_sampling_done(&data->ctx, dev);
	}
}

/* Validate ADC data buffer size */
static int adc_tl323x_validate_buffer_size(const struct adc_sequence *sequence)
{
	struct tl323x_adc_data data;
	/* Using dummy data to get the sample size */
	size_t needed = sizeof(data.sample);

	if (sequence->options) {
		needed *= (1 + sequence->options->extra_samplings);
	}

	if (sequence->buffer_size < needed) {
		return -ENOMEM;
	}

	return 0;
}

/* Validate ADC read API input parameters */
static int adc_tl323x_validate_sequence(const struct adc_sequence *sequence)
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

	status = adc_tl323x_validate_buffer_size(sequence);
	if (status) {
		LOG_ERR("Buffer size too small.");
		return status;
	}

	return 0;
}

/* API implementation: channel_setup */
static int adc_tl323x_channel_setup(const struct device *dev,
				    const struct adc_channel_cfg *channel_cfg)
{
	const struct tl323x_adc_cfg *cfg = dev->config;
	sd_adc_sample_clk_freq_e sample_clk;
	sd_adc_vbat_div_e vbat_div;
	sd_adc_downsample_rate_e downsample_rate;
	struct tl323x_adc_data *data = dev->data;
    int ret;

	ARG_UNUSED(dev);

    /* Initialize the SD ADC module to single-channel mode */
	sd_adc_init(SD_ADC_SINGLE_DC_MODE);

	/* Configure sample frequency */
	ret = adc_tl323x_setup_sample_freq(cfg->sample_freq, &sample_clk);
	if (ret != 0) {
		return ret;
	}

	/* Configure downsample rate */
	ret = adc_tl323x_setup_downsample_rate(cfg->downsample_rate, &downsample_rate);
	if (ret != 0) {
		return ret;
	}

   // VBAT mode or GPIO mode
    if (channel_cfg->input_positive == DT_ADC_VBAT) {
        // VBAT mode
        data->mode = SD_ADC_VBAT_MODE;

        // Set VBAT divider
        switch (cfg->vbat_divider) {
        case 4:
            vbat_div = SD_ADC_VBAT_DIV_1F4;
            break;
        default:
            LOG_ERR("Selected VBAT divider is not supported.");
            return -EINVAL;
        }

        /* Configure as VBAT mode */
        sd_adc_vbat_sample_init(sample_clk, vbat_div, downsample_rate);
    } else {
        // GPIO mode
        data->mode = SD_ADC_GPIO_MODE;

        sd_adc_p_input_pin_def_e p_pin = adc_tl323x_get_p_pin(channel_cfg->input_positive);
        sd_adc_n_input_pin_def_e n_pin = SD_ADC_GNDN;

        /* Configure GPIO sampling parameters */
        sd_adc_gpio_cfg_t adc_gpio_cfg = {
            .clk_freq = sample_clk,
            .downsample_rate = downsample_rate,
            .gpio_div = SD_ADC_GPIO_CHN_DIV_1F4,
			.input_p = p_pin,
			.input_n = n_pin
        };

        /* Initialize GPIO sampling */
        sd_adc_gpio_sample_init(&adc_gpio_cfg);
    }
#ifdef CONFIG_PM_DEVICE
	memcpy(&tl323x_channel_cfg, channel_cfg, sizeof(struct adc_channel_cfg));
#endif
    return 0;
}

/* API implementation: read */
static int adc_tl323x_read(const struct device *dev, const struct adc_sequence *sequence)
{
	struct tl323x_adc_data *data = dev->data;
	int status;

	// Verification parameters
	status = adc_tl323x_validate_sequence(sequence);
	if (status != 0) {
		return status;
	}
	// Save buffer pointer
	data->buffer = sequence->buffer;

	// Start ADC conversion
	adc_context_start_read(&data->ctx, sequence);
	return adc_context_wait_for_completion(&data->ctx);
}

static int adc_tl323x_init(const struct device *dev)
{
	struct tl323x_adc_data *data = dev->data;
	const struct tl323x_adc_cfg *config = dev->config;
	int err;

	/* Configure dt provided device signals when available */
    err = pinctrl_apply_state(config->pcfg, PINCTRL_STATE_DEFAULT);
    if (err < 0) {
        LOG_ERR("ADC pinctrl setup failed (%d)", err);
        return err;
    }

	k_sem_init(&data->acq_sem, 0, 1);
	/* To save resources, the ADC thread will not be created because the user is not using */
	/* k_thread_create(&data->thread, data->stack,
		CONFIG_ADC_TL323X_ACQUISITION_THREAD_STACK_SIZE,
		(k_thread_entry_t)adc_tl323x_acquisition_thread,
		(void *)dev, NULL, NULL,
		CONFIG_ADC_TL323X_ACQUISITION_THREAD_PRIO,
		0, K_NO_WAIT);
	*/
	adc_context_unlock_unconditionally(&data->ctx);

	return 0;
}

#ifdef CONFIG_PM_DEVICE
__GENERIC_SECTION(.ram_code)
static int adc_tl323x_pm_action(const struct device *dev, enum pm_device_action action)
{
	switch (action) {
	case PM_DEVICE_ACTION_RESUME:
	{
#if CONFIG_SOC_SERIES_RISCV_TELINK_TLX_RETENTION
		extern volatile bool tlx_deep_sleep_retention;
		if (tlx_deep_sleep_retention) {
			adc_tl323x_channel_setup(dev, &tl323x_channel_cfg);
		}
#endif /* CONFIG_SOC_SERIES_RISCV_TELINK_TLX_RETENTION */
	}
	break;

	case PM_DEVICE_ACTION_SUSPEND:
	{
#if CONFIG_SOC_SERIES_RISCV_TELINK_TLX_RETENTION
		/*
		 * Close LPC before sleep, otherwise
		 * it will increase the standby current.
		 */
		lpc_vbat_detect_disable();
		lpc_power_down();
#endif /* CONFIG_SOC_SERIES_RISCV_TELINK_TLX_RETENTION */
	}
	break;

	default:
		return -ENOTSUP;
	}

	return 0;
}

PM_DEVICE_DT_INST_DEFINE(0, adc_tl323x_pm_action);
#endif /* CONFIG_PM_DEVICE */

static struct tl323x_adc_data tl323x_adc_data_0 = {
	ADC_CONTEXT_INIT_TIMER(tl323x_adc_data_0, ctx),
	ADC_CONTEXT_INIT_LOCK(tl323x_adc_data_0, ctx),
	ADC_CONTEXT_INIT_SYNC(tl323x_adc_data_0, ctx),
};

#ifdef CONFIG_PINCTRL
PINCTRL_DT_INST_DEFINE(0);
#endif

static const struct tl323x_adc_cfg tl323x_adc_cfg_0 = {
	.resolution = DT_INST_PROP(0, resolution),
	.sample_freq = DT_INST_PROP(0, sample_freq),
	.downsample_rate = DT_INST_PROP(0, downsample_rate),
	.vbat_divider = DT_INST_PROP(0, vbat_divider),
#ifdef CONFIG_PINCTRL
    .pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(0),
#else
    .pcfg = NULL,
#endif
};

static const struct adc_driver_api adc_tl323x_api = {
	.channel_setup = adc_tl323x_channel_setup,
	.read = adc_tl323x_read,
};

DEVICE_DT_INST_DEFINE(0,
		      adc_tl323x_init,
		      PM_DEVICE_DT_INST_GET(0),
		      &tl323x_adc_data_0,
		      &tl323x_adc_cfg_0,
		      POST_KERNEL,
		      CONFIG_ADC_INIT_PRIORITY,
		      &adc_tl323x_api);
