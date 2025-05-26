/*
 * Copyright (c) 2025 Telink Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef RISCV_TELINK_W91_SYS_CLOCK_H
#define RISCV_TELINK_W91_SYS_CLOCK_H

#include <stdint.h>

uint64_t sys_clock_w91_get_time_ms(void);
uint64_t sys_clock_w91_get_time_us(void);
uint64_t sys_clock_w91_get_time_ns(void);

#endif /* RISCV_TELINK_W91_SYS_CLOCK_H */
