/*
 * Copyright (c) 2024~2026 Telink Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <sys.h>
#include <clock.h>
#include <gpio.h>
#include <ext_driver/ext_pm.h>
#if !CONFIG_SOC_RISCV_TELINK_TL523X
#include "rf_common.h"
#endif
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

/* List of supported CCLK frequencies */
#if CONFIG_SOC_RISCV_TELINK_TL322X
#if TLK_ONLY_BLE_HOST
#include "stack/multicore_comm/service/service_d25f.h"
#endif
#endif

/* Drivers changes for hal_v2, so should not change castart.s, add external*/
#if CONFIG_SOC_RISCV_TELINK_TL323X
_attribute_data_retention_sec_ unsigned int g_pm_mspi_cfg;
__attribute__((section(".ram_code_retention"))) __attribute__((noinline)) void
pm_retention_register_recover(void)
{
}
#endif

/* Power Mode value */
#if CONFIG_SOC_RISCV_TELINK_TL523X
#define CLK_24MHZ 24000000u
#define CLK_48MHZ 48000000u
#endif

/* MID register flash size */
#define FLASH_MID_SIZE_OFFSET 16
#define FLASH_MID_SIZE_MASK   0x00ff0000

/* Power Mode value */
#if CONFIG_SOC_RISCV_TELINK_TL523X
#if DT_ENUM_IDX(DT_NODELABEL(power), power_mode) == 0
#define POWER_MODE LDO_1P25_LDO_1P8
#elif DT_ENUM_IDX(DT_NODELABEL(power), power_mode) == 1
#define POWER_MODE DCDC_1P25_LDO_1P8
#else
#error "Wrong value for power-mode parameter"
#endif
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
#if CONFIG_SOC_RISCV_TELINK_TL523X
#if ((CCLK_FREQ != CLK_24MHZ) && (CCLK_FREQ != CLK_48MHZ))
#error "Invalid clock-frequency. Supported values: 24, 48 MHz"
#endif
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

#if CONFIG_PM && CONFIG_SOC_RISCV_TELINK_TL323X
#define RST_BIT_CLR(x, n)   ((x) |= (n))
#define CLOCK_BIT_CLR(x, n) ((x) &= ~(n))
__attribute__((noinline)) __attribute__((section(".ram_code")))
__attribute__((optimize("O2"))) void gen_fsk_close_unused_clock(void)
{
	RST_BIT_CLR(reg_rst0, FLD_RST0_I2C0);
	RST_BIT_CLR(reg_rst0, FLD_RST0_UART1);

	RST_BIT_CLR(reg_rst1, FLD_RST1_UART3);
	RST_BIT_CLR(reg_rst1, FLD_RST1_GSPI);
	RST_BIT_CLR(reg_rst1, FLD_RST1_DMA);
	RST_BIT_CLR(reg_rst1, FLD_RST1_SPISLV);

	RST_BIT_CLR(reg_rst2, FLD_RST2_I2C1);
	RST_BIT_CLR(reg_rst2, FLD_RST2_LM);
	RST_BIT_CLR(reg_rst2, FLD_RST2_TRNG);

	RST_BIT_CLR(reg_rst3, FLD_RST3_QDEC1);
	RST_BIT_CLR(reg_rst3, FLD_RST3_TRACE);
	RST_BIT_CLR(reg_rst3, FLD_RST3_BROM);
	RST_BIT_CLR(reg_rst3, FLD_RST3_QDEC);

	RST_BIT_CLR(reg_rst4, FLD_RST4_DC);
	RST_BIT_CLR(reg_rst4, FLD_RST4_UART4);
	RST_BIT_CLR(reg_rst4, FLD_RST4_SKE);
	RST_BIT_CLR(reg_rst4, FLD_RST4_HASH); /* will enable when HW HASH used */

	RST_BIT_CLR(reg_rst5, FLD_RST5_UART2);
	RST_BIT_CLR(reg_rst5, FLD_RST5_PEM);

	RST_BIT_CLR(reg_rst6, FLD_RST6_RZ);

	RST_BIT_CLR(reg_rst7, FLD_RST7_USB1);

	CLOCK_BIT_CLR(reg_clk_en0, FLD_CLK0_LSPI_EN);
	CLOCK_BIT_CLR(reg_clk_en0, FLD_CLK0_I2C0_EN);
	CLOCK_BIT_CLR(reg_clk_en0, FLD_CLK0_UART1_EN);

	CLOCK_BIT_CLR(reg_clk_en1, FLD_CLK0_UART3_EN);
	CLOCK_BIT_CLR(reg_clk_en1, FLD_CLK1_DMA_EN);
	CLOCK_BIT_CLR(reg_clk_en1, FLD_CLK1_GSPI_EN);
	CLOCK_BIT_CLR(reg_clk_en1, FLD_CLK1_SPISLV_EN);

	CLOCK_BIT_CLR(reg_clk_en2, FLD_CLK2_I2C1_EN);

	CLOCK_BIT_CLR(reg_clk_en3, FLD_CLK3_QDEC1_EN);
	CLOCK_BIT_CLR(reg_clk_en3, FLD_CLK3_TRACE_EN);
	CLOCK_BIT_CLR(reg_clk_en3, FLD_CLK3_BROM_EN);
	CLOCK_BIT_CLR(reg_clk_en3, FLD_CLK3_QDEC0_EN);

	CLOCK_BIT_CLR(reg_clk_en4, FLD_CLK4_DC_EN);
	CLOCK_BIT_CLR(reg_clk_en4, FLD_CLK4_UART4_EN);
	CLOCK_BIT_CLR(reg_clk_en4, FLD_CLK4_SKE_EN);
	CLOCK_BIT_CLR(reg_clk_en4, FLD_CLK4_HASH_EN); /* will enable when HW HASH used */

	CLOCK_BIT_CLR(reg_clk_en5, FLD_CLK5_UART2_EN);
	CLOCK_BIT_CLR(reg_clk_en5, FLD_CLK5_PEM_EN);

	CLOCK_BIT_CLR(reg_clk_en6, FLD_CLK6_RZ_EN);

	CLOCK_BIT_CLR(reg_clk_en7, FLD_CLK7_USB1_EN);
}
#endif /* CONFIG_PM  */

#if CONFIG_SOC_RISCV_TELINK_TL323X && CONFIG_PM
#include "pm.h"
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
#endif /* CONFIG_PM  */

	/* in non pm mode ,will set ldo to 1.2v to make it work ok */
#if CONFIG_SOC_RISCV_TELINK_TL323X && !CONFIG_PM
	cclk = CLK_96MHZ;
#endif

	/* system init */
	sys_init(POWER_MODE, VBAT_TYPE, INTERNAL_CAP_XTAL24M);

#if CONFIG_SOC_RISCV_TELINK_TL721X
	if (cclk == CLK_240MHZ) {
		pm_set_dvdd(CORE_0P9V_SRAM_0P9V_BB_0P9V, DMA1, 1000);
	}
	pm_set_ret_ldo_voltage(RET_LDO_TRIM_0P65V);
#endif

/* note: only the 3.3uH, need to set this value , user open by yourself. 6.8uH just ignore .*/
#if CONFIG_SOC_RISCV_TELINK_TL323X && CONFIG_SOC_PMOS_SWITCH_TIME_CTL
	analog_write_reg8(0x01, (analog_read_reg8(0x01) & 0xf8) |
					0x06); /* change from 0x04 to 0x06 for the board changes. */
#endif                                         /*CONFIG_SOC_PMOS_SWITCH_TIME_CTL*/

#if CONFIG_PM
	gpio_shutdown(GPIO_ALL);
#endif /* CONFIG_PM */

#if CONFIG_SOC_RISCV_TELINK_TL323X && CONFIG_ADC_TELINK_TL323X
	g_adc_calib_flag = efuse_calib_sd_adc_vref();
#endif

#if (defined(CONFIG_BT_TLX) || defined(CONFIG_IEEE802154_TELINK_TLX))
	soc_load_rf_parameters_normal();
#endif

	/* clocks init: CCLK, HCLK, PCLK */
	switch (cclk) {
#if CONFIG_SOC_RISCV_TELINK_TL523X
	case CLK_24MHZ:
		/* fpga has no clk to set for now */
		break;
#endif
	default:
		break;
	}

	/* Init Machine Timer source clock: 32 KHz RC */
	/* clock_32k_init(CLK_32K_RC); */
	/* clock_cal_32k_rc(); */

	/* Stop 32k watchdog */
	wd_32k_stop();

	/* int deepRetWakeUp = pm_is_MCU_deepRetentionWakeup(); */
	/* #if DEBUG_GPIO_ENABLE */
	/* gpio_init(!deepRetWakeUp); */
	/* #else */
	/* (void)deepRetWakeUp; */
	/* #endif */
	wd_stop();
	gpio_shutdown(GPIO_ALL);
	gpio_set_up_down_res(GPIO_SWS, GPIO_PIN_PULLUP_1M);

	gpio_function_en(GPIOB_ALL);
	gpio_output_en(GPIOB_ALL);

	gpio_function_en(GPIOC_ALL);
	gpio_output_en(GPIOC_ALL);
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
#endif /* CONFIG_PM */

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
