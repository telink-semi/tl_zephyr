/*
 * Copyright (c) 2026 Telink Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "tlsr_profiler.h"

void tlsr_profiler_init(struct tlsr_profiler *prof)
{
	prof->get_tick_func = NULL;
	prof->printf_func = NULL;
	prof->index = 0;
}

void tlsr_profiler_reset(struct tlsr_profiler *prof)
{
	prof->index = 0;
}

void tlsr_profiler_set_get_tick(struct tlsr_profiler *prof, tlsr_profiler_get_tick func)
{
	prof->get_tick_func = func;
}

void tlsr_profiler_set_printf(struct tlsr_profiler *prof, tlsr_profiler_printf func)
{
	prof->printf_func = func;
}

void tlsr_profiler_mark(struct tlsr_profiler *prof, const char *label)
{
	if (prof->get_tick_func && prof->index < TLSR_PROFILER_ITEMS_CNT) {
		prof->items[prof->index].tick = prof->get_tick_func();
		prof->items[prof->index].label = label;
		prof->index++;
	} else {
		prof->printf_func("PROFILING OVRFLOW\n");
	}
}

size_t tlsr_profiler_get_items_count(const struct tlsr_profiler *prof)
{
	return prof->index;
}

uint64_t tlsr_profiler_get_item_tick(const struct tlsr_profiler *prof, size_t index)
{
	uint64_t tick = 0;

	if (index < tlsr_profiler_get_items_count(prof)) {
		tick = prof->items[index].tick;
	}
	return tick;
}

const char *tlsr_profiler_get_item_label(const struct tlsr_profiler *prof, size_t index)
{
	const char *label = NULL;

	if (index < tlsr_profiler_get_items_count(prof)) {
		label = prof->items[index].label;
	}
	return label;
}

void tlsr_profiler_show_data(const struct tlsr_profiler *prof)
{
	if (prof->printf_func) {
		prof->printf_func("PROFILING DATA\n");
		for (size_t i = 0; i < tlsr_profiler_get_items_count(prof); ++i) {
			prof->printf_func("%zu\t%s\t%llu\n", i,
				tlsr_profiler_get_item_label(prof, i), tlsr_profiler_get_item_tick(prof, i));
		}
		prof->printf_func("END OF PROFILING DATA\n");
	}
}
