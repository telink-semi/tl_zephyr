/*
 * Copyright (c) 2026 Telink Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef TLSR_PROFILER_EXT_H
#define TLSR_PROFILER_EXT_H

#if CONFIG_TLSR_PROFILER

void tlsr_profiler_ext_init(void);
void tlsr_profiler_ext_reset(void);
void tlsr_profiler_ext_mark(const char *label);
void tlsr_profiler_ext_show_data(void);

#define tlsr_profiler_ini()         tlsr_profiler_ext_init()
#define tlsr_profiler_rst()         tlsr_profiler_ext_reset()
#define tlsr_profiler_set(label)    tlsr_profiler_ext_mark(label)
#define tlsr_profiler_show()        tlsr_profiler_ext_show_data()

#else

#define tlsr_profiler_ini()
#define tlsr_profiler_rst()
#define tlsr_profiler_set(label)
#define tlsr_profiler_show()

#endif /* CONFIG_TLSR_PROFILER */

#endif /* TLSR_PROFILER_EXT_H */
