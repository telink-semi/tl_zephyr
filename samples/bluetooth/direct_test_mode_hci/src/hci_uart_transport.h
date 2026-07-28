/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef HCI_UART_TRANSPORT_H_
#define HCI_UART_TRANSPORT_H_

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file
 * @defgroup dtm_hci_uart_transport DTM HCI UART Transport
 * @{
 * @brief HCI H4 UART transport layer for the DTM HCI bridge sample.
 *
 * This module handles reading and writing complete HCI H4 packets
 * over the UART interface. Each packet on the wire has the format:
 *   [H4 packet type (1 byte)] [HCI payload ...]
 *
 * H4 packet types:
 *   0x01 = HCI Command
 *   0x02 = ACL Data
 *   0x03 = SCO Data
 *   0x04 = HCI Event
 *   0x05 = ISO Data
 */

/** Maximum size of a single HCI H4 packet (type byte + max payload). */
#define HCI_UART_MAX_PKT_SIZE 260

/**
 * @brief Initialize the HCI UART transport.
 *
 * Configures the UART with the baudrate specified by
 * CONFIG_DTM_HCI_BAUDRATE (default 115200).
 *
 * @return 0 on success, negative error code on failure.
 */
int hci_uart_init(void);

/**
 * @brief Read a complete HCI H4 packet from UART (blocking).
 *
 * This function blocks until a complete HCI packet is received.
 * The output buffer contains the H4 type byte followed by the
 * HCI payload.
 *
 * @param[out] buf   Buffer to store the received packet.
 * @param[out] len   Actual length of the received packet (including H4 byte).
 *
 * @return 0 on success, negative error code on failure.
 */
int hci_uart_read(uint8_t *buf, size_t *len);

/**
 * @brief Write an HCI H4 packet to UART.
 *
 * The buffer must start with the H4 type byte followed by the HCI payload.
 *
 * @param[in] buf  Buffer containing the HCI H4 packet to send.
 * @param[in] len  Length of the packet (including H4 byte).
 */
void hci_uart_write(const uint8_t *buf, size_t len);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* HCI_UART_TRANSPORT_H_ */
