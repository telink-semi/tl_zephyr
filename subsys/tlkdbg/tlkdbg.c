/*
 * Copyright (c) 2026 Telink Semiconductor
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/kernel.h>

#include "tlkdbg.h"

#if CONFIG_TLKDBG_PM
#include <zephyr/pm/pm.h>
#endif

volatile unsigned int tlkdbg_stat_ep8_full_hits;
volatile unsigned int tlkdbg_stat_drop_log;
volatile unsigned int tlkdbg_stat_vcd_words;
volatile unsigned int tlkdbg_stat_vcd_peak_1ms;

static unsigned int s_tlkdbg_last_sample;

void tlkdbg_stat_sample_1ms(void)
{
	unsigned int now = tlkdbg_stat_vcd_words;

	if (now - s_tlkdbg_last_sample > tlkdbg_stat_vcd_peak_1ms) {
		tlkdbg_stat_vcd_peak_1ms = now - s_tlkdbg_last_sample;
	}
	s_tlkdbg_last_sample = now;
}

const tlkdbg_transport_t tlkdbg_tr_null = {
	.name = "null",
	.mtu = 0,
	.init = NULL,
	.write = NULL,
	.busy = NULL,
	.flush = NULL,
	.reset = NULL,
};

#if CONFIG_TLKDBG_PM
/* The UART TX ring lives in .bss and is not preserved by a deep-retention
 * wake, so drain it before the core sleeps. On suspend-to-idle the state is
 * kept and the 24 MHz stimer continues; the next ms sample covers the sleep. */
static void tlkdbg_pm_state_entry(enum pm_state state)
{
	ARG_UNUSED(state);

	tlkdbg_tr_uart_accounting();
	(void)tlkdbg_tr_uart_drain();
}

static struct pm_notifier s_tlkdbg_pm_notifier = {
	.state_entry = tlkdbg_pm_state_entry,
};
#endif

int tlkdbg_init(void)
{
	tlkdbg_log_unregister_all();
	tlkdbg_tr_uart_init();
	tlkdbg_log_register(&tlkdbg_tr_uart_log);
	tlkdbg_log_init();

#if defined(TLKDBG_LOG_USE_HASH_LABELS)
	(void)tlkdbg_log_table_info_write(TLKDBG_LABEL_HASH_SEED,
					  TLKDBG_LABEL_TABLE_HASH);
#endif

#if CONFIG_TLKDBG_PM
	pm_notifier_register(&s_tlkdbg_pm_notifier);
#endif

#if CONFIG_TLKDBG_POLL_THREAD
	extern void tlkdbg_poll_thread_start(void);

	tlkdbg_poll_thread_start();
#endif

	return 0;
}
