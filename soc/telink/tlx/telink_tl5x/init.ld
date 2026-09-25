/*
 * Copyright (c) 2024 Antmicro <www.antmicro.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#if CONFIG_MCUBOOT
. += (CONFIG_ROM_START_OFFSET + CONFIG_MCUBOOT_START_OFFSET);
#endif
KEEP(*(.init.*))
KEEP(*(.flash_data))
#if CONFIG_RISCV_VECTORED_MODE
. = ALIGN(CONFIG_RISCV_TRAP_HANDLER_ALIGNMENT);
KEEP(*(_IRQ_VECTOR_TABLE_SECTION_SYMS))
#endif /* CONFIG_RISCV_VECTORED_MODE */
