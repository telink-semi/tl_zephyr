/*
 * Copyright (c) 2024~2026 Telink Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <sys.h>
#include <clock.h>
#include <gpio.h>
#include <ext_driver/ext_pm.h>
#include "flash.h"
#include <watchdog.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/storage/flash_map.h>
#include "plmt.h"

#if DEBUG_GPIO_ENABLE
#include "gpio_default.h"
#include "cmpt.h"
#endif

#if (defined(CONFIG_BT_TLX) || defined(CONFIG_IEEE802154_TELINK_TLX))
#include "tlx_bt_flash.h"
#endif

#include <stdlib.h>

/* List of supported CCLK frequencies */
#define CLK_24MHZ 24000000u
#define CLK_48MHZ 48000000u

/* MID register flash size */
#define FLASH_MID_SIZE_OFFSET 16
#define FLASH_MID_SIZE_MASK   0x00ff0000

/* Power Mode value */
#if DT_ENUM_IDX(DT_NODELABEL(power), power_mode) == 0
#define POWER_MODE LDO_1P25_LDO_1P8
#elif DT_ENUM_IDX(DT_NODELABEL(power), power_mode) == 1
#define POWER_MODE DCDC_1P25_LDO_1P8
#else
#error "Wrong value for power-mode parameter"
#endif

/* Vbat Type value */
#if DT_ENUM_IDX(DT_NODELABEL(power), vbat_type) == 0
#define VBAT_TYPE VBAT_MAX_VALUE_LESS_THAN_3V6
#elif DT_ENUM_IDX(DT_NODELABEL(power), vbat_type) == 1
#define VBAT_TYPE VBAT_MAX_VALUE_GREATER_THAN_3V6
#else
#error "Wrong value for vbat-type parameter"
#endif

/* Check System Clock value. */
#define CCLK_FREQ DT_PROP(DT_PATH(cpus, cpu_0), clock_frequency)
#if (CCLK_FREQ != CLK_24MHZ && CCLK_FREQ != CLK_48MHZ)
#error "Invalid clock-frequency. Supported values: 24, 48 MHz"
#endif
#undef CCLK_FREQ

#if (defined(CONFIG_BT_TLX) || defined(CONFIG_IEEE802154_TELINK_TLX))
/* SOC Parameters structure */
_attribute_data_retention_sec_ struct {
	unsigned char cap_freq_offset_en;
	unsigned char cap_freq_offset_value;
} soc_nvParam;

/**
 * @brief Perform SOC calibration at boot time (normal boot)
 */
void soc_load_rf_parameters_normal(void)
{
	unsigned char cap_freq_ofset;

	flash_read_page(FIXED_PARTITION_OFFSET(vendor_partition) + TLX_CALIBRATION_ADDR_OFFSET, 1,
			&cap_freq_ofset);
	if (cap_freq_ofset != 0xff) {
		soc_nvParam.cap_freq_offset_en = 1;
		soc_nvParam.cap_freq_offset_value = cap_freq_ofset;
		rf_update_internal_cap(soc_nvParam.cap_freq_offset_value);
	}
}

/**
 * @brief Perform SOC calibration at boot time (deep retention)
 */
void soc_load_rf_parameters_deep_retention(void)
{
	if (soc_nvParam.cap_freq_offset_value) {
		rf_update_internal_cap(soc_nvParam.cap_freq_offset_value);
	}
}
#endif

/**
 * @brief Perform basic initialization at boot.
 *
 * @return 0
 */
void soc_early_init_hook(void)
{
	unsigned int cclk = DT_PROP(DT_PATH(cpus, cpu_0), clock_frequency);

#ifdef CONFIG_PM
	/* Select internal 32K for BLE PM, ASAP after boot */
	blc_pm_select_internal_32k_crystal();
#endif

	/* system init */
	sys_init(POWER_MODE, VBAT_TYPE, INTERNAL_CAP_XTAL24M);

#if CONFIG_PM
	gpio_shutdown(GPIO_ALL);
#endif
	pm_update_status_info(1);

	/* SWS: 1M = low power, 10K = more robust; use 10K if SWire is unstable. */
	gpio_set_up_down_res(GPIO_SWS, GPIO_PIN_PULLUP_1M);

#if (defined(CONFIG_BT_TLX) || defined(CONFIG_IEEE802154_TELINK_TLX))
	soc_load_rf_parameters_normal();
#endif

	/* clocks init: CCLK, HCLK, PCLK */
	switch (cclk) {
	case CLK_24MHZ:
		/* fpga has no clk to set for now */
	    XTAL_24M_SYSCLK_24M;
		break;
	case CLK_48MHZ:
		DOUBLER_48M_SYSCLK_48M;
		break;
	default:
		break;
	}

	/*
	 * HAL clock_init() updates sys_clk.cclk only, but HCLK/PCLK/MSPI_CLK
	 * follow the system clock in hardware. Keep the software view in sync,
	 * otherwise drivers (e.g. UART baudrate divider) use a wrong PCLK.
	 */
	sys_clk.cclk = cclk / 1000000U;
	sys_clk.hclk = cclk / 1000000U;
	sys_clk.pclk = cclk / 1000000U;
	sys_clk.mspi_clk = cclk / 1000000U;

	/* Init Machine Timer source clock: 32 KHz RC */
	clock_cal_32k_rc();
	mtime_clk_init(CLK_32K_RC);

	/* Stop 32k watchdog */
	wd_32k_stop();

	/* int deepRetWakeUp = pm_is_MCU_deepRetentionWakeup(); */
	/* #if DEBUG_GPIO_ENABLE */
	/* gpio_init(!deepRetWakeUp); */
	/* #else */
	/* (void)deepRetWakeUp; */
	/* #endif */
	wd_stop();

}

/**
 * @brief Reset the system.
 */
void sys_arch_reboot(int type)
{
	ARG_UNUSED(type);

	/* protected_sys_reboot(); */
}

/**
 * @brief Restore SOC after deep-sleep.
 */
void soc_tlx_restore(void)
{
	unsigned int cclk = DT_PROP(DT_PATH(cpus, cpu_0), clock_frequency);

	/* system init */
	sys_init(POWER_MODE, VBAT_TYPE, INTERNAL_CAP_XTAL24M);

#if CONFIG_PM
	gpio_shutdown(GPIO_ALL);
#endif

	/* SWS: 1M = low power, 10K = more robust; use 10K if SWire is unstable. */
	gpio_set_up_down_res(GPIO_SWS, GPIO_PIN_PULLUP_1M);

#if (defined(CONFIG_BT_TLX) || defined(CONFIG_IEEE802154_TELINK_TLX))
	soc_load_rf_parameters_deep_retention();
#endif

	/* clocks init: CCLK, HCLK, PCLK */
	switch (cclk) {
	default:
		break;
	}
}

/**
 * @brief Check mounted flash size (should be greater than in .dts).
 */
static int soc_tlx_check_flash(void)
{
	return 0;
}

SYS_INIT(soc_tlx_check_flash, POST_KERNEL, 0);
