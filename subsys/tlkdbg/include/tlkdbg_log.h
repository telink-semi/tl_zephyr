/*
 * Copyright (c) 2026 Telink Semiconductor
 * SPDX-License-Identifier: Apache-2.0
 *
 * LOG channel encoder plus a run-time transport registry.
 */
#ifndef TLKDBG_LOG_H_
#define TLKDBG_LOG_H_

#include "tlkdbg_cfg.h"
#include "tlkdbg_transport.h"

#define TLKDBG_LOG_DATA_MAX     255
#define TLKDBG_LOG_PAYLOAD_MAX  (5 + TLKDBG_LOG_DATA_MAX)

/* Record types live in the low 3 bits of the tick field. 1 and 2 stay
 * reserved: their first byte would collide with the USB HCI status bytes. */
#define TLKDBG_LOG_TYPE_HASH_DATA  4
#define TLKDBG_LOG_TYPE_DYNAMIC    5
#define TLKDBG_LOG_TYPE_TABLE_INFO 6

#define TLKDBG_LOG_HASH_PAYLOAD_MAX  (6 + TLKDBG_LOG_DATA_MAX)
#define TLKDBG_LOG_DYNAMIC_LABEL_MAX 64

/* Build-time hash labels (type 4). Enabled by the build system when
 * CONFIG_TLKDBG_LABEL_TABLE is set; tlkdbg_label_hashes.h is generated into
 * the build tree. */
#if defined(TLKDBG_LOG_USE_HASH_LABELS)
#include "tlkdbg_label_hashes.h"

#define TLKDBG_LOG_CAT_(a, b) a##b
#define TLKDBG_LOG_CAT(a, b)  TLKDBG_LOG_CAT_(a, b)
#define TLKDBG_LOG_LABEL_HASH(tag) TLKDBG_LOG_CAT(TLKDBG_LABEL_HASH_, tag)

/* LOG_STR_DATA(tag, "label text", data, n): the literal is only scanned by
 * the generator; the compiled code references the 2-byte hash constant. */
#define LOG_STR_DATA(tag, str, data, n)                                        \
	do {                                                                    \
		unsigned char _tlkdbg_hash_payload[TLKDBG_LOG_HASH_PAYLOAD_MAX]; \
		int _tlkdbg_hash_len = tlkdbg_log_hash_encode(                   \
			TLKDBG_LOG_LABEL_HASH(tag),                              \
			(const unsigned char *)(data), (n),                      \
			_tlkdbg_hash_payload, (int)sizeof(_tlkdbg_hash_payload));\
		if (_tlkdbg_hash_len > 0) {                                      \
			(void)tlkdbg_log_write(_tlkdbg_hash_payload,              \
					       (unsigned short)_tlkdbg_hash_len);\
		}                                                               \
	} while (0)
#endif /* TLKDBG_LOG_USE_HASH_LABELS */

/* Dynamic label: the string travels in the record; no table is involved. */
#define LOG_STR_DYNAMIC(label, data, n) \
	(void)tlkdbg_log_dynamic_write((label), (const unsigned char *)(data), (n))

int tlkdbg_log_register(const tlkdbg_transport_t *tr);
void tlkdbg_log_unregister_all(void);
void tlkdbg_log_init(void);
int tlkdbg_log_write(const unsigned char *payload, unsigned short len);
int tlkdbg_log_busy(void);

int tlkdbg_log_hash_encode(unsigned short hash, const unsigned char *data, int n,
			   unsigned char *out, int outsz);
int tlkdbg_log_hash_write(unsigned short hash, const unsigned char *data, int n);

int tlkdbg_log_dynamic_encode(const char *label, const unsigned char *data, int n,
			      unsigned char *out, int outsz);
int tlkdbg_log_dynamic_write(const char *label, const unsigned char *data, int n);

int tlkdbg_log_table_info_write(unsigned short seed, unsigned int table_hash);

#endif /* TLKDBG_LOG_H_ */
