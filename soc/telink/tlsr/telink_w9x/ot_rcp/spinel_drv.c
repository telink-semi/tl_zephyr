/*
 * Copyright (c) 2025 Telink Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "spinel_drv.h"

#define LOG_LEVEL CONFIG_IEEE802154_DRIVER_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(spinel_drv);

int spinel_drv_send_cmd(uint8_t inst, int8_t t_id, spinel_tx_cb tx_cb, const void *ctx,
	uint32_t cmd, uint32_t prop, const char *fmt, ...)
{
	int ret = 0;
	va_list args;

	va_start(args, fmt);

	do {
		ret = spinel_datatype_pack(tx_cb, ctx,
			SPINEL_DATATYPE_COMMAND_S SPINEL_DATATYPE_UINT8_S,
			SPINEL_HEADER_FLAG | SPINEL_HEADER_IID(inst) | t_id, cmd, prop);

		if (ret < 0) {
			LOG_ERR("Failed to pack header spinel data (inst = %u)", inst);
			break;
		}

		if (fmt) {
			int ret_vpack = spinel_datatype_vpack(tx_cb, ctx, fmt, &args);

			if (ret_vpack < 0) {
				ret = ret_vpack;
				LOG_ERR("Failed to pack spinel data (inst = %u)", inst);
				break;
			}

			ret += ret_vpack;
		}
	} while (0);

	va_end(args);
	return ret;
}

int spinel_drv_send_reset(uint8_t inst, spinel_tx_cb tx_cb, const void *ctx, uint8_t type)
{
	int ret = 0;

	do {
		ret = spinel_datatype_pack(tx_cb, ctx,
			SPINEL_DATATYPE_COMMAND_S SPINEL_DATATYPE_UINT8_S,
			SPINEL_HEADER_FLAG | SPINEL_HEADER_IID(inst), SPINEL_CMD_RESET, type);

		if (ret < 0) {
			LOG_ERR("Failed to pack spinel data (inst = %u)", inst);
			break;
		}

	} while(0);

	return ret;
}
