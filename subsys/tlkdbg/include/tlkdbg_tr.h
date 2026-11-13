/*
 * Copyright (c) 2026 Telink Semiconductor
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef TLKDBG_TR_H_
#define TLKDBG_TR_H_

#include "tlkdbg_transport.h"

/* LOG transport over the UART stream. */
extern const tlkdbg_transport_t tlkdbg_tr_uart_log;

/* Sink that throws everything away. */
extern const tlkdbg_transport_t tlkdbg_tr_null;

extern volatile unsigned int tlkdbg_stat_ep8_full_hits;
extern volatile unsigned int tlkdbg_stat_drop_log;
extern volatile unsigned int tlkdbg_stat_vcd_words;
extern volatile unsigned int tlkdbg_stat_vcd_peak_1ms;

void tlkdbg_stat_sample_1ms(void);

/* UART backend entry points. The VCD side is bound at compile time. */
void tlkdbg_tr_uart_init(void);
void tlkdbg_tr_uart_write_word(unsigned int w);
unsigned int tlkdbg_tr_uart_drop_count(void);

/* Drive the transmitter. TX is poll-only: no UART interrupt is claimed. */
void tlkdbg_tr_uart_poll(void);

unsigned int tlkdbg_tr_uart_pending(void);

/* Flush everything and wait until the UART shifted it out. */
int tlkdbg_tr_uart_drain(void);

/* Emit one cumulative "TXC" accounting record. */
void tlkdbg_tr_uart_accounting(void);

#endif /* TLKDBG_TR_H_ */
