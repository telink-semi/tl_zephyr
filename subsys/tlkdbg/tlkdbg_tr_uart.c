/*
 * Copyright (c) 2026 Telink Semiconductor
 * SPDX-License-Identifier: Apache-2.0
 *
 * VCD + LOG transport over one UART with lightweight framing:
 *
 *   [A5 5A][ch][len:2 LE][payload][crc16 LE]
 *   ch 0x01 = LOG, ch 0x02 = VCD (whole 32-bit words)
 *
 * TX is poll-only. The transport never enables or registers a UART or DMA
 * interrupt: the application (or the optional poll thread) calls
 * tlkdbg_tr_uart_poll().
 */
#include <zephyr/kernel.h>
#include <zephyr/irq.h>

#include "tlkdbg_tr.h"
#include "tlkdbg_log.h"
#include "tlkdbg_cfg.h"
#include "tlkdbg_port_tl323x.h"

#include "uart.h"
#include "dma.h"
#include "lib/include/clock.h"

#define TLKDBG_UART_PORT     ((uart_num_e)CONFIG_TLKDBG_UART_INSTANCE)
#define TLKDBG_UART_DMA_CHN  ((dma_chn_e)CONFIG_TLKDBG_UART_DMA_CH)
#define TLKDBG_UART_RING_MASK (TLKDBG_UART_RING_SIZE - 1u)

/* Implemented in tlkdbg_uart_ll_tl323x.c. */
void tlkdbg_uart_hw_init(void);

static unsigned char s_tlkdbg_uart_tx_buf[TLKDBG_UART_RING_SIZE] __aligned(4);
static unsigned char s_tlkdbg_uart_tail[4] __aligned(4);

static volatile unsigned int s_tlkdbg_uart_wptr;
static volatile unsigned int s_tlkdbg_uart_rptr;
static volatile unsigned char s_tlkdbg_uart_tx_busy;
static volatile unsigned char s_tlkdbg_uart_tail_dma;
static volatile unsigned short s_tlkdbg_uart_dma_len;
static volatile unsigned int s_tlkdbg_uart_drop_cnt;

static unsigned char s_tlkdbg_uart_vcd_frame[TLKDBG_UART_VCD_BATCH];
static unsigned short s_tlkdbg_uart_vcd_len;
static unsigned int s_tlkdbg_uart_vcd_tick;

static unsigned char s_tlkdbg_uart_ready;

static void tlkdbg_uart_vcd_flush(void);

/* Cumulative accounting counters (see the "TXC" record). */
static unsigned int s_tlkdbg_uart_vcd_words;
static unsigned int s_tlkdbg_uart_vcd_drop_words;
static unsigned int s_tlkdbg_uart_log_tx;
static unsigned int s_tlkdbg_uart_log_drop;
static unsigned int s_tlkdbg_uart_ms_acc;
static unsigned int s_tlkdbg_uart_ms_rem;
static unsigned int s_tlkdbg_uart_tick_last;

static unsigned int s_tlkdbg_uart_acct_tick;
static unsigned int s_tlkdbg_uart_last_tick;
static unsigned int s_tlkdbg_uart_last_anchor;

static unsigned int tlkdbg_uart_ms(void)
{
	unsigned int now = tlkdbg_port_tick();
	unsigned int d = now - s_tlkdbg_uart_tick_last;

	s_tlkdbg_uart_tick_last = now;
	s_tlkdbg_uart_ms_rem += d;
	s_tlkdbg_uart_ms_acc += s_tlkdbg_uart_ms_rem / TLKDBG_TICKS_PER_MS;
	s_tlkdbg_uart_ms_rem %= TLKDBG_TICKS_PER_MS;
	return s_tlkdbg_uart_ms_acc;
}

/* Push a 1..3 byte remainder through the DMA (word-aligned source required). */
static void tlkdbg_uart_send_tail(void)
{
	unsigned int pending = s_tlkdbg_uart_wptr - s_tlkdbg_uart_rptr;
	unsigned int i;

	for (i = 0; i < pending; i++) {
		s_tlkdbg_uart_tail[i] =
			s_tlkdbg_uart_tx_buf[(s_tlkdbg_uart_rptr + i) & TLKDBG_UART_RING_MASK];
	}
	for (; i < 4u; i++) {
		s_tlkdbg_uart_tail[i] = 0;
	}

	s_tlkdbg_uart_dma_len = (unsigned short)pending;
	s_tlkdbg_uart_tail_dma = 1;
	s_tlkdbg_uart_tx_busy = 1;
	(void)uart_send_dma(TLKDBG_UART_PORT, s_tlkdbg_uart_tail, 4);
}

static void tlkdbg_uart_ring_compact(void)
{
	unsigned int pending = s_tlkdbg_uart_wptr - s_tlkdbg_uart_rptr;
	unsigned int i;

	for (i = 0; i < pending; i++) {
		s_tlkdbg_uart_tx_buf[i] =
			s_tlkdbg_uart_tx_buf[(s_tlkdbg_uart_rptr + i) & TLKDBG_UART_RING_MASK];
	}
	s_tlkdbg_uart_rptr = 0;
	s_tlkdbg_uart_wptr = pending;
}

static void tlkdbg_uart_kick_dma(void)
{
	unsigned int key = irq_lock();
	unsigned int pending;
	unsigned int idx;
	unsigned int contig;
	unsigned int n;

	if (s_tlkdbg_uart_tx_busy) {
		irq_unlock(key);
		return;
	}

	pending = s_tlkdbg_uart_wptr - s_tlkdbg_uart_rptr;
	if (pending == 0) {
		irq_unlock(key);
		return;
	}

	if (pending < 4) {
		tlkdbg_uart_send_tail();
		irq_unlock(key);
		return;
	}

	idx = s_tlkdbg_uart_rptr & TLKDBG_UART_RING_MASK;
	contig = TLKDBG_UART_RING_SIZE - idx;

	n = pending;
	if (n > contig) {
		n = contig;
	}
	n &= ~3u;
	if (n > TLKDBG_UART_DMA_MAX) {
		n = TLKDBG_UART_DMA_MAX;
	}
	if (n == 0) {
		irq_unlock(key);
		return;
	}

	s_tlkdbg_uart_dma_len = (unsigned short)n;
	s_tlkdbg_uart_tx_busy = 1;
	(void)uart_send_dma(TLKDBG_UART_PORT, s_tlkdbg_uart_tx_buf + idx, n);

	irq_unlock(key);
}

/* Retire the burst in flight if the transmitter is done. Caller holds irq_lock. */
static int tlkdbg_uart_complete(void)
{
	if (!s_tlkdbg_uart_tx_busy ||
	    !uart_get_irq_status(TLKDBG_UART_PORT, UART_TXDONE_IRQ_STATUS)) {
		return 0;
	}

	uart_clr_irq_status(TLKDBG_UART_PORT, UART_TXDONE_IRQ_STATUS);
	s_tlkdbg_uart_rptr += s_tlkdbg_uart_dma_len;
	s_tlkdbg_uart_tx_busy = 0;
	if (s_tlkdbg_uart_tail_dma) {
		s_tlkdbg_uart_tail_dma = 0;
		tlkdbg_uart_ring_compact();
	}
	return 1;
}

void tlkdbg_tr_uart_poll(void)
{
	static unsigned int tail_tick;
	unsigned int pending;

	if (s_tlkdbg_uart_ready == 0) {
		return;
	}

	unsigned int key = irq_lock();

	(void)tlkdbg_uart_complete();

	pending = s_tlkdbg_uart_wptr - s_tlkdbg_uart_rptr;
	if (s_tlkdbg_uart_tx_busy || pending == 0) {
		irq_unlock(key);
		return;
	}

	/* Batch, but never hold a tail longer than 1 ms. */
	if (pending < TLKDBG_UART_POLL_MIN &&
	    tail_tick != 0 && !tlkdbg_port_tick_exceed(tail_tick, 1000u)) {
		irq_unlock(key);
		return;
	}
	tail_tick = tlkdbg_port_tick() | 1;

	irq_unlock(key);

	tlkdbg_uart_kick_dma();
}

unsigned int tlkdbg_tr_uart_pending(void)
{
	return s_tlkdbg_uart_wptr - s_tlkdbg_uart_rptr;
}

int tlkdbg_tr_uart_drain(void)
{
	if (s_tlkdbg_uart_ready == 0) {
		return -1;
	}

	tlkdbg_uart_vcd_flush();

	unsigned int start = tlkdbg_port_tick() | 1;

	for (;;) {
		unsigned int key = irq_lock();
		unsigned int pending;

		(void)tlkdbg_uart_complete();
		pending = s_tlkdbg_uart_wptr - s_tlkdbg_uart_rptr;

		if (!s_tlkdbg_uart_tx_busy && pending == 0) {
			irq_unlock(key);
			return 0;
		}

		if (!s_tlkdbg_uart_tx_busy) {
			tlkdbg_uart_kick_dma();
		}

		irq_unlock(key);

		if (tlkdbg_port_tick_exceed(start, TLKDBG_UART_DRAIN_TIMEOUT_MS * 1000u)) {
			return -1;
		}
	}
}

static int tlkdbg_uart_ring_write(const unsigned char *p, unsigned short len)
{
	unsigned int pending = s_tlkdbg_uart_wptr - s_tlkdbg_uart_rptr;

	/* Leave one burst worth of headroom for the transfer in flight. */
	if (pending + len > TLKDBG_UART_RING_SIZE - TLKDBG_UART_DMA_MAX) {
		s_tlkdbg_uart_drop_cnt++;
		return -1;
	}

	for (unsigned short i = 0; i < len; i++) {
		s_tlkdbg_uart_tx_buf[s_tlkdbg_uart_wptr & TLKDBG_UART_RING_MASK] = p[i];
		s_tlkdbg_uart_wptr++;
	}

	return (int)len;
}

static void tlkdbg_uart_count_drop(unsigned char ch, unsigned short len)
{
	s_tlkdbg_uart_drop_cnt++;
	if (ch == TLKDBG_CH_VCD) {
		s_tlkdbg_uart_vcd_drop_words += (unsigned int)(len / 4u);
	} else {
		s_tlkdbg_uart_log_drop++;
	}
}

static int tlkdbg_uart_emit(unsigned char ch, const unsigned char *payload,
			    unsigned short len)
{
	unsigned char hdr[TLKDBG_UART_HDR_LEN];
	unsigned char crcb[TLKDBG_FRAME_CRC_LEN];
	unsigned int pending;

	if (len == 0 || s_tlkdbg_uart_ready == 0) {
		return -1;
	}

	/* The peer's parser cannot resync inside a payload, so refuse frames
	 * above its MTU and account for them. */
	if (len > TLKDBG_UART_FRAME_MTU) {
		tlkdbg_uart_count_drop(ch, len);
		return -1;
	}

	hdr[0] = TLKDBG_FRAME_SYNC0;
	hdr[1] = TLKDBG_FRAME_SYNC1;
	hdr[2] = ch;
	hdr[3] = (unsigned char)(len & 0xff);
	hdr[4] = (unsigned char)((len >> 8) & 0xff);

	unsigned short crc = tlkdbg_crc16_update(0xffff, hdr + 2, 3);

	crc = tlkdbg_crc16_update(crc, payload, len);
	crcb[0] = (unsigned char)(crc & 0xff);
	crcb[1] = (unsigned char)((crc >> 8) & 0xff);

	/* One frame must reach the ring as an uninterrupted byte run: a VCD word
	 * from an ISR must never land inside the header/payload/CRC copy. */
	unsigned int key = irq_lock();

	pending = s_tlkdbg_uart_wptr - s_tlkdbg_uart_rptr;
	if (pending + TLKDBG_UART_HDR_LEN + len + TLKDBG_FRAME_CRC_LEN >
	    TLKDBG_UART_RING_SIZE - TLKDBG_UART_DMA_MAX) {
		tlkdbg_uart_count_drop(ch, len);
		irq_unlock(key);
		return -1;
	}

	if (tlkdbg_uart_ring_write(hdr, TLKDBG_UART_HDR_LEN) < 0 ||
	    tlkdbg_uart_ring_write(payload, len) < 0 ||
	    tlkdbg_uart_ring_write(crcb, TLKDBG_FRAME_CRC_LEN) < 0) {
		tlkdbg_uart_count_drop(ch, len);
		irq_unlock(key);
		return -1;
	}

	if (ch == TLKDBG_CH_LOG) {
		s_tlkdbg_uart_log_tx++;
	}

	irq_unlock(key);
	return 0;
}

static void tlkdbg_uart_vcd_flush(void)
{
	if (s_tlkdbg_uart_vcd_len != 0) {
		(void)tlkdbg_uart_emit(TLKDBG_CH_VCD, s_tlkdbg_uart_vcd_frame,
				       s_tlkdbg_uart_vcd_len);
		s_tlkdbg_uart_vcd_len = 0;
	}
}

static void tlkdbg_uart_put_word(unsigned int w)
{
	if (s_tlkdbg_uart_vcd_len != 0 &&
	    tlkdbg_port_tick_exceed(s_tlkdbg_uart_vcd_tick, TLKDBG_UART_FLUSH_US)) {
		tlkdbg_uart_vcd_flush();
	}

	if (s_tlkdbg_uart_vcd_len == 0) {
		s_tlkdbg_uart_vcd_tick = tlkdbg_port_tick();
	}

	s_tlkdbg_uart_vcd_frame[s_tlkdbg_uart_vcd_len++] = (unsigned char)(w & 0xff);
	s_tlkdbg_uart_vcd_frame[s_tlkdbg_uart_vcd_len++] = (unsigned char)((w >> 8) & 0xff);
	s_tlkdbg_uart_vcd_frame[s_tlkdbg_uart_vcd_len++] = (unsigned char)((w >> 16) & 0xff);
	s_tlkdbg_uart_vcd_frame[s_tlkdbg_uart_vcd_len++] = (unsigned char)((w >> 24) & 0xff);
	s_tlkdbg_uart_vcd_words++;
	tlkdbg_stat_vcd_words++;

	if (s_tlkdbg_uart_vcd_len >= TLKDBG_UART_VCD_BATCH) {
		tlkdbg_uart_vcd_flush();
	}
}

static int tlkdbg_word_is_tick(unsigned int w)
{
	return (w & 1u) && (((w >> 1) & 3u) == 1u);
}

static void tlkdbg_uart_put_ms(void)
{
	unsigned int ms = (tlkdbg_uart_ms() + 1u) & 0xffffffu;

	tlkdbg_uart_put_word((((unsigned int)TLKDBG_UART_MS_SID & 127u) << 1) | (ms << 8));
	s_tlkdbg_uart_last_anchor = tlkdbg_port_tick() | 1;
}

static int tlkdbg_uart_anchor_due(void)
{
#if (TLKDBG_UART_ANCHOR_MS > 0)
	return s_tlkdbg_uart_last_anchor == 0 ||
	       tlkdbg_port_tick_exceed(s_tlkdbg_uart_last_anchor,
				       (unsigned int)TLKDBG_UART_ANCHOR_MS * 1000u);
#else
	return 0;
#endif
}

void tlkdbg_tr_uart_write_word(unsigned int w)
{
	unsigned int now = tlkdbg_port_tick();

	if (tlkdbg_word_is_tick(w)) {
		if (tlkdbg_uart_anchor_due()) {
			tlkdbg_uart_put_ms();
		}
		s_tlkdbg_uart_last_tick = now;
	}
#if (TLKDBG_UART_TICK_MS > 0)
	else if (s_tlkdbg_uart_last_tick != 0 &&
		 tlkdbg_port_tick_exceed(s_tlkdbg_uart_last_tick,
					 (unsigned int)TLKDBG_UART_TICK_MS * 1000u)) {
		tlkdbg_uart_put_ms();
		tlkdbg_uart_put_word(3u | (now & 0xfffff8u));
		s_tlkdbg_uart_last_tick = now;
	}
#endif

	tlkdbg_uart_put_word(w);

#if (TLKDBG_UART_ACCOUNT_MS > 0)
	if (s_tlkdbg_uart_acct_tick == 0 ||
	    tlkdbg_port_tick_exceed(s_tlkdbg_uart_acct_tick,
				    (unsigned int)TLKDBG_UART_ACCOUNT_MS * 1000u)) {
		tlkdbg_tr_uart_accounting();
	}
#endif
}

void tlkdbg_tr_uart_accounting(void)
{
	unsigned int v[5];
	unsigned char d[20];

	if (s_tlkdbg_uart_ready == 0) {
		return;
	}

	tlkdbg_uart_vcd_flush();

	v[0] = s_tlkdbg_uart_vcd_words;
	v[1] = s_tlkdbg_uart_vcd_drop_words;
	v[2] = s_tlkdbg_uart_log_tx;
	v[3] = s_tlkdbg_uart_log_drop;
	v[4] = tlkdbg_uart_ms();

	for (unsigned int i = 0; i < 5; i++) {
		d[i * 4 + 0] = (unsigned char)(v[i] & 0xff);
		d[i * 4 + 1] = (unsigned char)((v[i] >> 8) & 0xff);
		d[i * 4 + 2] = (unsigned char)((v[i] >> 16) & 0xff);
		d[i * 4 + 3] = (unsigned char)((v[i] >> 24) & 0xff);
	}

	s_tlkdbg_uart_acct_tick = tlkdbg_port_tick() | 1;
	(void)tlkdbg_log_dynamic_write("TXC", d, (int)sizeof(d));

#if defined(TLKDBG_LOG_USE_HASH_LABELS)
	/* Heartbeat so a capture that starts after boot can still validate the
	 * table. Type 6 is metadata and does not pollute the .log. */
	(void)tlkdbg_log_table_info_write(TLKDBG_LABEL_HASH_SEED,
					  TLKDBG_LABEL_TABLE_HASH);
#endif
}

void tlkdbg_tr_uart_init(void)
{
	if (s_tlkdbg_uart_ready) {
		return;
	}

	tlkdbg_uart_hw_init();

	/* Poll-only: no UART interrupt mask, no PLIC line, no DMA interrupt. */
	uart_set_irq_mask(TLKDBG_UART_PORT, (uart_irq_mask_e)0);

	s_tlkdbg_uart_acct_tick = 0;
	s_tlkdbg_uart_last_tick = tlkdbg_port_tick() | 1;
	s_tlkdbg_uart_ready = 1;
}

unsigned int tlkdbg_tr_uart_drop_count(void)
{
	return s_tlkdbg_uart_drop_cnt;
}

static int tlkdbg_tr_uart_log_init(void)
{
	tlkdbg_tr_uart_init();
	return 0;
}

static int tlkdbg_tr_uart_log_write(const unsigned char *p, unsigned short len)
{
	return (tlkdbg_uart_emit(TLKDBG_CH_LOG, p, len) < 0) ? -1 : (int)len;
}

static int tlkdbg_tr_uart_log_busy(void)
{
	return (s_tlkdbg_uart_wptr - s_tlkdbg_uart_rptr) >
		       (TLKDBG_UART_RING_SIZE - TLKDBG_UART_DMA_MAX)
		       ? 1
		       : 0;
}

static void tlkdbg_tr_uart_log_flush(void)
{
	tlkdbg_uart_vcd_flush();
	tlkdbg_uart_kick_dma();
}

static void tlkdbg_tr_uart_log_reset(void)
{
	s_tlkdbg_uart_wptr = 0;
	s_tlkdbg_uart_rptr = 0;
	s_tlkdbg_uart_vcd_len = 0;
}

const tlkdbg_transport_t tlkdbg_tr_uart_log = {
	.name = "uart_raw",
	.mtu = TLKDBG_UART_FRAME_MTU,
	.init = tlkdbg_tr_uart_log_init,
	.write = tlkdbg_tr_uart_log_write,
	.busy = tlkdbg_tr_uart_log_busy,
	.flush = tlkdbg_tr_uart_log_flush,
	.reset = tlkdbg_tr_uart_log_reset,
};
