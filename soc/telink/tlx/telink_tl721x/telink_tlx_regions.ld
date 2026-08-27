/*
 * Copyright (c) 2026 Telink Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#if !CONFIG_SOC_SERIES_RISCV_TELINK_TLX_RETENTION
	#define TELINK_RAM_REGION              RAMILM
	#define TELINK_RAM_CODE_REGION         RAMILM
	#define TELINK_RAM_CODE_RET_REGION     RAMILM
#else
	#define TELINK_RAM_REGION              RAMABLE_REGION
	#define TELINK_RAM_CODE_RET_REGION     RAMABLE_REGION
	#define TELINK_RAM_DATA_REGION         RAM_ILM_N
	#define TELINK_RAM_CODE_BLE_REGION     RAM_DLM
	#define TELINK_RAM_CODE_REGION         RAM_ILM_N
#endif
