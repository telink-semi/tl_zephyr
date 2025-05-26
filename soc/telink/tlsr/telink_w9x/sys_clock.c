/*
 * Copyright (c) 2025 Telink Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <sys_clock.h>
#include <zephyr/kernel.h>

/**
 * @brief Get the system time in milliseconds
 */
uint64_t sys_clock_w91_get_time_ms(void)
{
	uint64_t cycles = sys_clock_cycle_get_64();

	return (cycles / CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC) * MSEC_PER_SEC +
	       ((cycles % CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC) * MSEC_PER_SEC) /
		       CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC;
}

/**
 * @brief Get the system time in microseconds
 */
uint64_t sys_clock_w91_get_time_us(void)
{
	uint64_t cycles = sys_clock_cycle_get_64();

	return (cycles / CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC) * USEC_PER_SEC +
	       ((cycles % CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC) * USEC_PER_SEC) /
		       CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC;
}

/**
 * @brief Get the system time in nanoseconds
 */
uint64_t sys_clock_w91_get_time_ns(void)
{
	uint64_t cycles = sys_clock_cycle_get_64();

	return (cycles / CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC) * NSEC_PER_SEC +
	       ((cycles % CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC) * NSEC_PER_SEC) /
		       CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC;
}
