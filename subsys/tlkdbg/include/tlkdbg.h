/*
 * Copyright (c) 2026 Telink Semiconductor
 * SPDX-License-Identifier: Apache-2.0
 *
 * Public API for the tlkdbg VCD/LOG UART stream.
 */
#ifndef TLKDBG_H_
#define TLKDBG_H_

#include "tlkdbg_cfg.h"
#include "tlkdbg_transport.h"
#include "tlkdbg_log.h"
#include "tlkdbg_tr.h"
#include "tlkdbg_vcd.h"

/* Initialise the transport. When CONFIG_TLKDBG_POLL_THREAD is set this also
 * starts the poll thread. Returns 0 on success. */
int tlkdbg_init(void);

#endif /* TLKDBG_H_ */
