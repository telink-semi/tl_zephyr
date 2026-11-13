/*
 * Copyright (c) 2026 Telink Semiconductor
 * SPDX-License-Identifier: Apache-2.0
 *
 * Compile-time configuration for the tlkdbg UART stream.
 */
#ifndef TLKDBG_CFG_H_
#define TLKDBG_CFG_H_

#include <zephyr/kernel.h>

/* Transport backend identifiers (only UART is built here). */
#define TLKDBG_TR_NULL 0
#define TLKDBG_TR_UART 2

#define TLKDBG_VCD_TRANSPORT TLKDBG_TR_UART
#define TLKDBG_LOG_ENABLE    1

/* UART wire parameters. Keep the frame layout identical to the reference
 * implementation: [A5 5A][ch][len:2 LE][payload][crc16 LE], CRC-16/CCITT-FALSE
 * over [ch][len_lo][len_hi][payload]. */
#define TLKDBG_FRAME_SYNC0      0xA5
#define TLKDBG_FRAME_SYNC1      0x5A
#define TLKDBG_CH_LOG           0x01
#define TLKDBG_CH_VCD           0x02
#define TLKDBG_UART_FRAME_MTU   256
#define TLKDBG_UART_HDR_LEN     5
#define TLKDBG_FRAME_CRC_LEN    2

/* The VCD/LOG tick is the chip's 24 MHz stimer. The PC must be configured with
 * mhz = 24; this is unrelated to CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC. */
#define TLKDBG_TICKS_PER_MS 24000u

#define TLKDBG_UART_BAUDRATE   CONFIG_TLKDBG_UART_BAUD
#define TLKDBG_UART_RING_SIZE  CONFIG_TLKDBG_UART_RING_SIZE
#define TLKDBG_UART_DMA_MAX    CONFIG_TLKDBG_UART_DMA_MAX
#define TLKDBG_UART_VCD_BATCH  CONFIG_TLKDBG_UART_VCD_BATCH

/* Flush a partially filled VCD batch after this long (us). */
#define TLKDBG_UART_FLUSH_US 10000u

/* Maximum gap between two VCD timestamp words (ms). The 24-bit tick wraps
 * every 2^24 / 24 MHz ~= 699 ms; inject a timestamp before that. */
#define TLKDBG_UART_TICK_MS 300u

/* Absolute-time anchor (reserved b24 SID 0, value = device ms + 1). */
#define TLKDBG_UART_MS_SID    0u
#define TLKDBG_UART_ANCHOR_MS 100u

/* Cumulative accounting record ("TXC") cadence. */
#define TLKDBG_UART_ACCOUNT_MS 500u

/* Minimum bytes accumulated before starting a DMA burst. */
#define TLKDBG_UART_POLL_MIN 64u

/* Drain safety net. */
#define TLKDBG_UART_DRAIN_TIMEOUT_MS 100u

static inline unsigned short tlkdbg_crc16_update(unsigned short crc,
						 const unsigned char *p, unsigned int n)
{
	static const unsigned short tbl[16] = {
		0x0000, 0x1021, 0x2042, 0x3063, 0x4084, 0x50A5, 0x60C6, 0x70E7,
		0x8108, 0x9129, 0xA14A, 0xB16B, 0xC18C, 0xD1AD, 0xE1CE, 0xF1EF,
	};
	while (n--) {
		unsigned char b = *p++;
		crc = (unsigned short)((crc << 4) ^ tbl[((crc >> 12) ^ (b >> 4)) & 0x0f]);
		crc = (unsigned short)((crc << 4) ^ tbl[((crc >> 12) ^ (b & 0x0f)) & 0x0f]);
	}
	return crc;
}

static inline unsigned short tlkdbg_crc16(const unsigned char *p, unsigned int n)
{
	return tlkdbg_crc16_update(0xffff, p, n);
}

#endif /* TLKDBG_CFG_H_ */
