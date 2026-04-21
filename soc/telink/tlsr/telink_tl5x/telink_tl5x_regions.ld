/*
 * Copyright (c) 2024 Telink Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

 #if !CONFIG_SOC_SERIES_RISCV_TELINK_TLX_RETENTION
	#if CONFIG_SOC_RISCV_TELINK_TL523X
		#define TELINK_RAM_REGION              RAMILM
		#define TELINK_RAM_CODE_REGION         RAMILM
		#define TELINK_RAM_CODE_RET_REGION     RAMILM
	#endif

#else

#endif /* CONFIG_SOC_SERIES_RISCV_TELINK_TLX_RETENTION */
