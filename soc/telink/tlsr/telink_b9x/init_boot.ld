/*
 * Copyright (c) 2026 Antmicro <www.antmicro.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

. += (CONFIG_ROM_START_OFFSET + CONFIG_MCUBOOT_START_OFFSET);
KEEP(*(.init.*))
