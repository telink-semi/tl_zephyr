/*
 * Copyright (c) 2024 Telink Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <offsets.h>
#include <zephyr/toolchain.h>

#include <soc.h>

#define NDS_MXSTATUS                 0x7C4

/* Exports */
#ifdef CONFIG_RISCV_SOC_CONTEXT_SAVE
GTEXT(__soc_save_context)
GTEXT(__soc_restore_context)
#endif

#ifdef CONFIG_RISCV_SOC_CONTEXT_SAVE

SECTION_FUNC(exception.other, __soc_save_context)

#ifdef CONFIG_TELINK_TLX_PFT
	csrr t0, NDS_MXSTATUS
#endif
#ifdef CONFIG_ANDES_HWDSP
	csrr t1, ucode
#endif

#ifdef CONFIG_TELINK_TLX_PFT
	sw t0, __soc_esf_t_mxstatus_OFFSET(a0)
#endif
#ifdef CONFIG_ANDES_HWDSP
	sw t1, __soc_esf_t_ucode_OFFSET(a0)
#endif
	ret

SECTION_FUNC(exception.other, __soc_restore_context)

#ifdef CONFIG_TELINK_TLX_PFT
	lw t0, __soc_esf_t_mxstatus_OFFSET(a0)
#endif
#ifdef CONFIG_ANDES_HWDSP
	lw t1, __soc_esf_t_ucode_OFFSET(a0)
#endif

#ifdef CONFIG_TELINK_TLX_PFT
	csrw NDS_MXSTATUS, t0
#endif
#ifdef CONFIG_ANDES_HWDSP
	csrw ucode, t1
#endif
	ret

#endif /* CONFIG_RISCV_SOC_CONTEXT_SAVE */
