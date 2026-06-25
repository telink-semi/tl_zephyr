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

#if CONFIG_SOC_RISCV_TELINK_TL323X && !CONFIG_PM && !CONFIG_MCUBOOT
#include "tlx_bt.h"
#endif /* CONFIG_SOC_RISCV_TELINK_TL323X && !CONFIG_PM && !CONFIG_MCUBOOT */

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
#if CONFIG_SOC_RISCV_TELINK_TL322X || CONFIG_SOC_RISCV_TELINK_TL323X || \
	CONFIG_SOC_RISCV_TELINK_TL721X || CONFIG_SOC_RISCV_TELINK_TL521X
_attribute_data_retention_sec_ unsigned int g_pm_mspi_cfg;
__attribute__((section(".ram_code_retention"))) __attribute__((noinline)) void
pm_retention_register_recover(void)
{
}
#endif

/* Power Mode value */
#if CONFIG_SOC_RISCV_TELINK_TL321X
/* List of supported CCLK frequencies */
#define CLK_24MHZ 24000000u
#define CLK_48MHZ 48000000u
#define CLK_96MHZ 96000000u
#elif CONFIG_SOC_RISCV_TELINK_TL322X
/* List of supported CCLK frequencies */
#define CLK_48MHZ  48000000u
#define CLK_64MHZ  64000000u
#define CLK_72MHZ  72000000u
#define CLK_96MHZ  96000000u
#define CLK_192MHZ 192000000u
#elif CONFIG_SOC_RISCV_TELINK_TL323X
#define CLK_24MHZ 24000000u
#define CLK_48MHZ 48000000u
#define CLK_96MHZ 96000000u
#elif CONFIG_SOC_RISCV_TELINK_TL521X
#define CLK_24MHZ 24000000u
#define CLK_48MHZ 48000000u
#define CLK_72MHZ 72000000u
#define CLK_144MHZ 144000000u
#elif CONFIG_SOC_RISCV_TELINK_TL721X
#define CLK_40MHZ  40000000u
#define CLK_48MHZ  48000000u
#define CLK_60MHZ  60000000u
#define CLK_80MHZ  80000000u
#define CLK_120MHZ 120000000u
#define CLK_240MHZ 240000000u
#endif

#if CONFIG_SOC_RISCV_TELINK_TL322X && CONFIG_USB_TELINK_TLX
/* Check Clock value for USB0. */
#if DT_PROP(DT_PATH(cpus, cpu_0), clock_frequency) < CLK_96MHZ
#error "USB0 digital voltage must be 1.1V and HCLK min's 48M"
#endif
#endif

/* MID register flash size */
#define FLASH_MID_SIZE_OFFSET 16
#define FLASH_MID_SIZE_MASK   0x00ff0000

/* Power Mode value */
#if CONFIG_SOC_RISCV_TELINK_TL321X
#if DT_ENUM_IDX(DT_NODELABEL(power), power_mode) == 0
#define POWER_MODE LDO_1P25_LDO_1P8
#elif DT_ENUM_IDX(DT_NODELABEL(power), power_mode) == 1
#define POWER_MODE DCDC_1P25_LDO_1P8
#else
#error "Wrong value for power-mode parameter"
#endif
#elif CONFIG_SOC_RISCV_TELINK_TL322X
#if DT_ENUM_IDX(DT_NODELABEL(power), power_mode) == 0
#define POWER_MODE LDO_1P25_LDO_1P8
#elif DT_ENUM_IDX(DT_NODELABEL(power), power_mode) == 1
#define POWER_MODE DCDC_1P25_LDO_1P8
#elif DT_ENUM_IDX(DT_NODELABEL(power), power_mode) == 2
#define POWER_MODE DCDC_1P25_DCDC_1P8
#else
#error "Wrong value for power-mode parameter"
#endif
#elif CONFIG_SOC_RISCV_TELINK_TL323X
#if DT_ENUM_IDX(DT_NODELABEL(power), power_mode) == 0
#define POWER_MODE LDO_1P25_LDO_1P8
#elif DT_ENUM_IDX(DT_NODELABEL(power), power_mode) == 1
#define POWER_MODE DCDC_1P25_LDO_1P8
#elif DT_ENUM_IDX(DT_NODELABEL(power), power_mode) == 2
#define POWER_MODE DCDC_1P25_DCDC_1P8
#else
#error "Wrong value for power-mode parameter"
#endif
#elif CONFIG_SOC_RISCV_TELINK_TL521X
#if DT_ENUM_IDX(DT_NODELABEL(power), power_mode) == 0
#define POWER_MODE LDO_1P25_LDO_1P8
#elif DT_ENUM_IDX(DT_NODELABEL(power), power_mode) == 1
#define POWER_MODE DCDC_1P25_LDO_1P8
#elif DT_ENUM_IDX(DT_NODELABEL(power), power_mode) == 2
#define POWER_MODE DCDC_1P25_DCDC_1P8
#else
#error "Wrong value for power-mode parameter"
#endif
#elif CONFIG_SOC_RISCV_TELINK_TL721X
#if DT_ENUM_IDX(DT_NODELABEL(power), power_mode) == 0
#define POWER_MODE LDO_0P94_LDO_1P8
#elif DT_ENUM_IDX(DT_NODELABEL(power), power_mode) == 1
#define POWER_MODE DCDC_0P94_LDO_1P8
#elif DT_ENUM_IDX(DT_NODELABEL(power), power_mode) == 2
#define POWER_MODE DCDC_0P94_DCDC_1P8
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
#if CONFIG_SOC_RISCV_TELINK_TL321X
#if ((CCLK_FREQ != CLK_24MHZ) && (CCLK_FREQ != CLK_48MHZ) && (CCLK_FREQ != CLK_96MHZ))
#error "Invalid clock-frequency. Supported values: 24, 48, 96 MHz"
#endif
#elif CONFIG_SOC_RISCV_TELINK_TL322X
#if ((CCLK_FREQ != CLK_48MHZ) && (CCLK_FREQ != CLK_64MHZ) && (CCLK_FREQ != CLK_72MHZ) &&           \
	 (CCLK_FREQ != CLK_96MHZ))
#error "Invalid clock-frequency. Supported values: 48,64,72,96 MHz"
#endif
#elif CONFIG_SOC_RISCV_TELINK_TL323X
#if ((CCLK_FREQ != CLK_24MHZ) && (CCLK_FREQ != CLK_48MHZ) && (CCLK_FREQ != CLK_96MHZ))
#error "Invalid clock-frequency. Supported values: 24, 48, 96 MHz"
#endif
#elif CONFIG_SOC_RISCV_TELINK_TL521X
#if ((CCLK_FREQ != CLK_24MHZ) && (CCLK_FREQ != CLK_48MHZ) && (CCLK_FREQ != CLK_72MHZ) &&           \
	 (CCLK_FREQ != CLK_144MHZ))
#error "Invalid clock-frequency. Supported values: 24, 48, 72, 144 MHz"
#endif
#elif CONFIG_SOC_RISCV_TELINK_TL721X
#if ((CCLK_FREQ != CLK_40MHZ) && (CCLK_FREQ != CLK_48MHZ) && (CCLK_FREQ != CLK_60MHZ) &&           \
	 (CCLK_FREQ != CLK_80MHZ) && (CCLK_FREQ != CLK_120MHZ) && (CCLK_FREQ != CLK_240MHZ))
#error "Invalid clock-frequency. Supported values: 40, 48, 60, 80, 120, 240 MHz"
#endif
#endif
#undef CCLK_FREQ

#if CONFIG_SOC_RISCV_TELINK_TL323X && CONFIG_ADC_TELINK_TL323X
extern drv_api_status_e efuse_calib_sd_adc_vref(void);
_attribute_data_retention_sec_ unsigned int g_adc_calib_flag;
#endif

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
#if CONFIG_PM && CONFIG_SOC_RISCV_TELINK_TL721X
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
#define RST_BIT_SET(x, n)    ((x) |=~(n))
#define RST_BIT_CLR(x, n)    ((x) &=~(n))
#define CLOCK_BIT_CLR(x, n)    ((x) &=~(n))
#if CONFIG_SOC_RISCV_TELINK_TL323X
__attribute__((noinline)) __attribute__((section(".ram_code"))) __attribute__((optimize("O2")))
void gen_fsk_close_unused_clock(void)
{
	RST_BIT_CLR(reg_rst0, FLD_RST0_I2C0);
	/* comment because some user cases need UART1*/
	/* RST_BIT_CLR(reg_rst0, FLD_RST0_UART1); */

	RST_BIT_CLR(reg_rst1, FLD_RST1_UART3);
	RST_BIT_CLR(reg_rst1, FLD_RST1_GSPI);
	RST_BIT_CLR(reg_rst1, FLD_RST1_DMA);
	RST_BIT_CLR(reg_rst1, FLD_RST1_SPISLV);

	RST_BIT_CLR(reg_rst2, FLD_RST2_I2C1);
	RST_BIT_CLR(reg_rst2, FLD_RST2_TRNG);

	RST_BIT_CLR(reg_rst3, FLD_RST3_QDEC1);
	RST_BIT_CLR(reg_rst3, FLD_RST3_BROM);
	RST_BIT_CLR(reg_rst3, FLD_RST3_QDEC);

	RST_BIT_CLR(reg_rst4, FLD_RST4_UART4);

	RST_BIT_CLR(reg_rst5, FLD_RST5_UART2);
	RST_BIT_CLR(reg_rst5, FLD_RST5_PEM);

	RST_BIT_CLR(reg_rst6, FLD_RST6_RZ);

	RST_BIT_CLR(reg_rst7, FLD_RST7_USB1);

	CLOCK_BIT_CLR(reg_clk_en0, FLD_CLK0_LSPI_EN);
	CLOCK_BIT_CLR(reg_clk_en0, FLD_CLK0_I2C0_EN);
	/* comment because some user cases need UART1*/
	/* CLOCK_BIT_CLR(reg_clk_en0, FLD_CLK0_UART1_EN); */

	CLOCK_BIT_CLR(reg_clk_en1, FLD_CLK0_UART3_EN);
	CLOCK_BIT_CLR(reg_clk_en1, FLD_CLK1_DMA_EN);
	CLOCK_BIT_CLR(reg_clk_en1, FLD_CLK1_GSPI_EN);
	CLOCK_BIT_CLR(reg_clk_en1, FLD_CLK1_SPISLV_EN);

	CLOCK_BIT_CLR(reg_clk_en2, FLD_CLK2_I2C1_EN);

	CLOCK_BIT_CLR(reg_clk_en3, FLD_CLK3_QDEC1_EN);
	CLOCK_BIT_CLR(reg_clk_en3, FLD_CLK3_BROM_EN);
	CLOCK_BIT_CLR(reg_clk_en3, FLD_CLK3_QDEC0_EN);

	CLOCK_BIT_CLR(reg_clk_en4, FLD_CLK4_UART4_EN);

	CLOCK_BIT_CLR(reg_clk_en5, FLD_CLK5_UART2_EN);
	CLOCK_BIT_CLR(reg_clk_en5, FLD_CLK5_PEM_EN);

	CLOCK_BIT_CLR(reg_clk_en6, FLD_CLK6_RZ_EN);

	CLOCK_BIT_CLR(reg_clk_en7, FLD_CLK7_USB1_EN);
}
#elif CONFIG_SOC_RISCV_TELINK_TL721X
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
#endif /* CONFIG_SOC_RISCV_TELINK_TL323X */
#endif /* CONFIG_PM  */

#if (CONFIG_SOC_RISCV_TELINK_TL323X || CONFIG_SOC_RISCV_TELINK_TL721X) && CONFIG_PM
#include "pm.h"
/* #include "pm_internal.h"
 */
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

	/* Only need for lighting to switch from zigbee to matter */
#if CONFIG_SOC_RISCV_TELINK_TL323X && !CONFIG_PM && !CONFIG_MCUBOOT
	/* Enable clock before operate rf register */
	rf_mode_init();
	/* Reset Radio */
	rf_radio_reset();
	rf_reset_dma();
	rf_baseband_reset();

	rf_clr_irq_status(FLD_RF_IRQ_ALL);
#endif /* CONFIG_SOC_RISCV_TELINK_TL323X && !CONFIG_PM && !CONFIG_MCUBOOT */

#if CONFIG_SOC_RISCV_TELINK_TL721X
	if (cclk == CLK_240MHZ) {
		pm_set_dvdd(CORE_0P9V_SRAM_0P9V_BB_0P9V, DMA1, 1000);
	}
#if CONFIG_PM
	pm_set_dig_ldo_voltage(DIG_LDO_TRIM_0P750V);
	cclk = CLK_48MHZ;
	pm_set_ret_ldo_voltage(RET_LDO_TRIM_0P65V);
#endif
#endif

/* note: only the 3.3uH, need to set this value , user open by yourself. 6.8uH just ignore. */
#if (CONFIG_SOC_RISCV_TELINK_TL323X) && CONFIG_SOC_PMOS_SWITCH_TIME_CTL
	/* change from 0x04 to 0x06 for the board changes. */
	analog_write_reg8(0x01, (analog_read_reg8(0x01) & 0xf8) | 0x06);
#endif /*CONFIG_SOC_PMOS_SWITCH_TIME_CTL*/

	gpio_shutdown(GPIO_ALL);

#if CONFIG_SOC_RISCV_TELINK_TL323X && CONFIG_ADC_TELINK_TL323X
	g_adc_calib_flag = efuse_calib_sd_adc_vref();
#endif

#if (defined(CONFIG_BT_TLX) || defined(CONFIG_IEEE802154_TELINK_TLX))
	soc_load_rf_parameters_normal();
#endif

	/* clocks init: CCLK, HCLK, PCLK */
	switch (cclk) {
#if CONFIG_SOC_RISCV_TELINK_TL321X
	case CLK_24MHZ:
		PLL_192M_CCLK_24M_HCLK_24M_PCLK_24M_MSPI_48M;
		break;
#elif CONFIG_SOC_RISCV_TELINK_TL323X
	case CLK_24MHZ:
		PLL_192M_CCLK_24M_HCLK_24M_PCLK_24M_MSPI_48M;
		break;
#elif CONFIG_SOC_RISCV_TELINK_TL521X
	case CLK_24MHZ:
		PLL_144M_CCLK_24M_HCLK_24M_PCLK_24M_MSPI_48M;
		break;
#endif

	case CLK_48MHZ:
#if CONFIG_SOC_RISCV_TELINK_TL321X
		PLL_192M_CCLK_48M_HCLK_48M_PCLK_48M_MSPI_48M;
#elif CONFIG_SOC_RISCV_TELINK_TL322X
		PLL_192M_D25F_48M_HCLK_N22_24M_PCLK_12M_MSPI_48M;
#elif CONFIG_SOC_RISCV_TELINK_TL323X
		PLL_192M_CCLK_48M_HCLK_24M_PCLK_12M_MSPI_48M;
#if CONFIG_PM
		pm_set_calib_0p925V_dig_ldo_voltage();
#endif /* CONFIG_PM  */
#elif CONFIG_SOC_RISCV_TELINK_TL521X
		PLL_144M_CCLK_48M_HCLK_48M_PCLK_48M_MSPI_48M;
#elif CONFIG_SOC_RISCV_TELINK_TL721X
		PLL_240M_CCLK_48M_HCLK_48M_PCLK_48M_MSPI_48M;
#endif /* CONFIG_SOC_RISCV_TELINK_TLX */
		break;
#if CONFIG_SOC_RISCV_TELINK_TL721X
	case CLK_60MHZ:
		PLL_240M_CCLK_60M_HCLK_60M_PCLK_15M_MSPI_48M;
		break;
	case CLK_80MHZ:
		PLL_240M_CCLK_80M_HCLK_40M_PCLK_40M_MSPI_48M;
		break;
#endif /* CONFIG_SOC_RISCV_TELINK_TL721X */

#if CONFIG_SOC_RISCV_TELINK_TL322X
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
#endif
#if CONFIG_SOC_RISCV_TELINK_TL521X
	case CLK_72MHZ:
		/* pm_set_dig_ldo(DIG_VOL_1V1_MODE, 1000); */
		PLL_144M_CCLK_72M_HCLK_36M_PCLK_36M_MSPI_48M;
		break;
	case CLK_144MHZ:
		/* pm_set_dig_ldo(DIG_VOL_1V1_MODE, 1000); */
		PLL_144M_CCLK_144M_HCLK_72M_PCLK_72M_MSPI_48M;
		break;
#endif

#if CONFIG_SOC_RISCV_TELINK_TL321X
		/* case CLK_96MHZ:
		 *  PLL_192M_CCLK_96M_HCLK_48M_PCLK_48M_MSPI_64M;
		 *  break;
		 */
#elif CONFIG_SOC_RISCV_TELINK_TL323X
	case CLK_96MHZ:
#if CONFIG_PM
		pm_set_dig_ldo_voltage(DIG_LDO_TRIM_0P1025V);
#endif /* CONFIG_PM  */
		PLL_192M_CCLK_96M_HCLK_48M_PCLK_48M_MSPI_48M;
		break;
#endif

#if CONFIG_SOC_RISCV_TELINK_TL322X
		/* case CLK_192MHZ:
		 *  pm_set_dig_ldo(DIG_VOL_1V1_MODE, 1000);
		 *  PLL_192M_D25F_192M_HCLK_N22_96M_PCLK_96M_MSPI_48M;
		 *  break;
		 */
#endif

#if CONFIG_SOC_RISCV_TELINK_TL721X
	case CLK_120MHZ:
		PLL_240M_CCLK_120M_HCLK_60M_PCLK_60M_MSPI_48M;
		break;
#endif
	}

	/* Init Machine Timer source clock: 32 KHz RC */
	clock_32k_init(CLK_32K_RC);
	clock_cal_32k_rc();
	/* pke is not enabled by default on TL323X */

#if CONFIG_SOC_RISCV_TELINK_TL721X
	extern void pke_dig_en(void);
	pke_dig_en();
#endif

#if CONFIG_SOC_RISCV_TELINK_TL323X || CONFIG_SOC_RISCV_TELINK_TL521X
	extern void pke_dig_en(void);
	pke_dig_en();

	/* 32k watchdog is set by hardware ,init is 5s
	 * to avoid lpd block mspi ,should open 32k wd.
	 */
	wd_32k_stop();
	/*  in zephyr with BT, max sleep time is about 23s*/
	wd_32k_set_interval_ms(30000);
	wd_32k_start();
#else
	/* Stop 32k watchdog */
	wd_32k_stop();

#endif

#if CONFIG_SOC_RISCV_TELINK_TL322X
#undef N22_FW_DOWNLOAD_FLASH_ADDR
#define N22_FW_DOWNLOAD_FLASH_ADDR CONFIG_FLASH_BASE_ADDRESS + 0x80000 + 0x13040
	sys_n22_init(N22_FW_DOWNLOAD_FLASH_ADDR);
#if !defined(TLK_ONLY_BLE_HOST)
	rf_n22_dig_init();
#endif
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

#if CONFIG_SOC_RISCV_TELINK_TL721X && CONFIG_PM
__GENERIC_SECTION(.ram_code)
#endif
void soc_tlx_restore(void)
{
	unsigned int cclk = DT_PROP(DT_PATH(cpus, cpu_0), clock_frequency);

	/* system init */
	sys_init(POWER_MODE, VBAT_TYPE, INTERNAL_CAP_XTAL24M);

#if CONFIG_SOC_RISCV_TELINK_TL323X || CONFIG_SOC_RISCV_TELINK_TL521X
	/*after exit from suspend or deep-retention ,start 32k wd before lpd*/
	wd_32k_feed();
	wd_32k_start();
#endif

/* note: only the 3.3uH, need to set this value , user open by yourself. 6.8uH just ignore .*/
#if (CONFIG_SOC_RISCV_TELINK_TL323X || CONFIG_SOC_RISCV_TELINK_TL521X) &&                          \
	 CONFIG_SOC_PMOS_SWITCH_TIME_CTL
	/* change from 0x04 to 0x06 for the board changes. */
	analog_write_reg8(0x01, (analog_read_reg8(0x01) & 0xf8) | 0x06);
#endif /*CONFIG_SOC_PMOS_SWITCH_TIME_CTL*/

	gpio_shutdown(GPIO_ALL);

#if CONFIG_SOC_RISCV_TELINK_TL323X && CONFIG_ADC_TELINK_TL323X
	if (g_adc_calib_flag == DRV_API_SUCCESS) {
		g_adc_calib_flag = efuse_calib_sd_adc_vref();
	}
#endif

#if CONFIG_SOC_RISCV_TELINK_TL721X && CONFIG_PM
	cclk = CLK_48MHZ;
#endif

#if (defined(CONFIG_BT_TLX) || defined(CONFIG_IEEE802154_TELINK_TLX))
	soc_load_rf_parameters_deep_retention();
#endif

	/* clocks init: CCLK, HCLK, PCLK */
	switch (cclk) {
#if CONFIG_SOC_RISCV_TELINK_TL321X
	case CLK_24MHZ:
		PLL_192M_CCLK_24M_HCLK_24M_PCLK_24M_MSPI_48M;
		break;
#elif CONFIG_SOC_RISCV_TELINK_TL323X
	case CLK_24MHZ:
		PLL_192M_CCLK_24M_HCLK_24M_PCLK_24M_MSPI_48M;
		break;
#elif CONFIG_SOC_RISCV_TELINK_TL521X
	case CLK_24MHZ:
		PLL_144M_CCLK_24M_HCLK_24M_PCLK_24M_MSPI_48M;
		break;
#endif

	case CLK_48MHZ:
#if CONFIG_SOC_RISCV_TELINK_TL321X
		PLL_192M_CCLK_48M_HCLK_48M_PCLK_48M_MSPI_48M;
#elif CONFIG_SOC_RISCV_TELINK_TL322X
		PLL_192M_D25F_48M_HCLK_N22_24M_PCLK_12M_MSPI_48M;
#elif CONFIG_SOC_RISCV_TELINK_TL323X
		PLL_192M_CCLK_48M_HCLK_24M_PCLK_12M_MSPI_48M;
#if CONFIG_PM
		pm_set_calib_0p925V_dig_ldo_voltage();
		/* gen_fsk_close_unused_clock(); */
#endif /* CONFIG_PM  */
#elif CONFIG_SOC_RISCV_TELINK_TL521X
		PLL_144M_CCLK_48M_HCLK_48M_PCLK_48M_MSPI_48M;
#elif CONFIG_SOC_RISCV_TELINK_TL721X
#if CONFIG_PM
		gen_fsk_close_unused_clock();
#endif /* CONFIG_PM  */
		PLL_240M_CCLK_48M_HCLK_48M_PCLK_48M_MSPI_48M;
#endif /* CONFIG_SOC_RISCV_TELINK_TLX */
		break;
#if CONFIG_SOC_RISCV_TELINK_TL721X
	case CLK_60MHZ:
		PLL_240M_CCLK_60M_HCLK_60M_PCLK_15M_MSPI_48M;
		break;
	case CLK_80MHZ:
		PLL_240M_CCLK_80M_HCLK_40M_PCLK_40M_MSPI_48M;
		break;
#endif /* CONFIG_SOC_RISCV_TELINK_TL721X */

#if CONFIG_SOC_RISCV_TELINK_TL322X
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
#endif
#if CONFIG_SOC_RISCV_TELINK_TL521X
	case CLK_72MHZ:
		/* pm_set_dig_ldo(DIG_VOL_1V1_MODE, 1000); */
		PLL_144M_CCLK_72M_HCLK_72M_PCLK_72M_MSPI_48M;
		break;
	case CLK_144MHZ:
		/* pm_set_dig_ldo(DIG_VOL_1V1_MODE, 1000); */
		PLL_144M_CCLK_144M_HCLK_72M_PCLK_72M_MSPI_48M;
		break;
#endif

#if CONFIG_SOC_RISCV_TELINK_TL321X
		/* case CLK_96MHZ:
		 *  PLL_192M_CCLK_96M_HCLK_48M_PCLK_48M_MSPI_64M;
		 *  break;
		 */
#elif CONFIG_SOC_RISCV_TELINK_TL323X
	case CLK_96MHZ:
#if CONFIG_PM
		pm_set_dig_ldo_voltage(DIG_LDO_TRIM_0P1025V);
#endif /* CONFIG_PM  */
		PLL_192M_CCLK_96M_HCLK_48M_PCLK_48M_MSPI_48M;
		break;
#endif

#if CONFIG_SOC_RISCV_TELINK_TL322X
		/* case CLK_192MHZ:
		 *  pm_set_dig_ldo(DIG_VOL_1V1_MODE, 1000);
		 *  PLL_192M_D25F_192M_HCLK_N22_96M_PCLK_96M_MSPI_48M;
		 *  break;
		 */
#endif

#if CONFIG_SOC_RISCV_TELINK_TL721X
	case CLK_120MHZ:
		PLL_240M_CCLK_120M_HCLK_60M_PCLK_60M_MSPI_48M;
	case CLK_240MHZ:
		PLL_240M_CCLK_240M_HCLK_120M_PCLK_120M_MSPI_48M;
		break;
#endif
	}

	/* pke is not enabled by default on TL323X, TL721X and TL521X  */
#if CONFIG_SOC_RISCV_TELINK_TL323X || CONFIG_SOC_RISCV_TELINK_TL721X || \
	CONFIG_SOC_RISCV_TELINK_TL521X
	extern void pke_dig_en(void);
	pke_dig_en();
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

#if CONFIG_SOC_RISCV_TELINK_TL721X
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
#elif CONFIG_SOC_RISCV_TELINK_TL322X
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
#elif CONFIG_SOC_RISCV_TELINK_TL321X || CONFIG_SOC_RISCV_TELINK_TL323X ||                          \
	CONFIG_SOC_RISCV_TELINK_TL521X
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
#endif

/**
 * @brief Check mounted flash size (should be greater than in .dts).
 */
static int soc_tlx_check_flash(void)
{
	static const size_t dts_flash_size = DT_REG_SIZE(DT_CHOSEN(zephyr_flash));
	size_t hw_flash_size = 0;
	flash_capacity_e hw_flash_cap;
	uint32_t mid;

#if CONFIG_SOC_RISCV_TELINK_TL321X || CONFIG_SOC_RISCV_TELINK_TL323X ||                            \
	CONFIG_SOC_RISCV_TELINK_TL521X
	mid = flash_read_mid();
#elif CONFIG_SOC_RISCV_TELINK_TL322X
	mid = flash_read_mid_with_device_num(SLAVE0);
#elif CONFIG_SOC_RISCV_TELINK_TL721X
	mid = flash_read_mid_with_device_num(SLAVE0);
#endif /* CONFIG_SOC_RISCV_TELINK_TLX */
	hw_flash_cap = (flash_capacity_e)((mid & FLASH_MID_SIZE_MASK) >> FLASH_MID_SIZE_OFFSET);

	/* Enable Quad SPI (4x) read and write mode */
#if CONFIG_SOC_RISCV_TELINK_TL321X || CONFIG_SOC_RISCV_TELINK_TL323X ||                            \
	CONFIG_SOC_RISCV_TELINK_TL521X
	if (flash_set_4line_read_write(mid) != 1) {
#elif CONFIG_SOC_RISCV_TELINK_TL322X
	if (flash_set_4line_read_write(SLAVE0, mid) != 1) {
#elif CONFIG_SOC_RISCV_TELINK_TL721X
	if (flash_set_4line_read_write(SLAVE0, mid) != 1) {
#endif /* CONFIG_SOC_RISCV_TELINK_TLX */
		printk("!!! Error: Failed to switch flash model 0x%X to quad mode\n", mid);
	}

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

#if CONFIG_SOC_RISCV_TELINK_TL323X || CONFIG_SOC_RISCV_TELINK_TL521X
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
#endif
