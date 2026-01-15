/*
 * Copyright (c) 2026 Telink Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "tlsr_profiler.h"
#include <stimer.h>
#include <zephyr/sys/printk.h>

static struct tlsr_profiler tlsr_profiler_ext;

static uint64_t tlsr_profiler_ext_get_tick(void)
{
	return stimer_get_tick();
}

static int tlsr_profiler_ext_printf(const char *fmt, ...)
{
	va_list args;

	va_start(args, fmt);
	vprintk(fmt, args);
	va_end(args);
	return 0;
}

void tlsr_profiler_ext_init(void)
{
	tlsr_profiler_init(&tlsr_profiler_ext);
	tlsr_profiler_set_get_tick(&tlsr_profiler_ext, tlsr_profiler_ext_get_tick);
	tlsr_profiler_set_printf(&tlsr_profiler_ext, tlsr_profiler_ext_printf);
}

void tlsr_profiler_ext_reset(void)
{
	tlsr_profiler_reset(&tlsr_profiler_ext);
}

void tlsr_profiler_ext_mark(const char *label)
{
	tlsr_profiler_mark(&tlsr_profiler_ext, label);
}

void tlsr_profiler_ext_show_data(void)
{
	tlsr_profiler_show_data(&tlsr_profiler_ext);
}
