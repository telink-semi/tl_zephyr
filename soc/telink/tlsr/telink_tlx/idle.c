/*
 * Copyright (c) 2024 Telink Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/toolchain.h>
#include <zephyr/tracing/tracing.h>

#ifdef CONFIG_ARCH_HAS_CUSTOM_CPU_IDLE
__GENERIC_SECTION(.ram_code) void arch_cpu_idle(void)
{
	sys_trace_idle();
	__asm__ volatile("wfi");
	irq_unlock(MSTATUS_IEN);
}
#endif

