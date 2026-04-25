/*
 * Copyright (c) 2024 Telink Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/toolchain.h>
#include <zephyr/tracing/tracing.h>

#include <plmt.h>

#if CONFIG_ARCH_HAS_CUSTOM_CPU_IDLE || CONFIG_ARCH_HAS_CUSTOM_CPU_ATOMIC_IDLE

/* Due to silicon bug in B92 platform, the WFI emulation is implemented */
static volatile bool __irq_pending;
__attribute__((used)) volatile uint32_t mstatus_flag = 0;
__attribute__((used)) volatile uint32_t mie_flag = 0;

__attribute__((used)) volatile uint64_t AAA_mcmp = 0;
__attribute__((used)) volatile uint64_t AAA_mtime = 0;
static ALWAYS_INLINE void riscv_idle(unsigned int key)
{
	sys_trace_idle();
	__irq_pending = true;
	irq_unlock(key);

	/* Wait for interrupt */
	#if CONFIG_SOC_RISCV_TELINK_TL322X || CONFIG_SOC_RISCV_TELINK_TL523X
		mstatus_flag = csr_read(mstatus);
		mie_flag = csr_read(mie);
		AAA_mcmp = mtime_get_cmp_value();
		AAA_mtime = mtime_get_value();
		__asm__ volatile("wfi");
	#else
		mstatus_flag = csr_read(mstatus);
		mie_flag = csr_read(mie);
		while (__irq_pending) {
			AAA_mcmp = mtime_get_cmp_value();
			AAA_mtime = mtime_get_value();
		}
	#endif
}

void __soc_handle_irq(unsigned long mcause)
{
    __irq_pending = false;
    __asm__ volatile (
        "li t1, 1\n\t"
        "sll t0, t1, %0\n\t"
        "csrrc t1, mip, t0"
        : 
        : "r"(mcause)
        : "t0", "t1", "memory"
    );
}
#endif /* CONFIG_ARCH_HAS_CUSTOM_CPU_IDLE || CONFIG_ARCH_HAS_CUSTOM_CPU_ATOMIC_IDLE */

#if CONFIG_ARCH_HAS_CUSTOM_CPU_IDLE
void arch_cpu_idle(void)
{
	riscv_idle(MSTATUS_IEN);
}
#endif /* CONFIG_ARCH_HAS_CUSTOM_CPU_IDLE */


#if CONFIG_ARCH_HAS_CUSTOM_CPU_ATOMIC_IDLE
void arch_cpu_atomic_idle(unsigned int key)
{
	riscv_idle(key);
}
#endif /* CONFIG_ARCH_HAS_CUSTOM_CPU_ATOMIC_IDLE */
