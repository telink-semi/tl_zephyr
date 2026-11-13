/*
 * Copyright (c) 2026 Telink Semiconductor
 * SPDX-License-Identifier: Apache-2.0
 *
 * One transport backend. write() is byte oriented so the same backend serves
 * VCD (4-byte words) and LOG (whole records).
 */
#ifndef TLKDBG_TRANSPORT_H_
#define TLKDBG_TRANSPORT_H_

typedef struct {
	const char *name;
	unsigned short mtu;
	int (*init)(void);
	int (*write)(const unsigned char *p, unsigned short len);
	int (*busy)(void);
	void (*flush)(void);
	void (*reset)(void);
} tlkdbg_transport_t;

#endif /* TLKDBG_TRANSPORT_H_ */
