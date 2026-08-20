/*
 * Copyright (c) 2024~2026 Telink Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <sys.h>
#include <clock.h>
#include <gpio.h>
#include <ext_driver/ext_pm.h>
#include "rf_common.h"
#include "flash.h"
#include <watchdog.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/storage/flash_map.h>

#if DEBUG_GPIO_ENABLE
#include "gpio_default.h"
#endif

#if (defined(CONFIG_BT_TLX) || defined(CONFIG_IEEE802154_TELINK_TL521X))
#include "tlx_bt_flash.h"
#endif

#include <stdlib.h>

/* Drivers changes for hal_v2, so should not change castart.s, add external*/
_attribute_data_retention_sec_ unsigned int g_pm_mspi_cfg;
__attribute__((section(".ram_code_retention"))) __attribute__((noinline)) void
pm_retention_register_recover(void)
{
}

/* List of supported CCLK frequencies */
#define CLK_24MHZ  24000000u
#define CLK_48MHZ  48000000u
#define CLK_72MHZ  72000000u
#define CLK_144MHZ 144000000u

/* MID register flash size */
#define FLASH_MID_SIZE_OFFSET 16
#define FLASH_MID_SIZE_MASK   0x00ff0000

/* Power Mode value */
#if DT_ENUM_IDX(DT_NODELABEL(power), power_mode) == 0
#define POWER_MODE LDO_1P25_LDO_1P8
#elif DT_ENUM_IDX(DT_NODELABEL(power), power_mode) == 1
#define POWER_MODE DCDC_1P25_LDO_1P8
#elif DT_ENUM_IDX(DT_NODELABEL(power), power_mode) == 2
#define POWER_MODE DCDC_1P25_DCDC_1P8
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
#if ((CCLK_FREQ != CLK_24MHZ) && (CCLK_FREQ != CLK_48MHZ) && (CCLK_FREQ != CLK_72MHZ) &&           \
	(CCLK_FREQ != CLK_144MHZ))
#error "Invalid clock-frequency. Supported values: 24, 48, 72, 144 MHz"
#endif
#undef CCLK_FREQ

#if (defined(CONFIG_BT_TLX) || defined(CONFIG_IEEE802154_TELINK_TL521X))
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

#if CONFIG_PM
#define RST_BIT_SET(x, n)   ((x) |= ~(n))
#define RST_BIT_CLR(x, n)   ((x) &= ~(n))
#define CLOCK_BIT_CLR(x, n) ((x) &= ~(n))
#endif /* CONFIG_PM  */

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
#endif /* CONFIG_PM  */

	/* system init */
	sys_init(POWER_MODE, VBAT_TYPE, INTERNAL_CAP_XTAL24M);

/* note: only the 3.3uH, need to set this value , user open by yourself. 6.8uH just ignore. */
#if CONFIG_SOC_PMOS_SWITCH_TIME_CTL
	/* change from 0x04 to 0x06 for the board changes. */
	analog_write_reg8(0x01, (analog_read_reg8(0x01) & 0xf8) | 0x06);
#endif /*CONFIG_SOC_PMOS_SWITCH_TIME_CTL*/

	gpio_shutdown(GPIO_ALL);

#if (defined(CONFIG_BT_TLX) || defined(CONFIG_IEEE802154_TELINK_TL521X))
	soc_load_rf_parameters_normal();
#endif

	/* clocks init: CCLK, HCLK, PCLK */
	switch (cclk) {
	case CLK_24MHZ:
		PLL_144M_CCLK_24M_HCLK_24M_PCLK_24M_MSPI_48M;
		break;

	case CLK_48MHZ:
		PLL_144M_CCLK_48M_HCLK_48M_PCLK_48M_MSPI_48M;
		break;

	case CLK_72MHZ:
		/* pm_set_dig_ldo(DIG_VOL_1V1_MODE, 1000); */
		PLL_144M_CCLK_72M_HCLK_36M_PCLK_36M_MSPI_48M;
		break;
	case CLK_144MHZ:
		/* pm_set_dig_ldo(DIG_VOL_1V1_MODE, 1000); */
		PLL_144M_CCLK_144M_HCLK_72M_PCLK_72M_MSPI_48M;
		break;
	}

	/* Init Machine Timer source clock: 32 KHz RC */
	clock_32k_init(CLK_32K_RC);
	clock_cal_32k_rc();

	extern void pke_dig_en(void);
	pke_dig_en();

	/* 32k watchdog is set by hardware ,init is 5s
	 * to avoid lpd block mspi ,should open 32k wd.
	 */
	wd_32k_stop();
	/*  in zephyr with BT, max sleep time is about 23s*/
	wd_32k_set_interval_ms(30000);
	wd_32k_start();

	/* MCU deep retention wakeUp */
	int deepRetWakeUp = pm_is_MCU_deepRetentionWakeup();

#if DEBUG_GPIO_ENABLE
	gpio_init(!deepRetWakeUp);
#else
	/* remove warning */
	(void)deepRetWakeUp;
#endif
}

/**
 * @brief Reset the system.
 */
void sys_arch_reboot(int type)
{
	ARG_UNUSED(type);

	protected_sys_reboot();
}

/**
 * @brief Restore SOC after deep-sleep.
 */

void soc_tlx_restore(void)
{
	unsigned int cclk = DT_PROP(DT_PATH(cpus, cpu_0), clock_frequency);

	/* system init */
	sys_init(POWER_MODE, VBAT_TYPE, INTERNAL_CAP_XTAL24M);

	/*after exit from suspend or deep-retention ,start 32k wd before lpd*/
	wd_32k_feed();
	wd_32k_start();

/* note: only the 3.3uH, need to set this value , user open by yourself. 6.8uH just ignore .*/
#if CONFIG_SOC_PMOS_SWITCH_TIME_CTL
	/* change from 0x04 to 0x06 for the board changes. */
	analog_write_reg8(0x01, (analog_read_reg8(0x01) & 0xf8) | 0x06);
#endif /*CONFIG_SOC_PMOS_SWITCH_TIME_CTL*/

	gpio_shutdown(GPIO_ALL);

#if (defined(CONFIG_BT_TLX) || defined(CONFIG_IEEE802154_TELINK_TL521X))
	soc_load_rf_parameters_deep_retention();
#endif

	/* clocks init: CCLK, HCLK, PCLK */
	switch (cclk) {
	case CLK_24MHZ:
		PLL_144M_CCLK_24M_HCLK_24M_PCLK_24M_MSPI_48M;
		break;

	case CLK_48MHZ:
		PLL_144M_CCLK_48M_HCLK_48M_PCLK_48M_MSPI_48M;
		break;

	case CLK_72MHZ:
		/* pm_set_dig_ldo(DIG_VOL_1V1_MODE, 1000); */
		PLL_144M_CCLK_72M_HCLK_72M_PCLK_72M_MSPI_48M;
		break;
	case CLK_144MHZ:
		/* pm_set_dig_ldo(DIG_VOL_1V1_MODE, 1000); */
		PLL_144M_CCLK_144M_HCLK_72M_PCLK_72M_MSPI_48M;
		break;
	}

	/* pke enable */
	extern void pke_dig_en(void);
	pke_dig_en();

	/* MCU deep retention wakeUp */
	int deepRetWakeUp = pm_is_MCU_deepRetentionWakeup();

#if DEBUG_GPIO_ENABLE
	gpio_init(!deepRetWakeUp);
#else
	/* remove warning */
	(void)deepRetWakeUp;
#endif
}

#include "flash/flash_common.h"
#include "flash_base.h"
/**
 * @brief       This function is used to set the use of four lines when reading and writing flash.
 * @param[in]   flash_mid   - the mid of flash.
 * @return      1: success, 0: error, 2: mid is not supported.
 */
unsigned char flash_set_4line_read_write(unsigned int flash_mid)
{
	unsigned char status = flash_4line_en(flash_mid);

	if (status == 1) {
		flash_read_page = flash_4read;
		flash_set_rd_xip_config_sram(FLASH_X4READ_CMD);
		flash_write_page = flash_quad_page_program;
	}

	return status;
}

/**
 * @brief Check mounted flash size (should be greater than in .dts).
 */
static int soc_tlx_check_flash(void)
{
	static const size_t dts_flash_size = DT_REG_SIZE(DT_CHOSEN(zephyr_flash));
	size_t hw_flash_size = 0;
	flash_capacity_e hw_flash_cap;
	uint32_t mid;

	mid = flash_read_mid();
	hw_flash_cap = (flash_capacity_e)((mid & FLASH_MID_SIZE_MASK) >> FLASH_MID_SIZE_OFFSET);

#if defined(CONFIG_TELINK_TLX_2_WIRE_SPI_ENABLE) && CONFIG_TELINK_TLX_2_WIRE_SPI_ENABLE
#else
	/* Enable Quad SPI (4x) read and write mode */
	if (flash_set_4line_read_write(mid) != 1) {
		printk("!!! Error: Failed to switch flash model 0x%X to quad mode\n", mid);
	}
#endif

	switch (hw_flash_cap) {
	case FLASH_SIZE_1M:
		hw_flash_size = 1 * 1024 * 1024;
		break;
	case FLASH_SIZE_2M:
		hw_flash_size = 2 * 1024 * 1024;
		break;
	case FLASH_SIZE_4M:
		hw_flash_size = 4 * 1024 * 1024;
		break;
	case FLASH_SIZE_8M:
		hw_flash_size = 8 * 1024 * 1024;
		break;
	case FLASH_SIZE_16M:
		hw_flash_size = 16 * 1024 * 1024;
		break;
	default:
		break;
	}

	if (hw_flash_size < dts_flash_size) {
		printk("!!! flash error: expected (.dts) %u, actually %u\n", dts_flash_size,
		       hw_flash_size);
		abort();
	}

	return 0;
}

SYS_INIT(soc_tlx_check_flash, POST_KERNEL, 0);

#include <zephyr/kernel.h>

struct k_timer wd_32k_timer;
void wd_32k_timer_callback(struct k_timer *timer)
{
	wd_32k_feed();
}

static int soc_tlx_wd_32k_init(void)
{
	wd_32k_feed();
	k_timer_init(&wd_32k_timer, wd_32k_timer_callback, NULL);
	k_timer_start(&wd_32k_timer, K_MSEC(0), K_MSEC(3000));

	return 0;
}
SYS_INIT(soc_tlx_wd_32k_init, POST_KERNEL, 2);
