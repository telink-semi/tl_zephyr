/*
 * Copyright (c) 2024 Telink Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "telink_tl5x_regions.ld"

/* ram_code section is defined in ramcode.ld */
SECTION_DATA_PROLOGUE(ram_code_reserved, (NOLOAD),)
{
	. = (0x80840000 + ((0 + _ramcode_size_align_256_) * 1));
	. = ALIGN(256);
} GROUP_DATA_LINK_IN(TELINK_RAM_REGION, RAMABLE_REGION)

SECTION_DATA_PROLOGUE(cache, (NOLOAD),)
{
	. += 0x900;
	. = ALIGN(256);
} GROUP_DATA_LINK_IN(TELINK_RAM_REGION, RAMABLE_REGION)

SECTION_DATA_PROLOGUE(retention_data,,)
{
	. = ALIGN(4);
	*(.retention_data)
	*(".retention_data.*")
	*(.retention_data_ble)
	*(".retention_data_ble.*")

	PROVIDE (_RETENTION_DATA_VMA_END = .);
	PROVIDE (_RETENTION_DATA_VMA_START = ADDR(retention_data));
	PROVIDE (_RETENTION_DATA_LMA_START = LOADADDR(retention_data));
} GROUP_DATA_LINK_IN(TELINK_RAM_REGION, ROMABLE_REGION)

SECTION_DATA_PROLOGUE(noinit_ram, (NOLOAD),)
{
	. = ALIGN(4);
	*(.noinit)
	*(".noinit.*")
} GROUP_DATA_LINK_IN(TELINK_RAM_REGION, RAMABLE_REGION)



