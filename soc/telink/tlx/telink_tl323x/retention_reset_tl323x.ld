/*
 * Copyright (c) 2026 Telink Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "telink_tlx_regions.ld"

/*
 * Keep retention_reset at the beginning of the RAM region (0x68000).
 *
 * The TLx deep-sleep "boot from SRAM" path resumes at the fixed SRAM base
 * 0x68000, so .retention_reset must be the first section in the RAM region.
 * When all RAM sections are merged into one region, this file must be linked
 * before matter_retention_tl323x.ld, otherwise retention_reset ends up after
 * ram_dlm_data / ram_dlm_bss and wake-up executes data instead of code.
 * The relative order of all other RAM sections is left unchanged.
 */

SECTION_DATA_PROLOGUE(retention_reset,,)
{
	. = ALIGN(4);
	KEEP(*(.retention_reset.*))

	PROVIDE (_RETENTION_RESET_VMA_END = .);
	PROVIDE (_RETENTION_RESET_VMA_START = ADDR(retention_reset));
	PROVIDE (_RETENTION_RESET_LMA_START = LOADADDR(retention_reset));
} GROUP_DATA_LINK_IN(TELINK_RAM_REGION, ROMABLE_REGION)
