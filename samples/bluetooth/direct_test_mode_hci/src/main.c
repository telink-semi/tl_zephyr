/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 *
 * DTM HCI Bridge Sample
 *
 * This sample runs on the D25F core of TL322X. It acts as an HCI transport
 * bridge between the host (connected via UART) and the N22 core (BLE controller):
 *
 *   Host UART  --HCI-->  D25F  --shared memory-->  N22 (BLE controller)
 *   Host UART  <--HCI--  D25F  <--shared memory--  N22 (BLE controller)
 *
 * D25F receives HCI H4 packets from UART and forwards them to N22 via
 * shared memory. HCI events/responses from N22 are received via shared
 * memory and forwarded back to the host via UART.
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include "hci_uart_transport.h"
#include "service_d25f.h"
#include "mcc.h"

LOG_MODULE_REGISTER(main, CONFIG_LOG_DEFAULT_LEVEL);

/* Timeout for waiting for N22 HCI response */
#define HCI_RESPONSE_TIMEOUT_MS 5000

/* Poll interval for mcc_d25f_loop() while waiting for response */
#define SHM_POLL_INTERVAL_MS 1

/* Maximum HCI packet size including H4 type byte */
#define HCI_PKT_BUF_SIZE HCI_UART_MAX_PKT_SIZE

/* Semaphore signaled by the shared memory callback when an HCI packet
 * arrives from the N22 core.
 */
static K_SEM_DEFINE(shm_hci_sem, 0, 1);

/* Buffer for HCI packets received from N22 via shared memory */
static uint8_t shm_hci_buf[HCI_PKT_BUF_SIZE];
static volatile size_t shm_hci_len;

/* Shared memory receive callback for TLK_SHM_MSG_HCI messages from N22.
 * Called by mcc_d25f_loop() when the controller thread pops a message
 * from the N22-to-D25F shared memory FIFO.
 *
 * The data format is: [H4 type (1 byte)] [HCI payload ...]
 * This matches the format used by mcc_d25f_hci_send_msg().
 */
static void dtm_shm_hci_recv_cb(uint8_t *pData, uint16_t len)
{
	if (len > HCI_PKT_BUF_SIZE) {
		LOG_ERR("HCI packet too large from N22: %u bytes", len);
		return;
	}

	memcpy(shm_hci_buf, pData, len);
	shm_hci_len = len;
	k_sem_give(&shm_hci_sem);
}

int main(void)
{
	printk("Starting DTM HCI bridge sample\n");

	int err;

	err = hci_uart_init();
	if (err) {
		printk("HCI UART init failed: %d\n", err);
		return err;
	}

	/* Register shared memory callback for TLK_SHM_MSG_HCI messages.
	 * N22 (BLE controller) sends HCI events and command responses
	 * via this message type.
	 */
	mcc_d25f_register_shm_recv_cb(TLK_SHM_MSG_HCI, dtm_shm_hci_recv_cb);

	for (;;) {
		/* Receive HCI packet from host via UART */
		uint8_t hci_pkt[HCI_PKT_BUF_SIZE];
		size_t hci_len = 0;

		printk("Waiting for HCI packet from UART...\n");

		err = hci_uart_read(hci_pkt, &hci_len);
		if (err) {
			printk("HCI UART read error: %d\n", err);
			continue;
		}

		printk("Received HCI packet: type=0x%02X, len=%zu\n",
		       hci_pkt[0], hci_len);

		/* Forward HCI packet to N22 via shared memory.
		 * mcc_d25f_hci_send_msg expects [H4 type][HCI payload] format.
		 */
		shm_fifo_status_e shm_ret = mcc_d25f_hci_send_msg(hci_pkt, hci_len);

		if (shm_ret != SHM_FIFO_SUCCESS) {
			printk("Failed to send HCI to N22: %d\n", shm_ret);
			continue;
		}

		/* Poll for response from N22.
		 * mcc_d25f_loop() pops messages from the N22-to-D25F FIFO
		 * and dispatches them to registered callbacks (including ours).
		 */
		bool got_response = false;
		int64_t deadline = k_uptime_get() + HCI_RESPONSE_TIMEOUT_MS;

		while (k_uptime_get() < deadline) {
			mcc_d25f_loop();

			/* Check if our callback was invoked */
			while (k_sem_take(&shm_hci_sem, K_NO_WAIT) == 0) {
				got_response = true;

				printk("Received HCI response: type=0x%02X, len=%zu\n",
				       shm_hci_buf[0], shm_hci_len);

				/* Forward HCI response to host via UART */
				hci_uart_write(shm_hci_buf, shm_hci_len);
			}

			if (got_response) {
				break;
			}

			k_sleep(K_MSEC(SHM_POLL_INTERVAL_MS));
		}

		if (!got_response) {
			printk("Timeout waiting for N22 HCI response\n");
		}
	}
}
