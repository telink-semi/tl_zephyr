/*
 * Copyright (c) 2024 Telink Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief Linker script for the Telink TL5X SoCs
 */

#include <zephyr/devicetree.h>
#include <zephyr/linker/linker-tool.h>

MEMORY
{
}

#include <zephyr/arch/riscv/common/linker.ld>

#ifdef CONFIG_MCUBOOT
	ASSERT(__rom_region_size <= DT_REG_SIZE(DT_NODELABEL(boot_partition)), "boot_partition overflows")
#endif
