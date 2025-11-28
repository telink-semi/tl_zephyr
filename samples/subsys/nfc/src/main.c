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

#include <zephyr/kernel.h>
#include <zephyr/sys/reboot.h>

#include <zephyr/nfc/nfc_tag.h>
#include <zephyr/nfc/ndef/msg.h>
#include <zephyr/nfc/ndef/text_rec.h>

#if defined(CONFIG_ST25DVXXKC)
#define NFC_DEV st25dvxxkc
#define DEV_PTR DEVICE_DT_GET(DT_NODELABEL(NFC_DEV))
#endif

#define MAX_REC_COUNT     3
#define NDEF_MSG_BUF_SIZE 128

/* Text message in English with its language code. */
static const uint8_t en_payload[] = {'H', 'e', 'l', 'l', 'o', ' ', 'W', 'o', 'r', 'l', 'd', '!'};
static const uint8_t en_code[] = {'e', 'n'};

/* Text message in Norwegian with its language code. */
static const uint8_t no_payload[] = {'H', 'a', 'l', 'l', 'o', ' ', 'V',
				     'e', 'r', 'd', 'e', 'n', '!'};
static const uint8_t no_code[] = {'N', 'O'};

/* Text message in Polish with its language code. */
static const uint8_t pl_payload[] = {'V', 'i', 't', 'a', 'j', ' ', 0xc5u, 0x9au,
				     'w', 'i', 'e', 'c', 'i', 'e', '!'};
static const uint8_t pl_code[] = {'P', 'L'};

/* Buffer used to hold an NFC NDEF message. */
static uint8_t ndef_msg_buf[NDEF_MSG_BUF_SIZE];

static void nfc_callback(const struct device *dev, enum nfc_tag_event event, const uint8_t *data,
			 size_t data_len)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(data);
	ARG_UNUSED(data_len);

	printk("NFC-CALLBACK EVENT: %d\n", event);
}

/**
 * @brief Function for encoding the NDEF text message.
 */
static int welcome_msg_encode(uint8_t *buffer, uint32_t *len)
{
	int err;

	/* Create NFC NDEF text record description in English */
	NFC_NDEF_TEXT_RECORD_DESC_DEF(nfc_en_text_rec, UTF_8, en_code, sizeof(en_code), en_payload,
				      sizeof(en_payload));

	/* Create NFC NDEF text record description in Norwegian */
	NFC_NDEF_TEXT_RECORD_DESC_DEF(nfc_no_text_rec, UTF_8, no_code, sizeof(no_code), no_payload,
				      sizeof(no_payload));

	/* Create NFC NDEF text record description in Polish */
	NFC_NDEF_TEXT_RECORD_DESC_DEF(nfc_pl_text_rec, UTF_8, pl_code, sizeof(pl_code), pl_payload,
				      sizeof(pl_payload));

	/* Create NFC NDEF message description, capacity - MAX_REC_COUNT
	 * records
	 */
	NFC_NDEF_MSG_DEF(nfc_text_msg, MAX_REC_COUNT);

	/* Add text records to NDEF text message */
	err = nfc_ndef_msg_record_add(&NFC_NDEF_MSG(nfc_text_msg),
				      &NFC_NDEF_TEXT_RECORD_DESC(nfc_en_text_rec));
	if (err < 0) {
		printk("Cannot add first record!\n");
		return err;
	}
	err = nfc_ndef_msg_record_add(&NFC_NDEF_MSG(nfc_text_msg),
				      &NFC_NDEF_TEXT_RECORD_DESC(nfc_no_text_rec));
	if (err < 0) {
		printk("Cannot add second record!\n");
		return err;
	}
	err = nfc_ndef_msg_record_add(&NFC_NDEF_MSG(nfc_text_msg),
				      &NFC_NDEF_TEXT_RECORD_DESC(nfc_pl_text_rec));
	if (err < 0) {
		printk("Cannot add third record!\n");
		return err;
	}

	err = nfc_ndef_msg_encode(&NFC_NDEF_MSG(nfc_text_msg), buffer, len);
	if (err < 0) {
		printk("Cannot encode message!\n");
	}

	return err;
}

int main(void)
{
	int rv;
	uint32_t len = sizeof(ndef_msg_buf);

	printk("Starting NFC Text Record example\n");

	const struct device *dev = DEV_PTR;

	if (dev == NULL) {
		printk("Could not get %s device\n", STRINGIFY(NFC_DEV));
		return -ENODEV;
	}
	printk("Device = ok: %s\n", STRINGIFY(NFC_DEV));


	if (!device_is_ready(dev)) {
		int ret = device_init(dev);

		if (ret != 0) {
			printk("Failed to initialize NFC device: %d", ret);
			return -1;
		}
	}

	/* Set up NFC driver*/
	rv = nfc_tag_init(dev, nfc_callback);
	if (rv != 0) {
		printk("Cannot setup NFC subsys!\n");
	}

	/* Set up Tag mode */
	if (rv == 0) {
		rv = nfc_tag_set_type(dev, NFC_TAG_TYPE_T5T);
		if (rv != 0) {
			printk("Cannot setup NFC Tag mode! (%d)\n", rv);
		}
	}

	/* Encode welcome message */
	if (rv == 0) {
		rv = welcome_msg_encode(ndef_msg_buf, &len);
		if (rv != 0) {
			printk("Cannot encode message! (%d)\n", rv);
		}
	}

	/* Set created message as the NFC payload */
	if (rv == 0) {
		rv = nfc_tag_set_ndef(dev, ndef_msg_buf, len);
		if (rv != 0) {
			printk("Cannot set payload! (%d)\n", rv);
		}
	}

	/* Start sensing NFC field */
	if (rv == 0) {
		rv = nfc_tag_start(dev);
		if (rv != 0) {
			printk("Cannot start emulation! (%d)\n", rv);
		}
	}

	printk("NFC configuration done (%d)\n", rv);

	if (rv != 0) {
#if CONFIG_REBOOT
		/* sys_reboot(SYS_REBOOT_COLD); */
#endif /* CONFIG_REBOOT */
		return -EIO;
	} else {
		return 0;
	}
}
