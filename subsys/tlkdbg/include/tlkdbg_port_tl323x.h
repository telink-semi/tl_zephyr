/*
 * Copyright (c) 2026 Telink Semiconductor
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef TLKDBG_PORT_TL323X_H_
#define TLKDBG_PORT_TL323X_H_

#include <stimer.h>
#include "tlkdbg_tr.h"

/* Free running 24 MHz tick. The PC "mhz" must be 24. */
#define tlkdbg_port_tick() stimer_get_tick()

/* Wrap-safe: true once @p us microseconds elapsed since @p ref. */
#define tlkdbg_port_tick_exceed(ref, us) clock_time_exceed((ref), (us))

#endif /* TLKDBG_PORT_TL323X_H_ */
