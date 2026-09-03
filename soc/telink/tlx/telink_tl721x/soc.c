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

/* Drivers changes for hal_v2, so should not change castart.s, add external*/
_attribute_data_retention_sec_ unsigned int g_pm_mspi_cfg;
__attribute__((section(".ram_code_retention"))) __attribute__((noinline)) void
pm_retention_register_recover(void)
{
}

/* List of supported CCLK frequencies */
#define CLK_40MHZ  40000000u
#define CLK_48MHZ  48000000u
#define CLK_60MHZ  60000000u
#define CLK_80MHZ  80000000u
#define CLK_120MHZ 120000000u
#define CLK_240MHZ 240000000u

/* MID register flash size */
#define FLASH_MID_SIZE_OFFSET 16
#define FLASH_MID_SIZE_MASK   0x00ff0000

/* Power Mode value */
#if DT_ENUM_IDX(DT_NODELABEL(power), power_mode) == 0
#define POWER_MODE LDO_0P94_LDO_1P8
#elif DT_ENUM_IDX(DT_NODELABEL(power), power_mode) == 1
#define POWER_MODE DCDC_0P94_LDO_1P8
#elif DT_ENUM_IDX(DT_NODELABEL(power), power_mode) == 2
#define POWER_MODE DCDC_0P94_DCDC_1P8
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
#if (CCLK_FREQ != CLK_40MHZ && CCLK_FREQ != CLK_48MHZ && \
	CCLK_FREQ != CLK_60MHZ && CCLK_FREQ != CLK_80MHZ && \
	CCLK_FREQ != CLK_120MHZ && CCLK_FREQ != CLK_240MHZ)
#error "Invalid clock-frequency. Supported values: 40, 48, 60, 80, 120, 240 MHz"
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
#if CONFIG_PM
__GENERIC_SECTION(.ram_code)
#endif
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
__attribute__((noinline)) __attribute__((section(".ram_code")))
__attribute__((optimize("O2"))) void gen_fsk_close_unused_clock(void)
{
	RST_BIT_SET(reg_rst0, FLD_RST0_I2C);
	RST_BIT_SET(reg_rst0, FLD_RST0_USB);
	RST_BIT_SET(reg_rst0, FLD_RST0_UART1);
	RST_BIT_SET(reg_rst1, FLD_RST1_SPISLV);
	RST_BIT_SET(reg_rst2, FLD_RST2_AUD);
	RST_BIT_SET(reg_rst2, FLD_RST2_I2C1);
	RST_BIT_SET(reg_rst2, FLD_RST2_LM);
	RST_BIT_SET(reg_rst2, FLD_RST2_TRNG);
	RST_BIT_SET(reg_rst2, FLD_RST2_DPR);
	RST_BIT_SET(reg_rst3, FLD_RST3_QDEC);
	RST_BIT_SET(reg_rst5, FLD_RST5_UART2);
	RST_BIT_SET(reg_rst5, FLD_RST5_KEY_SCAN);
	RST_BIT_SET(reg_rst5, FLD_RST5_PEM);
	RST_BIT_SET(reg_rst5, FLD_RST5_CHACHA20);
	RST_BIT_SET(reg_rst6, FLD_RST6_RZ);

	CLOCK_BIT_CLR(reg_clk_en0, FLD_CLK0_I2C_EN);
	CLOCK_BIT_CLR(reg_clk_en0, FLD_CLK0_USB_EN);
	CLOCK_BIT_CLR(reg_clk_en0, FLD_CLK0_UART1_EN);
	CLOCK_BIT_CLR(reg_clk_en1, FLD_CLK1_SPISLV_EN);
	CLOCK_BIT_CLR(reg_clk_en2, FLD_CLK2_AUD_EN);
	CLOCK_BIT_CLR(reg_clk_en2, FLD_CLK2_I2C1_EN);
	CLOCK_BIT_CLR(reg_clk_en2, FLD_CLK2_TRNG_EN);
	CLOCK_BIT_CLR(reg_clk_en3, FLD_CLK3_TRACE_EN);
	CLOCK_BIT_CLR(reg_clk_en3, FLD_CLK3_BROM_EN);
	CLOCK_BIT_CLR(reg_clk_en5, FLD_CLK5_UART2_EN);
	CLOCK_BIT_CLR(reg_clk_en5, FLD_CLK5_KEYSCAN_EN);
	CLOCK_BIT_CLR(reg_clk_en5, FLD_CLK5_PEM_EN);
	CLOCK_BIT_CLR(reg_clk_en5, FLD_CLK5_CHACHA20_EN);
	CLOCK_BIT_CLR(reg_clk_en6, FLD_CLK6_RZ_EN);
}
#endif

#if CONFIG_PM
#include "pm.h"
#if !CONFIG_COMPILE_SDK
#include "pm_internal.h"
#endif
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
	gpio_set_up_down_res(GPIO_SWS, GPIO_PIN_PULLUP_1M);

	if (cclk == CLK_240MHZ) {
		pm_set_dvdd(CORE_0P9V_SRAM_0P9V_BB_0P9V, DMA1, 1000);
	}
#if CONFIG_PM
	pm_set_dig_ldo_voltage(DIG_LDO_TRIM_0P750V);
	cclk = CLK_48MHZ;
	pm_set_ret_ldo_voltage(RET_LDO_TRIM_0P65V);
#endif

	gpio_shutdown(GPIO_ALL);

#if (defined(CONFIG_BT_TLX) || defined(CONFIG_IEEE802154_TELINK_TLX))
	soc_load_rf_parameters_normal();
#endif

	/* clocks init: CCLK, HCLK, PCLK */
	switch (cclk) {
	case CLK_48MHZ:
		PLL_240M_CCLK_48M_HCLK_48M_PCLK_48M_MSPI_48M;
		break;
	case CLK_60MHZ:
		PLL_240M_CCLK_60M_HCLK_60M_PCLK_15M_MSPI_48M;
		break;
	case CLK_80MHZ:
		PLL_240M_CCLK_80M_HCLK_40M_PCLK_40M_MSPI_48M;
		break;

	case CLK_120MHZ:
		PLL_240M_CCLK_120M_HCLK_60M_PCLK_60M_MSPI_48M;
		break;
	}

	/* Init Machine Timer source clock: 32 KHz RC */
	clock_32k_init(CLK_32K_RC);
	clock_cal_32k_rc();

	/* pke is not enabled by default */
	extern void pke_dig_en(void);
	pke_dig_en();

	/* Stop 32k watchdog */
	wd_32k_stop();

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
#if CONFIG_PM
__GENERIC_SECTION(.ram_code)
#endif
void soc_tlx_restore(void)
{
	unsigned int cclk = DT_PROP(DT_PATH(cpus, cpu_0), clock_frequency);

	/* system init */
	sys_init(POWER_MODE, VBAT_TYPE, INTERNAL_CAP_XTAL24M);

	gpio_shutdown(GPIO_ALL);

#if CONFIG_PM
	cclk = CLK_48MHZ;
#endif

#if (defined(CONFIG_BT_TLX) || defined(CONFIG_IEEE802154_TELINK_TLX))
	soc_load_rf_parameters_deep_retention();
#endif

	/* clocks init: CCLK, HCLK, PCLK */
	switch (cclk) {
	case CLK_48MHZ:
#if CONFIG_PM
		gen_fsk_close_unused_clock();
#endif
		PLL_240M_CCLK_48M_HCLK_48M_PCLK_48M_MSPI_48M;
		break;
	case CLK_60MHZ:
		PLL_240M_CCLK_60M_HCLK_60M_PCLK_15M_MSPI_48M;
		break;
	case CLK_80MHZ:
		PLL_240M_CCLK_80M_HCLK_40M_PCLK_40M_MSPI_48M;
		break;

	case CLK_120MHZ:
		PLL_240M_CCLK_120M_HCLK_60M_PCLK_60M_MSPI_48M;
	case CLK_240MHZ:
		PLL_240M_CCLK_240M_HCLK_120M_PCLK_120M_MSPI_48M;
		break;
	}

	/* pke is not enabled by default */
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
