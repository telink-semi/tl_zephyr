/*
 * Copyright (c) 2026 Telink Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#if !CONFIG_SOC_SERIES_RISCV_TELINK_TLX_RETENTION
	#define TELINK_RAM_REGION              RAMABLE_REGION
	#define TELINK_RAM_CODE_REGION         RAM_ILM_N
	#define TELINK_RAM_CODE_RET_REGION     RAMABLE_REGION
	#define TELINK_RAM_DATA_REGION         RAM_ILM_N
	#define TELINK_RAM_CODE_BLE_REGION     RAM_ILM_N
#else
#if CONFIG_MCUBOOT
	/* MCUboot image (mcu_boot_tlx.ld): keep the extra RAM_ILM_N area.
	 * It is required to avoid bootloader RAM overflow with LZMA.
	 */
	#define TELINK_RAM_REGION              RAMABLE_REGION
	#define TELINK_RAM_CODE_RET_REGION     RAMABLE_REGION
	#define TELINK_RAM_DATA_REGION         RAM_ILM_N
	#define TELINK_RAM_CODE_BLE_REGION     RAM_ILM_N
	#define TELINK_RAM_CODE_REGION         RAM_ILM_N
#else
	/* Application image (matter_retention_tl323x.ld): unified 160 KB SRAM,
	 * everything lives in the single contiguous RAM region.
	 * RAM_ILM_N is no longer used.
	 */
	#define TELINK_RAM_REGION              RAMABLE_REGION
	#define TELINK_RAM_CODE_REGION         RAMABLE_REGION
	#define TELINK_RAM_CODE_RET_REGION     RAMABLE_REGION
	#define TELINK_RAM_DATA_REGION         RAMABLE_REGION
	#define TELINK_RAM_CODE_BLE_REGION     RAMABLE_REGION
#endif
#endif
