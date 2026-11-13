/*
 * Copyright (c) 2026 Telink Semiconductor
 * SPDX-License-Identifier: Apache-2.0
 *
 * LOG channel encoder and transport registry.
 */
#include <string.h>
#include <zephyr/kernel.h>

#include "tlkdbg_log.h"
#include "tlkdbg_tr.h"
#include "tlkdbg_port_tl323x.h"

#define TLKDBG_LOG_MAX_TRANSPORTS 4

static const tlkdbg_transport_t *s_tlkdbg_log_tr[TLKDBG_LOG_MAX_TRANSPORTS];
static unsigned char s_tlkdbg_log_tr_num;

int tlkdbg_log_register(const tlkdbg_transport_t *tr)
{
	if (tr == NULL || s_tlkdbg_log_tr_num >= TLKDBG_LOG_MAX_TRANSPORTS) {
		return -1;
	}

	s_tlkdbg_log_tr[s_tlkdbg_log_tr_num++] = tr;
	return 0;
}

void tlkdbg_log_unregister_all(void)
{
	s_tlkdbg_log_tr_num = 0;
}

void tlkdbg_log_init(void)
{
	for (unsigned char i = 0; i < s_tlkdbg_log_tr_num; i++) {
		if (s_tlkdbg_log_tr[i]->init != NULL) {
			(void)s_tlkdbg_log_tr[i]->init();
		}
	}
}

int tlkdbg_log_write(const unsigned char *payload, unsigned short len)
{
	int sent = 0;

	for (unsigned char i = 0; i < s_tlkdbg_log_tr_num; i++) {
		const tlkdbg_transport_t *tr = s_tlkdbg_log_tr[i];

		if (tr->write == NULL) {
			continue;
		}
		if (tr->write(payload, len) >= 0) {
			sent++;
		} else {
			tlkdbg_stat_drop_log++;
		}
	}
	return sent;
}

int tlkdbg_log_busy(void)
{
	if (s_tlkdbg_log_tr_num == 0) {
		return 1;
	}
	if (s_tlkdbg_log_tr_num == 1) {
		const tlkdbg_transport_t *tr = s_tlkdbg_log_tr[0];

		return (tr->busy != NULL && tr->busy() != 0);
	}
	for (unsigned char i = 0; i < s_tlkdbg_log_tr_num; i++) {
		const tlkdbg_transport_t *tr = s_tlkdbg_log_tr[i];

		if (tr->busy == NULL || tr->busy() == 0) {
			return 0;
		}
	}
	return 1;
}

int tlkdbg_log_hash_encode(unsigned short hash, const unsigned char *data, int n,
			   unsigned char *out, int outsz)
{
	if (out == NULL || n < 0 || n > TLKDBG_LOG_DATA_MAX) {
		return 0;
	}
	int len = 6 + n;

	if (len > outsz) {
		return 0;
	}

	unsigned int tick = tlkdbg_port_tick();

	out[0] = (unsigned char)((tick & 0xf8) | TLKDBG_LOG_TYPE_HASH_DATA);
	out[1] = (unsigned char)((tick >> 8) & 0xff);
	out[2] = (unsigned char)((tick >> 16) & 0xff);
	out[3] = (unsigned char)(hash & 0xff);
	out[4] = (unsigned char)(hash >> 8);
	out[5] = (unsigned char)(n & 0xff);
	for (int i = 0; i < n; i++) {
		out[6 + i] = data[i];
	}
	return len;
}

int tlkdbg_log_hash_write(unsigned short hash, const unsigned char *data, int n)
{
	unsigned char payload[TLKDBG_LOG_HASH_PAYLOAD_MAX];
	int len = tlkdbg_log_hash_encode(hash, data, n, payload, (int)sizeof(payload));

	if (len <= 0) {
		return 0;
	}
	return tlkdbg_log_write(payload, (unsigned short)len);
}

int tlkdbg_log_dynamic_encode(const char *label, const unsigned char *data, int n,
			      unsigned char *out, int outsz)
{
	if (out == NULL || label == NULL || n < 0) {
		return 0;
	}

	int ll = (int)strlen(label);

	if (ll > TLKDBG_LOG_DYNAMIC_LABEL_MAX) {
		ll = TLKDBG_LOG_DYNAMIC_LABEL_MAX;
	}
	if (ll + n > TLKDBG_LOG_DATA_MAX) {
		n = TLKDBG_LOG_DATA_MAX - ll;
		if (n < 0) {
			n = 0;
		}
	}
	int len = 5 + ll + n;

	if (len > outsz) {
		return 0;
	}

	unsigned int tick = tlkdbg_port_tick();

	out[0] = (unsigned char)((tick & 0xf8) | TLKDBG_LOG_TYPE_DYNAMIC);
	out[1] = (unsigned char)((tick >> 8) & 0xff);
	out[2] = (unsigned char)((tick >> 16) & 0xff);
	out[3] = (unsigned char)ll;
	for (int i = 0; i < ll; i++) {
		out[4 + i] = (unsigned char)label[i];
	}
	out[4 + ll] = (unsigned char)n;
	for (int i = 0; i < n; i++) {
		out[5 + ll + i] = data[i];
	}
	return len;
}

int tlkdbg_log_dynamic_write(const char *label, const unsigned char *data, int n)
{
	unsigned char payload[TLKDBG_LOG_PAYLOAD_MAX];
	int len = tlkdbg_log_dynamic_encode(label, data, n, payload, (int)sizeof(payload));

	if (len <= 0) {
		return 0;
	}
	return tlkdbg_log_write(payload, (unsigned short)len);
}

int tlkdbg_log_table_info_write(unsigned short seed, unsigned int table_hash)
{
	unsigned char payload[8];
	unsigned int tick = tlkdbg_port_tick();

	payload[0] = (unsigned char)((tick & 0xf8) | TLKDBG_LOG_TYPE_TABLE_INFO);
	payload[1] = (unsigned char)((tick >> 8) & 0xff);
	payload[2] = (unsigned char)((tick >> 16) & 0xff);
	payload[3] = (unsigned char)(seed & 0xff);
	payload[4] = (unsigned char)(seed >> 8);
	payload[5] = (unsigned char)(table_hash & 0xff);
	payload[6] = (unsigned char)((table_hash >> 8) & 0xff);
	payload[7] = (unsigned char)((table_hash >> 16) & 0xff);
	return tlkdbg_log_write(payload, (unsigned short)sizeof(payload));
}
