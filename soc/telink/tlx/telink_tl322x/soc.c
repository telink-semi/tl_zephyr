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

#if (defined(CONFIG_BT_TLX) || defined(CONFIG_IEEE802154_TELINK_TLX))
#include "tlx_bt_flash.h"
#endif

#include <stdlib.h>

#if TLK_ONLY_BLE_HOST
#include "stack/multicore_comm/service/service_d25f.h"
#endif

/* Drivers changes for hal_v2, so should not change castart.s, add external*/
_attribute_data_retention_sec_ unsigned int g_pm_mspi_cfg;
__attribute__((section(".ram_code_retention"))) __attribute__((noinline)) void
pm_retention_register_recover(void)
{
}

/* List of supported CCLK frequencies */
#define CLK_48MHZ  48000000u
#define CLK_64MHZ  64000000u
#define CLK_72MHZ  72000000u
#define CLK_96MHZ  96000000u
#define CLK_192MHZ 192000000u
#define PLL_192M_D25F_192M_HCLK_N22_96M_PCLK_96M_MSPI_48M \
	clock_init(CLK_BASEBAND_PLL_192M, CLK_DIV1, CCLK_DIV2_TO_HCLK_DIV2_TO_PCLK, CLK_DIV4)

#if CONFIG_USB_TELINK_TLX
/* Check Clock value for USB0. */
#if DT_PROP(DT_PATH(cpus, cpu_0), clock_frequency) < CLK_96MHZ
#error "USB0 digital voltage must be 1.1V and HCLK min's 48M"
#endif
#endif

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
#if (CCLK_FREQ != CLK_48MHZ && CCLK_FREQ != CLK_64MHZ && \
	CCLK_FREQ != CLK_72MHZ && CCLK_FREQ != CLK_96MHZ && \
	CCLK_FREQ != CLK_192MHZ)
#error "Invalid clock-frequency. Supported values: 48,64,72,96,192 MHz"
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

#if CONFIG_PM
#define RST_BIT_SET(x, n)   ((x) |= ~(n))
#define RST_BIT_CLR(x, n)   ((x) &= ~(n))
#define CLOCK_BIT_CLR(x, n) ((x) &= ~(n))
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

	gpio_shutdown(GPIO_ALL);

	gpio_set_up_down_res(GPIO_SWS, GPIO_PIN_PULLUP_1M);

#if (defined(CONFIG_BT_TLX) || defined(CONFIG_IEEE802154_TELINK_TLX))
	soc_load_rf_parameters_normal();
#endif

	/* clocks init: CCLK, HCLK, PCLK */
	switch (cclk) {
	case CLK_48MHZ:
		PLL_192M_D25F_48M_HCLK_N22_24M_PCLK_12M_MSPI_48M;
		break;

	case CLK_64MHZ:
		PLL_192M_D25F_64M_HCLK_N22_32M_PCLK_32M_MSPI_48M;
		pm_set_dig_ldo(DIG_VOL_1V_MODE, 1000);
		break;

	case CLK_72MHZ:
		PLL_144M_D25F_72M_HCLK_N22_36M_PCLK_36M_MSPI_48M;
		pm_set_dig_ldo(DIG_VOL_1V_MODE, 1000);
		break;

	case CLK_96MHZ:
		pm_set_dig_ldo(DIG_VOL_1V1_MODE, 1000);
		PLL_192M_D25F_96M_HCLK_N22_48M_PCLK_48M_MSPI_48M;
		break;

	case CLK_192MHZ:
		pm_set_dig_ldo(DIG_VOL_1V1_MODE, 1000);
		PLL_192M_D25F_192M_HCLK_N22_96M_PCLK_96M_MSPI_48M;
		break;
	}

	/* Init Machine Timer source clock: 32 KHz RC */
	clock_32k_init(CLK_32K_RC);
	clock_cal_32k_rc();

	/* Stop 32k watchdog */
	wd_32k_stop();

#undef N22_FW_DOWNLOAD_FLASH_ADDR
#define N22_FW_DOWNLOAD_FLASH_ADDR CONFIG_FLASH_BASE_ADDRESS + 0x80000 + 0x13040
	sys_n22_init(N22_FW_DOWNLOAD_FLASH_ADDR);
#if !defined(TLK_ONLY_BLE_HOST)
	rf_n22_dig_init();
#endif

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

	gpio_shutdown(GPIO_ALL);

	gpio_set_up_down_res(GPIO_SWS, GPIO_PIN_PULLUP_1M);

#if (defined(CONFIG_BT_TLX) || defined(CONFIG_IEEE802154_TELINK_TLX))
	soc_load_rf_parameters_deep_retention();
#endif

	/* clocks init: CCLK, HCLK, PCLK */
	switch (cclk) {
	case CLK_48MHZ:
		PLL_192M_D25F_48M_HCLK_N22_24M_PCLK_12M_MSPI_48M;
		break;

	case CLK_64MHZ:
		PLL_192M_D25F_64M_HCLK_N22_32M_PCLK_32M_MSPI_48M;
		pm_set_dig_ldo(DIG_VOL_1V_MODE, 1000);
		break;

	case CLK_72MHZ:
		PLL_144M_D25F_72M_HCLK_N22_36M_PCLK_36M_MSPI_48M;
		pm_set_dig_ldo(DIG_VOL_1V_MODE, 1000);
		break;

	case CLK_96MHZ:
		pm_set_dig_ldo(DIG_VOL_1V1_MODE, 1000);
		PLL_192M_D25F_96M_HCLK_N22_48M_PCLK_48M_MSPI_48M;
		break;

	case CLK_192MHZ:
		pm_set_dig_ldo(DIG_VOL_1V1_MODE, 1000);
		PLL_192M_D25F_192M_HCLK_N22_96M_PCLK_96M_MSPI_48M;
		break;
	}

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
 * @param[in]   device_num	- the number of slave device.
 * @param[in]   flash_mid	- the mid of flash.
 * @return      1: success, 0: error, 2: mid is not supported.
 */
unsigned char flash_set_4line_read_write(mspi_slave_device_num_e device_num, unsigned int flash_mid)
{
	unsigned char status = flash_4line_en_with_device_num(device_num, flash_mid);

	if (status == 1) {
		flash_read_page = flash_4read;
		flash_set_rd_xip_config_sram(device_num, FLASH_X4READ_CMD);
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

	mid = flash_read_mid_with_device_num(SLAVE0);
	hw_flash_cap = (flash_capacity_e)((mid & FLASH_MID_SIZE_MASK) >> FLASH_MID_SIZE_OFFSET);

#if (defined(CONFIG_TELINK_TLX_2_WIRE_SPI_ENABLE) && CONFIG_TELINK_TLX_2_WIRE_SPI_ENABLE)
#else
	/* Enable Quad SPI (4x) read and write mode */
	if (flash_set_4line_read_write(SLAVE0, mid) != 1) {
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

#ifdef CONFIG_TELINK_TL322X_ENABLE_N22
static int soc_tlx_mcc_init(void)
{
	extern void mb_irq_handler(void);
	IRQ_CONNECT(IRQ_MAILBOX_N22_TO_D25 + CONFIG_2ND_LVL_ISR_TBL_OFFSET, 2, mb_irq_handler, 0,
		    0);
	volatile uint32_t key = arch_irq_lock();

	sys_n22_start();
	mcc_d25f_service_init();
	arch_irq_unlock(key);

	return 0;
}
SYS_INIT(soc_tlx_mcc_init, POST_KERNEL, 1);
#endif
