/*
 * Copyright (c) 2025 Telink Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/sw_isr_table.h>
#include <zephyr/kernel.h>
#include "compiler.h"
#include "plic.h"

#define _IRQ_VECTOR_NUMBER             UTIL_OR(DT_PROP(DT_NODELABEL(plic0), riscv_ndev), 0)
#define _IRQ_VECTOR_PLIC_COMP_ADDR     (DT_REG_ADDR(DT_NODELABEL(plic0)) + 0x200004)
#define _IRQ_VECTOR_TABLE_RECORD(i, _) ((uintptr_t)&_isr_vectored_wrapper)

extern void _isr_vectored_wrapper(void);
extern void _higher_pri_isr_vectored_wrapper(void);

uintptr_t __irq_vector_table _irq_vector_table[_IRQ_VECTOR_NUMBER + 1] = {
	[0] = ((uintptr_t)&_isr_wrapper),
	[IRQ_SYSTIMER] = ((uintptr_t)&_higher_pri_isr_vectored_wrapper),
	[(IRQ_SYSTIMER + 1) ... (IRQ_ZB_RT - 1)] = ((uintptr_t)&_isr_vectored_wrapper),
	[IRQ_ZB_RT] = ((uintptr_t)&_higher_pri_isr_vectored_wrapper),
	[IRQ_ZB_RT + 1 ... _IRQ_VECTOR_NUMBER] = ((uintptr_t)&_isr_vectored_wrapper),
};

_attribute_ram_code_
void _irq_vector_handler(uint32_t num)
{
	struct _isr_table_entry *entry = &_sw_isr_table[CONFIG_2ND_LVL_ISR_TBL_OFFSET + num];
	uint32_t mie_bkp = csr_read_clear(mie, MIP_MTIP | MIP_MSIP);

	csr_set(mstatus, MSTATUS_IEN);
	entry->isr(entry->arg);
	csr_clear(mstatus, MSTATUS_IEN);
	csr_write(mie, mie_bkp);
	*(volatile uint32_t *)(_IRQ_VECTOR_PLIC_COMP_ADDR) = num;
}
