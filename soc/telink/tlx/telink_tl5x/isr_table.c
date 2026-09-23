/*
 * Copyright (c) 2025 Telink Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/sw_isr_table.h>
#include <zephyr/kernel.h>
#include "gpio.h"

#define _IRQ_VECTOR_DT_NODE            DT_NODELABEL(plic0)
#define _IRQ_VECTOR_DEVICE             DEVICE_DT_GET(_IRQ_VECTOR_DT_NODE)
#define _IRQ_VECTOR_NUMBER             UTIL_OR(DT_PROP(_IRQ_VECTOR_DT_NODE, riscv_ndev), 0)
#define _IRQ_VECTOR_PLIC_COMP_ADDR     (DT_REG_ADDR(_IRQ_VECTOR_DT_NODE) + 0x130)
#define _IRQ_VECTOR_TABLE_RECORD(i, _) ((uintptr_t)&_isr_vectored_wrapper)

extern void _isr_vectored_wrapper(void);

uintptr_t __irq_vector_table _irq_vector_table[_IRQ_VECTOR_NUMBER + 1] = {
	((uintptr_t)&_isr_wrapper), LISTIFY(_IRQ_VECTOR_NUMBER, _IRQ_VECTOR_TABLE_RECORD, (,)) };

void _irq_vector_handler(uint32_t num)
{
	TZH_DBG_CHN13_HIGH;

	struct _isr_table_entry *entry = &_sw_isr_table[CONFIG_2ND_LVL_ISR_TBL_OFFSET + num];
	uint32_t mie_bkp = csr_read_clear(mie, MIP_MTIP | MIP_MSIP);

#ifdef CONFIG_PLIC_TELINK_SHELL_IRQ_COUNT
	extern void plic_irq_inc_irq_count(const struct device *dev, uint8_t cpu_id,
					   uint32_t local_irq);

	plic_irq_inc_irq_count(_IRQ_VECTOR_DEVICE, arch_curr_cpu()->id, num);
#endif /* CONFIG_PLIC_TELINK_SHELL_IRQ_COUNT */
	csr_set(mstatus, MSTATUS_IEN);
	entry->isr(entry->arg);
	csr_clear(mstatus, MSTATUS_IEN);
	csr_write(mie, mie_bkp);
	*(volatile uint32_t *)(_IRQ_VECTOR_PLIC_COMP_ADDR) = num;

	TZH_DBG_CHN13_LOW;
}
