/*
 * Copyright (c) 2023 Telink Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef SOC_RISCV_TELINK_W91_SOC_OFFSETS_H
#define SOC_RISCV_TELINK_W91_SOC_OFFSETS_H

#ifdef CONFIG_RISCV_SOC_OFFSETS

/* Telink W91 specific registers. */
#if defined(CONFIG_TELINK_W91_PFT) && defined(CONFIG_ANDES_HWDSP)
#define GEN_SOC_OFFSET_SYMS()                                                                      \
	GEN_OFFSET_SYM(soc_esf_t, mxstatus);                                                       \
	GEN_OFFSET_SYM(soc_esf_t, ucode)

#elif defined(CONFIG_TELINK_W91_PFT)
#define GEN_SOC_OFFSET_SYMS() GEN_OFFSET_SYM(soc_esf_t, mxstatus)

#elif defined(CONFIG_ANDES_HWDSP)
#define GEN_SOC_OFFSET_SYMS() GEN_OFFSET_SYM(soc_esf_t, ucode)

#endif

#endif /* CONFIG_RISCV_SOC_OFFSETS */

#endif /* SOC_RISCV_TELINK_W91_SOC_OFFSETS_H*/
