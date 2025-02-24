/*
 * Copyright (c) 2025 Telink Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "spinel_drv.h"

#define LOG_LEVEL CONFIG_IEEE802154_DRIVER_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(spinel_drv);

void spinel_drv_init(struct spinel_drv_data *spinel_drv, uint8_t inst)
{
	spinel_drv->inst = inst;
	spinel_drv->t_id = 1;
}

int spinel_drv_send_cmd(struct spinel_drv_data *spinel_drv,
	spinel_tx_cb tx_cb, const void *ctx, uint32_t cmd, uint32_t prop, const char *fmt, ...)
{
	int ret = 0;
	va_list args;

	va_start(args, fmt);

	do {
		ret = spinel_datatype_pack(tx_cb, ctx,
			SPINEL_DATATYPE_COMMAND_S SPINEL_DATATYPE_UINT8_S,
			SPINEL_HEADER_FLAG | SPINEL_HEADER_IID(spinel_drv->inst) | spinel_drv->t_id,
			cmd, prop);

		if (ret < 0) {
			LOG_ERR("Failed to pack header spinel data (inst = %u)", spinel_drv->inst);
			break;
		}

		if (fmt) {
			int ret_vpack = spinel_datatype_vpack(tx_cb, ctx, fmt, &args);

			if (ret_vpack < 0) {
				ret = ret_vpack;
				LOG_ERR("Failed to pack spinel data (inst = %u)", spinel_drv->inst);
				break;
			}

			ret += ret_vpack;
		}
	} while (0);

	va_end(args);
	return ret;
}

int spinel_drv_send_reset(struct spinel_drv_data *spinel_drv,
	spinel_tx_cb tx_cb, const void *ctx, uint8_t type)
{
	int ret = 0;

	do {
		ret = spinel_datatype_pack(tx_cb, ctx,
			SPINEL_DATATYPE_COMMAND_S SPINEL_DATATYPE_UINT8_S,
			SPINEL_HEADER_FLAG | SPINEL_HEADER_IID(spinel_drv->inst), SPINEL_CMD_RESET, type);

		if (ret < 0) {
			LOG_ERR("Failed to pack spinel data (inst = %u)", spinel_drv->inst);
			break;
		}

	} while(0);

	return ret;
}

bool spinel_drv_check_reset(struct spinel_drv_data *spinel_drv,
	const uint8_t *data, size_t data_size)
{
	/* TODO: parse reset response */
	return true;
}

bool spinel_drv_reception_data(struct spinel_drv_data *spinel_drv,
	const uint8_t *data, size_t data_size)
{
	bool result = false;

	/* TODO: instance (possibly t_id) check */
	if (data_size >= SPINEL_DRV_RECEPTION_DATA_HEADER_LEN) {
		if (data[1] == SPINEL_CMD_PROP_VALUE_IS && data[2] == SPINEL_PROP_STREAM_RAW) {
			result = true;
		}
	}
	return result;
}

int spinel_drv_send_get_ieee_eui64(struct spinel_drv_data *spinel_drv,
	spinel_tx_cb tx_cb, const void *ctx)
{
	int ret = 0;

	do {
		ret = spinel_datatype_pack(tx_cb, ctx, "Cii",
			SPINEL_HEADER_FLAG | SPINEL_HEADER_IID(spinel_drv->inst) | spinel_drv->t_id,
			SPINEL_CMD_PROP_VALUE_GET, SPINEL_PROP_HWADDR);

		if (ret < 0) {
			LOG_ERR("Failed to pack spinel data (inst = %u)", spinel_drv->inst);
			break;
		}

	} while(0);

	return ret;
}

bool spinel_drv_check_get_ieee_eui64(struct spinel_drv_data *spinel_drv,
	const uint8_t *data, size_t data_size, uint8_t ieee_eui64[8])
{
	/* TODO: parse reset response, now dummy data */
	for (size_t i = 0; i < 8; i++) {
		ieee_eui64[i] = i;
	}
	return true;
}
