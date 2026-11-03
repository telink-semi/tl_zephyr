/*
 * Copyright (c) 2026 Telink Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Zephyr M24SR driver demonstration.
 * use next android application on your smartphone
 * https://play.google.com/store/apps/details?id=com.nxp.taginfolite
 */

#include <m24sr.h>
#include <m24sr_ext.h>

#include <zephyr/sys/byteorder.h>
#include <zephyr/kernel.h>

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(main, LOG_LEVEL_INF);

#define MAX_APDU_SIZE 261

static void nfc_event_callback(void *ctx)
{
	do {
		LOG_INF("m24sr gpo callback triggered (smartphone sent data)");

		uint16_t result;
		uint16_t apdu_len;

		result = M24SR_ReadBinary(M24SR_I2C_ADDRESS, 0, sizeof(apdu_len),
					  (uint8_t *)&apdu_len);
		if (result != M24SR_STATUS_SUCCESS) {
			LOG_ERR("m24sr read apdu length failed %x", result);
			break;
		}
		apdu_len = sys_be16_to_cpu(apdu_len);
		if (!apdu_len || apdu_len > MAX_APDU_SIZE - sizeof(apdu_len)) {
			LOG_ERR("m24sr income apdu length failed %u", apdu_len);
			break;
		}

		uint8_t apdu_data[sizeof(apdu_len) + apdu_len];

		result = M24SR_ReadBinary(M24SR_I2C_ADDRESS, sizeof(apdu_len),
					  sizeof(apdu_data) - sizeof(apdu_len),
					  &apdu_data[sizeof(apdu_len)]);
		if (result != M24SR_STATUS_SUCCESS) {
			LOG_ERR("m24sr read apdu data failed %x", result);
			break;
		}
		*(uint16_t *)apdu_data = sys_cpu_to_be16(apdu_len);
		LOG_HEXDUMP_INF(apdu_data, sizeof(apdu_data), "income apdu");

		uint8_t apdu_response[] = {0x90, 0x00};

		result = M24SR_UpdateBinary(M24SR_I2C_ADDRESS, 0, sizeof(apdu_response),
					    apdu_response);
		if (result != M24SR_STATUS_SUCCESS) {
			LOG_ERR("m24sr write apdu data failed %x", result);
			break;
		}
		LOG_INF("m24sr response apdu [90 00] sent successfully");
	} while (0);
}

int main(void)
{
	uint16_t result;

	do {
		LOG_INF("m24sr test app...");
		M24SR_Init(M24SR_I2C_ADDRESS, M24SR_GPO_INTERRUPT);
		result = M24SR_ManageRFGPO(M24SR_I2C_ADDRESS, M24SR_GPO_INTERRUPT);
		if (result != M24SR_STATUS_SUCCESS) {
			LOG_ERR("m24sr setup gpo failed %x", result);
			break;
		}
		M24SR_SetEventHandler(nfc_event_callback, NULL);
		LOG_INF("m24sr nfc system ready. please scan the tag with your android phone...");
		for (;;) {
			k_sleep(K_FOREVER);
		}
	} while (0);
	return result;
}
