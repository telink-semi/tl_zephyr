/*
 * Copyright (c) 2026 Telink Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef TLSR_PROFILER_H
#define TLSR_PROFILER_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifndef TLSR_PROFILER_ITEMS_CNT
#define TLSR_PROFILER_ITEMS_CNT CONFIG_TLSR_PROFILER_ITEMS
#endif /* TLSR_PROFILER_ITEMS_CNT */

typedef uint64_t (*tlsr_profiler_get_tick)(void);
typedef int (*tlsr_profiler_printf)(const char*, ...);

struct tlsr_profiler {
	tlsr_profiler_get_tick get_tick_func;
	tlsr_profiler_printf printf_func;
	size_t index;
	struct {
		const char *label;
		uint64_t tick;
	} items[TLSR_PROFILER_ITEMS_CNT];
};

void tlsr_profiler_init(struct tlsr_profiler *prof);
void tlsr_profiler_reset(struct tlsr_profiler *prof);
void tlsr_profiler_set_get_tick(struct tlsr_profiler *prof, tlsr_profiler_get_tick func);
void tlsr_profiler_set_printf(struct tlsr_profiler *prof, tlsr_profiler_printf func);
void tlsr_profiler_mark(struct tlsr_profiler *prof, const char *label);
size_t tlsr_profiler_get_items_count(const struct tlsr_profiler *prof);
uint64_t tlsr_profiler_get_item_tick(const struct tlsr_profiler *prof, size_t index);
const char *tlsr_profiler_get_item_label(const struct tlsr_profiler *prof, size_t index);
void tlsr_profiler_show_data(const struct tlsr_profiler *prof);

#endif /* TLSR_PROFILER_H */
