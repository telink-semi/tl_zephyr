/*
 * Copyright (c) 2026 Telink Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Zephyr M24SR driver demonstration.
 */

#include <m24sr.h>

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(main, LOG_LEVEL_INF);

int main(void)
{
	LOG_INF("application started");

	M24SR_Init(0xac, M24SR_GPO_INTERRUPT);

	LOG_INF("application finished");
	return 0;
}
