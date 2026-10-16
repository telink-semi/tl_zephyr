/*
 * Copyright (c) 2026 Telink Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Zephyr ST25DVXXKC mailbox driver demonstration use android ST25 application to interact
 */

#include "st25dv.h"

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(main, LOG_LEVEL_INF);

#if CONFIG_APPLICATION_TYPE_ST25DV_MAILBOX
static void st25dv_sent(struct st25dv_data *data)
{
	ARG_UNUSED(data);
	LOG_INF("st25dv tx");
}

static void st25dv_received(struct st25dv_data *data, const uint8_t *buffer, size_t len)
{
	LOG_HEXDUMP_INF(buffer, len, "st25dv rx");

	int result = st25dv_mailbox_send(data, buffer, len);

	if (result) {
		LOG_ERR("st25dv mailbox send failed %d", result);
	} else {
		LOG_INF("st25dv rx data written back into mailbox");
	}
}
#endif

int main(void)
{
	static ST25DV_DATA_DEFINE(st25dv, st25dv_matter);
	int result;

#if CONFIG_APPLICATION_TYPE_ST25DV_MAILBOX
	result = st25dv_init(&st25dv);

	if (result) {
		LOG_ERR("st25dv init failed %d", result);
		return result;
	}
	st25dv_set_callbacks(&st25dv, st25dv_sent, st25dv_received);
	LOG_INF("st25dv NFC mailbox data loopback...");
	for (;;) {
		/* dummy main process */
		k_msleep(1000);
	}
	result = st25dv_deinit(&st25dv);
	if (result) {
		LOG_ERR("st25dv deinit failed %d", result);
	}
#endif

#if CONFIG_APPLICATION_TYPE_ST25DV_STATIC_TAG_ADD
	const uint8_t tag_data[] = {0xd1, 0x01, 0x0f, 0x54, 0x02, 0x65, 0x6e, 0x48, 0x65, 0x6c,
				    0x6c, 0x6f, 0x20, 0x57, 0x6f, 0x72, 0x6c, 0x64, 0x21};

	result = st25dv_type5_tag_add(&st25dv, tag_data, sizeof(tag_data));
	if (result) {
		LOG_ERR("st25dv tag add failed %d", result);
	}
#endif

#if CONFIG_APPLICATION_TYPE_ST25DV_STATIC_TAG_RM
	result = st25dv_type5_tag_rm(&st25dv);
	if (result) {
		LOG_ERR("st25dv tag remove failed %d", result);
	}
#endif

	return result;
}
