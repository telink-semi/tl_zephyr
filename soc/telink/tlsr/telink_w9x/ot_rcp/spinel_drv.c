/*
 * Copyright (c) 2025 Telink Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "spinel_drv.h"

#define LOG_LEVEL CONFIG_IEEE802154_DRIVER_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(spinel_drv);

static int spinel_drv_set_t_id(struct spinel_drv_data *spinel_drv, uint32_t prop)
{
	uint8_t next_t_id;

	do {
		next_t_id = SPINEL_GET_NEXT_TID(spinel_drv->t_id.act_id);

		if (spinel_drv->t_id.act_id == next_t_id) {
			// All TIDs are used
			return -ENOMEM;
		}
	} while (spinel_drv->t_id.props[next_t_id] != SPINEL_PROP_UNDEFINED);

	spinel_drv->t_id.props[next_t_id] = prop;
	spinel_drv->t_id.act_id = next_t_id;

	return 0;
}

static bool spinel_drv_check_t_id(struct spinel_drv_data *spinel_drv,
	uint8_t t_id, uint32_t prop, bool clear_prop)
{
	if (!t_id) {
		// This event type (t_id = 0)
		return true;
	}

	if (spinel_drv->t_id.props[t_id] != prop) {
		return false;
	}

	if (clear_prop) {
		spinel_drv->t_id.props[t_id] = SPINEL_PROP_UNDEFINED;
	}

	return true;
}

static int spinel_drv_send_cmd(struct spinel_drv_data *spinel_drv,
	spinel_tx_cb tx_cb, const void *ctx, uint32_t cmd, uint32_t prop, const char *fmt, ...)
{
	int ret;
	va_list args;

	va_start(args, fmt);

	do {
		ret = spinel_drv_set_t_id(spinel_drv, prop);
		if (ret < 0) {
			LOG_ERR("Failed to set next Transaction ID (inst = %u, err = %d)",
				spinel_drv->inst, ret);
			break;
		}

		ret = spinel_datatype_pack(tx_cb, ctx,
			SPINEL_DATATYPE_COMMAND_PROP_S,
			SPINEL_HEADER_FLAG | SPINEL_HEADER_IID(spinel_drv->inst) | spinel_drv->t_id.act_id,
			cmd, prop);

		if (ret < 0) {
			LOG_ERR("Failed to pack spinel header (inst = %u, err = %d)", spinel_drv->inst, ret);
			break;
		}

		if (fmt) {
			int ret_vpack = spinel_datatype_vpack(tx_cb, ctx, fmt, &args);

			if (ret_vpack < 0) {
				ret = ret_vpack;
				LOG_ERR("Failed to pack spinel data (inst = %u, err = %d)", spinel_drv->inst, ret);
				break;
			}

			ret += ret_vpack;
		}
	} while (0);

	va_end(args);
	return ret;
}

static int spinel_drv_get_cmd(struct spinel_drv_data *spinel_drv, uint32_t cmd, uint32_t prop,
	const uint8_t *in_data, uint16_t in_data_size,
	const uint8_t **p_out_data, uint16_t *p_out_data_size)
{
	int ret;
	uint8_t header;
	uint32_t unpacked_cmd;
	uint32_t unpacked_prop;

	do {
		ret = spinel_datatype_unpack(in_data, in_data_size, false,
			SPINEL_DATATYPE_COMMAND_PROP_S SPINEL_DATATYPE_DATA_S,
			&header, &unpacked_cmd, &unpacked_prop, p_out_data, p_out_data_size);

		if (ret < 0) {
			LOG_ERR("Failed to unpack spinel header and data (inst = %u, err = %d)",
				spinel_drv->inst, ret);
			break;
		}

		if (spinel_drv->inst != SPINEL_HEADER_GET_IID(header) ||
				cmd != unpacked_cmd ||
				prop != unpacked_prop) {
			// Skip this frame
			ret = -EPERM;
			break;
		}

		if (!spinel_drv_check_t_id(spinel_drv, SPINEL_HEADER_GET_TID(header), prop, true)) {
			ret = -EIO;
			LOG_ERR("Unexpected Transaction ID (inst = %u)", spinel_drv->inst);
			break;
		}

		ret = 0;
	} while(0);

	if (ret < 0) {
		*p_out_data = NULL;
		*p_out_data_size = 0;
	}

	return ret;
}

void spinel_drv_init(struct spinel_drv_data *spinel_drv, uint8_t inst)
{
	spinel_drv->inst = inst;
	spinel_drv->t_id.act_id = 0;

	for (size_t i = 0; i < SPINEL_MAX_NUMB_TID; i++) {
		spinel_drv->t_id.props[i] = SPINEL_PROP_UNDEFINED;
	}
}

int spinel_drv_send_reset(struct spinel_drv_data *spinel_drv,
	spinel_tx_cb tx_cb, const void *ctx, uint8_t type)
{
	int ret = spinel_datatype_pack(tx_cb, ctx,
		SPINEL_DATATYPE_COMMAND_S SPINEL_DATATYPE_UINT8_S,
		SPINEL_HEADER_FLAG | SPINEL_HEADER_IID(spinel_drv->inst), SPINEL_CMD_RESET, type);

	if (ret < 0) {
		LOG_ERR("Failed to pack spinel reset command (inst = %u, err = %d)",
			spinel_drv->inst, ret);
	}

	return ret;
}

bool spinel_drv_check_reset(struct spinel_drv_data *spinel_drv,
	const uint8_t *data, uint16_t data_size)
{
	uint8_t header;
	uint32_t cmd;
	uint32_t prop;
	uint32_t status;

	int ret = spinel_datatype_unpack(data, data_size, false,
		SPINEL_DATATYPE_COMMAND_PROP_S SPINEL_DATATYPE_UINT_PACKED_S,
		&header, &cmd, &prop, &status);

	if (ret < 0) {
		LOG_ERR("Failed to unpack spinel header and data (inst = %u, err = %d)",
			spinel_drv->inst, ret);
		return false;
	}

	if (spinel_drv->inst != SPINEL_HEADER_GET_IID(header) ||
			cmd != SPINEL_CMD_PROP_VALUE_IS ||
			prop != SPINEL_PROP_LAST_STATUS) {
		return false;
	}

	if (status != SPINEL_STATUS_RESET_POWER_ON) {
		LOG_ERR("Incorrect reset status (inst = %u, status = %u)", spinel_drv->inst, status);
		return false;
	}

	return true;
}

bool spinel_drv_reception_data(struct spinel_drv_data *spinel_drv,
	const uint8_t *in_data, uint16_t in_data_size,
	const uint8_t **out_data, uint16_t *p_out_data_size)
{
	int ret = spinel_drv_get_cmd(spinel_drv, SPINEL_CMD_PROP_VALUE_IS, SPINEL_PROP_STREAM_RAW,
		in_data, in_data_size, out_data, p_out_data_size);

	if (ret == -EPERM) {
		// Skip this frame
		return false;
	} else if (ret < 0 || !*out_data || !*p_out_data_size) {
		LOG_ERR("Failed to get reception data (inst = %u, err = %d, data = %p, size = %u)",
			spinel_drv->inst, ret, out_data, *p_out_data_size);
		return false;
	}

	return true;
}

int spinel_drv_send_get_ieee_eui64(struct spinel_drv_data *spinel_drv,
	spinel_tx_cb tx_cb, const void *ctx)
{
	int ret = spinel_drv_send_cmd(spinel_drv, tx_cb, ctx,
		SPINEL_CMD_PROP_VALUE_GET, SPINEL_PROP_HWADDR, NULL);

	if (ret < 0) {
		LOG_ERR("Failed to send get_ieee_eui64 (inst = %u, err = %d)",
			spinel_drv->inst, ret);
	}

	return ret;
}

bool spinel_drv_check_get_ieee_eui64(struct spinel_drv_data *spinel_drv,
	const uint8_t *data, uint16_t data_size, uint8_t ieee_eui64[8])
{
	const uint8_t *param_data = NULL;
	uint16_t param_size = 0;

	int ret = spinel_drv_get_cmd(spinel_drv, SPINEL_CMD_PROP_VALUE_IS, SPINEL_PROP_HWADDR,
		data, data_size, &param_data, &param_size);

	if (ret == -EPERM) {
		// Skip this frame
		return false;
	} else if (ret < 0 || !param_data || param_size != sizeof(struct spinel_eui64)) {
		LOG_ERR("Failed to check get_ieee_eui64 (inst = %u, err = %d, data = %p, size = %u)",
			spinel_drv->inst, ret, param_data, param_size);
		return false;
	}

	ret = spinel_datatype_unpack(param_data, param_size, true,
		SPINEL_DATATYPE_EUI64_S, ieee_eui64);

	if (ret < 0) {
		LOG_ERR("Failed to get parameters of get_ieee_eui64 (inst = %u, err = %d)",
			spinel_drv->inst, ret);
			return false;
	}

	return true;
}

int spinel_drv_send_get_capabilities(struct spinel_drv_data *spinel_drv,
	spinel_tx_cb tx_cb, const void *ctx)
{
	int ret = spinel_drv_send_cmd(spinel_drv, tx_cb, ctx,
		SPINEL_CMD_PROP_VALUE_GET, SPINEL_PROP_RADIO_CAPS, NULL);

	if (ret < 0) {
		LOG_ERR("Failed to send get_capabilities (inst = %u, err = %d)",
			spinel_drv->inst, ret);
	}

	return ret;
}

bool spinel_drv_check_get_capabilities(struct spinel_drv_data *spinel_drv,
	const uint8_t *data, uint16_t data_size, enum ieee802154_hw_caps *radio_caps)
{
	const uint8_t *param_data = NULL;
	uint16_t param_size = 0;

	int ret = spinel_drv_get_cmd(spinel_drv, SPINEL_CMD_PROP_VALUE_IS, SPINEL_PROP_RADIO_CAPS,
		data, data_size, &param_data, &param_size);

	if (ret == -EPERM) {
		// Skip this frame
		return false;
	} else if (ret < 0 || !param_data || !param_size) {
		LOG_ERR("Failed to check get_capabilities (inst = %u, err = %d, data = %p, size = %u)",
			spinel_drv->inst, ret, param_data, param_size);
		return false;
	}

	/* TODO: parse caps response, now dummy data */
	*radio_caps = 0;
	
	return true;
}

int spinel_drv_send_enable_src_match(struct spinel_drv_data *spinel_drv,
	spinel_tx_cb tx_cb, const void *ctx, bool enable)
{
	/*
	 * modules/lib/openthread/src/lib/spinel/radio_spinel.hpp
	 * otError EnableSrcMatch(bool aEnable)
	 */
	return 0;
}

bool spinel_drv_check_enable_src_match(struct spinel_drv_data *spinel_drv,
	const uint8_t *data, uint16_t data_size)
{
	/*
	 * modules/lib/openthread/src/lib/spinel/radio_spinel.hpp
	 * otError EnableSrcMatch(bool aEnable)
	 */
	return true;
}

int spinel_drv_send_ack_fpb(struct spinel_drv_data *spinel_drv,
	spinel_tx_cb tx_cb, const void *ctx, uint16_t addr, bool enable)
{
	/*
	 * modules/lib/openthread/src/lib/spinel/radio_spinel.hpp
	 * otError AddSrcMatchShortEntry(uint16_t aShortAddress)
	 * otError ClearSrcMatchShortEntry(uint16_t aShortAddress)
	 */
	return 0;
}

bool spinel_drv_check_ack_fpb(struct spinel_drv_data *spinel_drv,
	const uint8_t *data, uint16_t data_size)
{
	/*
	 * modules/lib/openthread/src/lib/spinel/radio_spinel.hpp
	 * otError AddSrcMatchShortEntry(uint16_t aShortAddress)
	 * otError ClearSrcMatchShortEntry(uint16_t aShortAddress)
	 */
	return true;
}

int spinel_drv_send_ack_fpb_ext(struct spinel_drv_data *spinel_drv,
	spinel_tx_cb tx_cb, const void *ctx, uint8_t addr[8], bool enable)
{
	/*
	 * modules/lib/openthread/src/lib/spinel/radio_spinel.hpp
	 * otError AddSrcMatchExtEntry(const otExtAddress &aExtAddress)
	 * otError ClearSrcMatchExtEntry(const otExtAddress &aExtAddress)
	 */
	return 0;
}

bool spinel_drv_check_ack_fpb_ext(struct spinel_drv_data *spinel_drv,
	const uint8_t *data, uint16_t data_size)
{
	/*
	 * modules/lib/openthread/src/lib/spinel/radio_spinel.hpp
	 * otError AddSrcMatchExtEntry(const otExtAddress &aExtAddress)
	 * otError ClearSrcMatchExtEntry(const otExtAddress &aExtAddress)
	 */
	return true;
}

int spinel_drv_send_ack_fpb_clear(struct spinel_drv_data *spinel_drv,
	spinel_tx_cb tx_cb, const void *ctx)
{
	/*
	 * modules/lib/openthread/src/lib/spinel/radio_spinel.hpp
	 * otError ClearSrcMatchShortEntries(void)
	 */
	return 0;
}
bool spinel_drv_check_ack_fpb_clear(struct spinel_drv_data *spinel_drv,
	const uint8_t *data, uint16_t data_size)
{
	/*
	 * modules/lib/openthread/src/lib/spinel/radio_spinel.hpp
	 * otError ClearSrcMatchShortEntries(void)
	 */
	return true;
}
int spinel_drv_send_ack_fpb_ext_clear(struct spinel_drv_data *spinel_drv,
	spinel_tx_cb tx_cb, const void *ctx)
{
	/*
	 * modules/lib/openthread/src/lib/spinel/radio_spinel.hpp
	 * otError ClearSrcMatchExtEntry(const otExtAddress &aExtAddress)
	 */
	return 0;
}
bool spinel_drv_check_ack_fpb_ext_clear(struct spinel_drv_data *spinel_drv,
	const uint8_t *data, uint16_t data_size)
{
	/*
	 * modules/lib/openthread/src/lib/spinel/radio_spinel.hpp
	 * otError ClearSrcMatchExtEntry(const otExtAddress &aExtAddress)
	 */
	return true;
}
