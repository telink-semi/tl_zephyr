/*
 * Copyright (c) 2026 Telink Semiconductor
 * SPDX-License-Identifier: Apache-2.0
 *
 * Optional poll loop. TX is poll-only by design: no UART or DMA interrupt is
 * registered. This thread only uses the kernel's existing system tick via
 * k_sleep(); it does not claim any interrupt vector.
 */
#include <zephyr/kernel.h>

#include "tlkdbg.h"

static K_THREAD_STACK_DEFINE(s_tlkdbg_poll_stack, CONFIG_TLKDBG_POLL_STACK_SIZE);
static struct k_thread s_tlkdbg_poll_thread;
static bool s_tlkdbg_poll_started;

static void tlkdbg_poll_fn(void *arg1, void *arg2, void *arg3)
{
	ARG_UNUSED(arg1);
	ARG_UNUSED(arg2);
	ARG_UNUSED(arg3);

	for (;;) {
		tlkdbg_tr_uart_poll();
		k_sleep(K_USEC(CONFIG_TLKDBG_POLL_PERIOD_US));
	}
}

void tlkdbg_poll_thread_start(void)
{
	if (s_tlkdbg_poll_started) {
		return;
	}
	s_tlkdbg_poll_started = true;

	k_thread_create(&s_tlkdbg_poll_thread, s_tlkdbg_poll_stack,
			K_THREAD_STACK_SIZEOF(s_tlkdbg_poll_stack), tlkdbg_poll_fn,
			NULL, NULL, NULL, K_PRIO_PREEMPT(1), 0, K_NO_WAIT);
	k_thread_name_set(&s_tlkdbg_poll_thread, "tlkdbg_poll");
}
