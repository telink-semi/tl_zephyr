/*
 * Copyright (c) 2026 Telink Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Zephyr ST25DVXXKC mailbox driver demonstration use android ST25 application to interact
 */

#include <st25dv.h>

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(main, LOG_LEVEL_INF);

struct application_data {
	struct st25dv_data *nfc_dev;
	struct k_sem *run_sem;
};

static const char *st25dv_mode_str(struct st25dv_data *data)
{
	const char *result = "unavailable";
	int mode = st25dv_mode_get(data);

	if (mode >= ST25DV_MODE_OFF && mode <= ST25DV_MODE_MAILBOX) {
		static const char * const mode_str[] = {
			"switched off",
			"nfc tag",
			"mailbox",
		};
		result = mode_str[mode];
	}

	return result;
}

static void nfc_dev_on_event(enum st25dv_event event, void *ctx, uint8_t *buf, size_t buf_len)
{
	struct application_data *app_data = (struct application_data *)ctx;

	switch (event) {
	case ST25DV_EVENT_FIELD_ON:
		LOG_INF("nfc device field on");
		break;
	case ST25DV_EVENT_FIELD_OFF:
		LOG_INF("nfc device field off");
		break;
	case ST25DV_EVENT_DATA_RECEIVED:
		LOG_HEXDUMP_INF(buf, buf_len, "nfc device rx");
		if (buf_len == 1 && buf[0] == 0x00) {
			LOG_HEXDUMP_INF(buf, buf_len, "application exit");
			k_sem_give(app_data->run_sem);
		} else if (buf_len == 1 && buf[0] == 0x01) {
			LOG_HEXDUMP_INF(buf, buf_len, "nfc device write tag");

			const uint8_t tag_data[] = {0xd1, 0x01, 0x0f, 0x54, 0x02, 0x65, 0x6e,
						    0x48, 0x65, 0x6c, 0x6c, 0x6f, 0x20, 0x57,
						    0x6f, 0x72, 0x6c, 0x64, 0x21};
			int result =
				st25dv_tag_type5_add(app_data->nfc_dev, tag_data, sizeof(tag_data));

			if (result) {
				LOG_ERR("nfc device tag data add failed %d", result);
			} else {
				LOG_INF("nfc device tag data set");
			}
		} else if (buf_len == 1 && buf[0] == 0x02) {
			LOG_HEXDUMP_INF(buf, buf_len, "nfc device erase tag");

			int result = st25dv_tag_type5_rm(app_data->nfc_dev);

			if (result) {
				LOG_ERR("nfc device tag remove failed %d", result);
			} else {
				LOG_INF("nfc device tag data erased");
			}
		} else if (buf_len == 1 && buf[0] == 0x03) {
			LOG_HEXDUMP_INF(buf, buf_len, "nfc device switch to tag mode");

			int result = st25dv_mode_set(app_data->nfc_dev, ST25DV_MODE_TAG);

			if (result) {
				LOG_ERR("nfc device switch to tag failed %d", result);
			} else {
				LOG_INF("nfc device mode %s", st25dv_mode_str(app_data->nfc_dev));
			}
		} else if (buf_len == 1 && buf[0] == 0x04) {
			LOG_HEXDUMP_INF(buf, buf_len, "nfc device switch off");

			int result = st25dv_mode_set(app_data->nfc_dev, ST25DV_MODE_OFF);

			if (result) {
				LOG_ERR("nfc device switch off failed %d", result);
			} else {
				LOG_INF("nfc device mode %s", st25dv_mode_str(app_data->nfc_dev));
			}
		} else {
			int result = st25dv_mailbox_send(app_data->nfc_dev, buf, buf_len);

			if (result) {
				LOG_ERR("nfc device mailbox send failed %d", result);
			} else {
				LOG_INF("nfc device rx data written back into mailbox");
			}
		}
		break;
	case ST25DV_EVENT_DATA_SENT:
		LOG_INF("nfc device tx");
		break;
	default:
		LOG_WRN("nfc device unknown event %u", event);
	}
}

int main(void)
{
	ST25DV_DATA_DEFINE(app_nfc_dev, st25dv_matter);
	static K_SEM_DEFINE(app_run_sem, 0, 1);
	struct application_data data = {.nfc_dev = &app_nfc_dev, .run_sem = &app_run_sem};
	int result;

	LOG_INF("application started");
	result = st25dv_init(&app_nfc_dev);
	if (result) {
		LOG_ERR("nfc device init failed %d", result);
		return result;
	}
	result = st25dv_mode_set(&app_nfc_dev, ST25DV_MODE_MAILBOX);
	if (result) {
		LOG_ERR("nfc device set mode failed %d", result);
		return result;
	}
	st25dv_set_callback(&app_nfc_dev, nfc_dev_on_event, &data);
	LOG_INF("nfc device mode %s", st25dv_mode_str(&app_nfc_dev));
	LOG_INF("nfc device mailbox data loopback...\n"
		"the next one byte data is reserved for commands\n"
		"0x00 - finish the application (no future commands possible)\n"
		"0x01 - write tag eeprom\n"
		"0x02 - erase tag eeprom\n"
		"0x03 - switch to tag mode (no future commands possible)\n"
		"0x04 - switch off (no future commands possible)\n"
		"commands are not sent back\n");
	(void)k_sem_take(&app_run_sem, K_FOREVER);
	result = st25dv_deinit(&app_nfc_dev);
	if (result) {
		LOG_ERR("nfc device deinit failed %d", result);
	}
	LOG_INF("application finished");

	return result;
}
