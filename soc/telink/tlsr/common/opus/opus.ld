/*
 * Copyright (c) 2025 Telink Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

SECTION_PROLOGUE(telink_opus_fast_code,,)
{
	. = ALIGN(4);
	*(.opus_ram_code)
	*(".opus_ram_code.*")
	PROVIDE (_TELINK_OPUS_FAST_CODE_VMA_END = .);
	PROVIDE (_TELINK_OPUS_FAST_CODE_VMA_START = ADDR(telink_opus_fast_code));
	PROVIDE (_TELINK_OPUS_FAST_CODE_LMA_START = LOADADDR(telink_opus_fast_code));
} GROUP_LINK_IN(ROMABLE_REGION)

SECTION_DATA_PROLOGUE(telink_opus_data,,)
{
	. = ALIGN(4);
	*(.opus_data)
	*(".opus_data.*")
	PROVIDE (_TELINK_OPUS_DATA_VMA_END = .);
	PROVIDE (_TELINK_OPUS_DATA_VMA_START = ADDR(telink_opus_data));
	PROVIDE (_TELINK_OPUS_DATA_LMA_START = LOADADDR(telink_opus_data));
} GROUP_DATA_LINK_IN(RAMABLE_REGION, ROMABLE_REGION)
