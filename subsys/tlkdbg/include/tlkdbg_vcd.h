/*
 * Copyright (c) 2026 Telink Semiconductor
 * SPDX-License-Identifier: Apache-2.0
 *
 * VCD channel: 32-bit encoded words, written inline. The TL751X/USB1 word
 * layout is used, which is also what the UART transport expects.
 *
 * Reserved IDs: framework-reserved IDs are allocated from 0 upward. SID 0 of
 * the 24-bit type is the absolute-time anchor, so application 24-bit signals
 * start at 1.
 */
#ifndef TLKDBG_VCD_H_
#define TLKDBG_VCD_H_

#include <zephyr/irq.h>
#include <stimer.h>
#include "tlkdbg_cfg.h"
#include "tlkdbg_tr.h"

#define tlkdbg_vcd_word(w) tlkdbg_tr_uart_write_word(w)

/* Used in thread context; the _irq variants are for callers that already run
 * with interrupts disabled (e.g. inside an ISR). */
#define vcd_tick(id)                                                   \
	do {                                                           \
		unsigned int _k = irq_lock();                          \
		unsigned int _t = stimer_get_tick();                   \
		tlkdbg_vcd_word(3u | (((id) & 0x7fu) << 24) |          \
				(_t & 0xfffff8u));                     \
		irq_unlock(_k);                                        \
	} while (0)

#define vcd_tick_irq(id)                                               \
	do {                                                           \
		unsigned int _t = stimer_get_tick();                   \
		tlkdbg_vcd_word(3u | (((id) & 0x7fu) << 24) |          \
				(_t & 0xfffff8u));                     \
	} while (0)

#define vcd_task(id, level)                                            \
	do {                                                           \
		unsigned int _k = irq_lock();                          \
		unsigned int _t = stimer_get_tick();                   \
		tlkdbg_vcd_word(((level) ? 7u : 5u) |                  \
				(((id) & 0x7fu) << 24) |               \
				(_t & 0xfffff8u));                     \
		irq_unlock(_k);                                        \
	} while (0)

#define vcd_b16(id, d)                                                 \
	do {                                                           \
		unsigned int _k = irq_lock();                          \
		tlkdbg_vcd_word(1u | (((id) & 0x3fu) << 24) |          \
				(((unsigned int)(d) & 0xffffu) << 8)); \
		irq_unlock(_k);                                        \
	} while (0)

#define vcd_b16_irq(id, d)                                             \
	tlkdbg_vcd_word(1u | (((id) & 0x3fu) << 24) |                  \
			(((unsigned int)(d) & 0xffffu) << 8))

#define vcd_b24(id, d)                                                 \
	do {                                                           \
		unsigned int _k = irq_lock();                          \
		tlkdbg_vcd_word((((id) & 0x7fu) << 1) |                \
				(((unsigned int)(d) & 0xffffffu) << 8)); \
		irq_unlock(_k);                                        \
	} while (0)

#define vcd_b24_irq(id, d)                                             \
	tlkdbg_vcd_word((((id) & 0x7fu) << 1) |                        \
			(((unsigned int)(d) & 0xffffffu) << 8))

#define vcd_sync()                                                     \
	do {                                                           \
		unsigned int _k = irq_lock();                          \
		tlkdbg_vcd_word(0u);                                   \
		tlkdbg_vcd_word(0u);                                   \
		tlkdbg_vcd_word(0u);                                   \
		tlkdbg_vcd_word(0u);                                   \
		irq_unlock(_k);                                        \
	} while (0)

#endif /* TLKDBG_VCD_H_ */
