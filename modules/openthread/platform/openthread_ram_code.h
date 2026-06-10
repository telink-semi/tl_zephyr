/*
 * Copyright (c) 2024~2026 Telink Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef OPENTHREAD_RAM_CODE_H
#define OPENTHREAD_RAM_CODE_H

#if CONFIG_OPENTHREAD_SED_RAM_CODE
/* put sed code into Retention RAM */
#define OT_SED_RAM  __attribute__((section(".ram_code")))
#else
/* default in Flash (text section) */
#define OT_SED_RAM
#endif

#endif /* OPENTHREAD_RAM_CODE_H */
