/*
 * Copyright (c) 2024 Telink Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief Linker script for the Telink TL322X SoCs
 */

#include <zephyr/devicetree.h>
#include <zephyr/linker/linker-tool.h>

/**
 * @brief Linker script for the Telink TL322X SoC.
 * In non-retention mode the full ram_ilm region is used,
 * in retention mode code is placed in the non-retention
 * part of ILM.
 */

MEMORY
{
#if !CONFIG_SOC_SERIES_RISCV_TELINK_TLX_RETENTION
	RAMILM       (rwx) : ORIGIN = DT_REG_ADDR(DT_NODELABEL(ram_ilm)),              LENGTH = DT_REG_SIZE(DT_NODELABEL(ram_ilm))
#else
	RAM_ILM_N    (rwx) : ORIGIN = DT_REG_ADDR(DT_NODELABEL(ram_ilm_nonretention)), LENGTH = DT_REG_SIZE(DT_NODELABEL(ram_ilm_nonretention))
#endif
}

#include <zephyr/arch/riscv/common/linker.ld>

#ifdef CONFIG_MCUBOOT
	ASSERT(__rom_region_size <= DT_REG_SIZE(DT_NODELABEL(boot_partition)), "boot_partition overflows")
#endif
