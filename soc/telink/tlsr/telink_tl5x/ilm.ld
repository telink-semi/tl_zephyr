/*
 * Copyright (c) 2024 Telink Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "telink_tl5x_regions.ld"

SECTION_DATA_PROLOGUE(ram_code,,)
{
	. = ALIGN(4);
	*(.ram_code)
	*(".ram_code.*")
	*(.ram_code_ble)
	*(".ram_code_ble.*")

	. = ALIGN(256);	
	PROVIDE (_RAMCODE_VMA_END = .);
	PROVIDE (_RAMCODE_VMA_START = ADDR(ram_code));
	PROVIDE (_RAMCODE_LMA_START = LOADADDR(ram_code));
} GROUP_DATA_LINK_IN(TELINK_RAM_CODE_REGION, ROMABLE_REGION)

SECTION_DATA_PROLOGUE(cache,,)
{
	. = ALIGN(256);	
	. += 0x900;
	. = ALIGN(256);
		
} GROUP_DATA_LINK_IN(TELINK_RAM_REGION, ROMABLE_REGION)

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

SECTION_DATA_PROLOGUE(noinit_ilm, (NOLOAD),)
{
	. = ALIGN(4);
	*(.noinit)
	*(".noinit.*")
} GROUP_DATA_LINK_IN(TELINK_RAM_REGION, TELINK_RAM_REGION)
