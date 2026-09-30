/*
 * Copyright (c) 2023 Sendrato
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Modifications:
 * Copyright (c) 2025 Telink Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/nfc/nfc_tag.h>

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(nfc_tag, LOG_LEVEL_INF);

static void nfc_callback(const struct device *dev, enum nfc_tag_event event, const uint8_t *data,
			 size_t data_len)
{
	LOG_INF("%s NFC event %u", dev->name, event);
	LOG_HEXDUMP_INF(data, data_len, "NFC event data");
}

int main(void)
{
	int result;

	do {
		const struct device *dev = DEVICE_DT_GET(DT_NODELABEL(st25dvxxkc));

		LOG_INF("NFC tag example");
		if (!device_is_ready(dev)) {

			LOG_ERR("NFC device not ready");
			result = -ENODEV;
			break;
		}
		result = nfc_tag_init(dev, nfc_callback);
		if (result) {
			LOG_ERR("Can't init tag");
			break;
		}
		result = nfc_tag_set_type(dev, NFC_TAG_TYPE_T5T);
		if (result) {
			LOG_ERR("Can't set tag type");
			break;
		}

		uint8_t ndef[] = {0xd1, 0x01, 0x0f, 0x54, 0x02, 0x65, 0x6e, 0x48, 0x65, 0x6c,
				  0x6c, 0x6f, 0x20, 0x57, 0x6f, 0x72, 0x6c, 0x64, 0x21};

		result = nfc_tag_set_ndef(dev, ndef, sizeof(ndef));
		if (result) {
			LOG_ERR("Can't set tag data");
			break;
		}
		result = nfc_tag_start(dev);
		if (result) {
			LOG_ERR("Can't start tag");
			break;
		}
		LOG_INF("NFC tag configured");
	} while (0);

	return result;
}
