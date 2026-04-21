/*
 * Copyright (c) 2024 Telink Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief Linker script for the Telink TLx SoCs
 */

#include <zephyr/devicetree.h>
#include <zephyr/linker/linker-tool.h>

/**
 * @brief Linker script for the Telink TLx SoCs.
 * This script differentiates memory configurations for various Telink SoCs:
 * 	TL321x SoCs have limited RAM and use a single memory section.
 *	TL721x SoCs do not require this single-section layout and instead.
 * implement separate RAM sections.
 */

MEMORY
{
#if !CONFIG_SOC_SERIES_RISCV_TELINK_TLX_RETENTION
	#if CONFIG_SOC_RISCV_TELINK_TL523X
	RAMILM       (rwx) : ORIGIN = DT_REG_ADDR(DT_NODELABEL(ram_ilm)),              LENGTH = DT_REG_SIZE(DT_NODELABEL(ram_ilm))
	#endif
#else
#endif /* CONFIG_SOC_SERIES_RISCV_TELINK_TLX_RETENTION */
}

#include <zephyr/arch/riscv/common/linker.ld>

#ifdef CONFIG_MCUBOOT
	ASSERT(__rom_region_size <= DT_REG_SIZE(DT_NODELABEL(boot_partition)), "boot_partition overflows")
#endif
