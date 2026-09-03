/*
 * Copyright (c) 2026 Telink Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(telink, CONFIG_KERNEL_LOG_LEVEL);

void telink_zero_isr(void)
{
	LOG_WRN("PLIC zero isr");
}
