/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <errno.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/logging/log.h>
#include <zephyr/kernel.h>

#include "hci_uart_transport.h"

LOG_MODULE_REGISTER(hci_uart, CONFIG_DTM_HCI_UART_TRANSPORT_LOG_LEVEL);

/* HCI H4 packet type indicators */
#define HCI_H4_CMD  0x01
#define HCI_H4_ACL  0x02
#define HCI_H4_SCO  0x03
#define HCI_H4_EVT  0x04
#define HCI_H4_ISO  0x05

/* HCI header sizes (not including H4 type byte) */
#define HCI_CMD_HDR_SIZE  3  /* opcode(2) + param_len(1) */
#define HCI_ACL_HDR_SIZE  4  /* handle(2) + data_len(2) */
#define HCI_SCO_HDR_SIZE  3  /* handle(2) + data_len(1) */
#define HCI_EVT_HDR_SIZE  2  /* event_code(1) + param_len(1) */
#define HCI_ISO_HDR_SIZE  4  /* handle(2) + data_len(2) */

#define DTM_UART DT_CHOSEN(uart1)

#ifndef CONFIG_DTM_HCI_BAUDRATE
#define CONFIG_DTM_HCI_BAUDRATE 115200
#endif

/* Poll cycle in microseconds based on baudrate (10 bits per byte) */
#define UART_POLL_CYCLE_US ((uint32_t)(10U * 1000000U / CONFIG_DTM_HCI_BAUDRATE))

static const struct device *hci_uart_dev = DEVICE_DT_GET_OR_NULL(DTM_UART);

/**
 * @brief Get the expected payload length from an HCI header.
 *
 * @param h4_type  HCI H4 packet type.
 * @param header   Pointer to the HCI header bytes.
 * @param hdr_len  Length of the header that has been read.
 * @param[out] payload_len  Expected payload length beyond the header.
 *
 * @return 0 on success, -EINVAL if unknown H4 type or insufficient header.
 */
static int hci_get_payload_len(uint8_t h4_type, const uint8_t *header,
			       size_t hdr_len, size_t *payload_len)
{
	switch (h4_type) {
	case HCI_H4_CMD:
		if (hdr_len < HCI_CMD_HDR_SIZE) {
			return -EINVAL;
		}
		*payload_len = header[2]; /* param_len */
		return 0;

	case HCI_H4_ACL:
		if (hdr_len < HCI_ACL_HDR_SIZE) {
			return -EINVAL;
		}
		*payload_len = header[2] | ((uint16_t)header[3] << 8);
		return 0;

	case HCI_H4_SCO:
		if (hdr_len < HCI_SCO_HDR_SIZE) {
			return -EINVAL;
		}
		*payload_len = header[2];
		return 0;

	case HCI_H4_EVT:
		if (hdr_len < HCI_EVT_HDR_SIZE) {
			return -EINVAL;
		}
		*payload_len = header[1]; /* param_len */
		return 0;

	case HCI_H4_ISO:
		if (hdr_len < HCI_ISO_HDR_SIZE) {
			return -EINVAL;
		}
		/* ISO: last 2 bytes of header are data_len (little-endian) */
		*payload_len = header[hdr_len - 2] |
			       ((uint16_t)header[hdr_len - 1] << 8);
		return 0;

	default:
		return -EINVAL;
	}
}

/**
 * @brief Get the HCI header size for a given H4 packet type.
 */
static size_t hci_hdr_size(uint8_t h4_type)
{
	switch (h4_type) {
	case HCI_H4_CMD: return HCI_CMD_HDR_SIZE;
	case HCI_H4_ACL: return HCI_ACL_HDR_SIZE;
	case HCI_H4_SCO: return HCI_SCO_HDR_SIZE;
	case HCI_H4_EVT: return HCI_EVT_HDR_SIZE;
	case HCI_H4_ISO: return HCI_ISO_HDR_SIZE;
	default:         return 0;
	}
}

int hci_uart_init(void)
{
	if (!device_is_ready(hci_uart_dev)) {
		LOG_ERR("UART device not ready");
		return -EIO;
	}

	struct uart_config cfg;
	int ret = uart_config_get(hci_uart_dev, &cfg);

	if (ret != 0) {
		return ret;
	}

	cfg.baudrate = CONFIG_DTM_HCI_BAUDRATE;
	ret = uart_configure(hci_uart_dev, &cfg);
	if (ret != 0) {
		return ret;
	}

	LOG_INF("HCI UART initialized at %d baud", CONFIG_DTM_HCI_BAUDRATE);
	return 0;
}

int hci_uart_read(uint8_t *buf, size_t *len)
{
	if (buf == NULL || len == NULL) {
		return -EINVAL;
	}

	uint8_t rx_byte;
	int err;
	size_t idx = 0;
	size_t expected_total = 0; /* 0 means waiting for H4 type byte */
	size_t hdr_len;
	uint8_t h4_type;

	for (;;) {
		k_sleep(K_USEC(UART_POLL_CYCLE_US));

		err = uart_poll_in(hci_uart_dev, &rx_byte);
		if (err) {
			if (err != -1) {
				LOG_ERR("UART polling error: %d", err);
			}
			continue;
		}

		if (idx == 0) {
			/* First byte: H4 packet type */
			h4_type = rx_byte;
			hdr_len = hci_hdr_size(h4_type);
			if (hdr_len == 0) {
				LOG_ERR("Unknown H4 packet type 0x%02X, discarding", h4_type);
				/* Reset and wait for next valid type byte */
				continue;
			}
			buf[idx++] = rx_byte;
			/* We still need the header to determine total length */
			expected_total = 0;
			continue;
		}

		buf[idx++] = rx_byte;

		/* After reading the H4 type, we need header + payload.
		 * Once we have the full header, compute the total expected size.
		 */
		if (expected_total == 0 && idx >= 1 + hdr_len) {
			size_t payload_len;

			err = hci_get_payload_len(h4_type, buf + 1, hdr_len, &payload_len);
			if (err) {
				LOG_ERR("Failed to get payload length for type 0x%02X", h4_type);
				idx = 0;
				continue;
			}

			expected_total = 1 + hdr_len + payload_len;

			if (expected_total > HCI_UART_MAX_PKT_SIZE) {
				LOG_ERR("Packet too large: %zu bytes", expected_total);
				idx = 0;
				expected_total = 0;
				continue;
			}
		}

		/* Check if we have received the complete packet */
		if (expected_total > 0 && idx >= expected_total) {
			*len = idx;
			LOG_DBG("Received HCI packet: type=0x%02X, len=%zu", h4_type, idx);
			return 0;
		}
	}
}

void hci_uart_write(const uint8_t *buf, size_t len)
{
	if (buf == NULL || len == 0) {
		return;
	}

	LOG_DBG("Sending HCI packet: type=0x%02X, len=%zu", buf[0], len);

	for (size_t i = 0; i < len; i++) {
		uart_poll_out(hci_uart_dev, buf[i]);
	}
}
