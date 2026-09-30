/*
 * Copyright (c) 2023-2026 Telink Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#define DT_DRV_COMPAT st_st25dvxxkc

#include <zephyr/drivers/i2c.h>

#include <zephyr/drivers/nfc/st25dv.h>
#include <zephyr/drivers/nfc/nfc_tag.h>
#include <zephyr/drivers/nfc/st25dvxxkc/lib_NDEF.h>
#include <zephyr/drivers/nfc/st25dvxxkc/tagtype5_wrapper.h>

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(st25dvxxkc, CONFIG_ST25DVXXKC_LOG_LEVEL);

#define ST25DV_USER_MEMORY_ADDR 0x53
#define ST25DV_SYSTEM_AREA_ADDR 0x57

static int st25dv_tag_init(const struct device *dev, nfc_tag_cb_t cb)
{
	LOG_DBG("st25dv tag init");

	struct st25dvxxkc_data *data = dev->data;

	data->nfc_tag_cb = cb;
	/* Init of the Type Tag 5 component (ST25DV-I2C) */
	if (BSP_NFCTAG_Init(dev, 0) != NFCTAG_OK) {
		return NFCTAG_ERROR;
	}
	return 0;
}

static int st25dv_tag_set_type(const struct device *dev, enum nfc_tag_type type)
{
	struct st25dvxxkc_data *data = dev->data;

	/* st25dvxxkc only support T5T messages */
	if (type != NFC_TAG_TYPE_T5T) {
		LOG_ERR("unsupported tag type");
		return -ENOTSUP;
	}

	NfcTag_SelectProtocol(NFCTAG_TYPE5);

	/* Check if no NDEF detected, init mem in Tag Type 5 */
	if (NfcType5_NDEFDetection(dev) != NDEF_OK) {
		CCFileStruct.MagicNumber = NFCT5_MAGICNUMBER_E1_CCFILE;
		CCFileStruct.Version = NFCT5_VERSION_V1_0;
		CCFileStruct.MemorySize = (ST25DVXXKC_NDEF_MAX_SIZE / 8) & 0xFF;
		CCFileStruct.TT5Tag = 0x05;
		/* Init of the Type Tag 5 component */
		if (NfcType5_TT5Init(dev) != NFCTAG_OK) {
			LOG_ERR("can't setup ST25 tag");
			return NDEF_ERROR;
		}
	}

	/* Type is set */
	data->tag_type = type;
	return 0;
}

static int st25dv_tag_get_type(const struct device *dev, enum nfc_tag_type *type)
{
	struct st25dvxxkc_data *data = dev->data;
	*type = data->tag_type;
	return 0;
}

static int st25dv_tag_start(const struct device *dev)
{
	ARG_UNUSED(dev);
	BSP_NFCTAG_ResetRFSleep_Dyn(dev, 0);
	return 0;
}

static int st25dv_tag_stop(const struct device *dev)
{
	ARG_UNUSED(dev);
	BSP_NFCTAG_SetRFSleep_Dyn(dev, 0);
	return 0;
}

static int st25dv_tag_set_ndef(const struct device *dev, uint8_t *buf, uint16_t len)
{
	uint8_t current_ndef[ST25DVXXKC_NDEF_MAX_SIZE] = {0};
	int rv = NfcTag_ReadNDEF(dev, current_ndef);

	if (rv) {
		return rv;
	}
	if (memcmp(current_ndef, buf, len)) {
		NfcTag_WriteNDEF(dev, 0, NULL);
		rv = NfcTag_WriteNDEF(dev, len, buf);
	}
	return rv;
}

static int st25dv_tag_cmd(const struct device *dev, enum nfc_tag_cmd cmd, uint8_t *buf,
			  uint16_t *buf_len)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(cmd);
	ARG_UNUSED(buf);
	ARG_UNUSED(buf_len);
	return 0;
}

static int st25dv_init(const struct device *dev)
{
	int result = -ENODEV;

	do {
		struct st25dvxxkc_data *data = dev->data;

		if (!device_is_ready(data->dev_i2c)) {
			LOG_ERR("st25dv i2c bus %s not ready", data->dev_i2c->name);
			break;
		}

		struct i2c_msg msg = {
			.flags = I2C_MSG_WRITE | I2C_MSG_STOP,
		};

		result = i2c_transfer(data->dev_i2c, &msg, 1, ST25DV_USER_MEMORY_ADDR);
		if (result) {
			LOG_ERR("st25dv device not detected %s : 0x%02x", data->dev_i2c->name,
				ST25DV_USER_MEMORY_ADDR);
			break;
		}
		result = i2c_transfer(data->dev_i2c, &msg, 1, ST25DV_SYSTEM_AREA_ADDR);
		if (result) {
			LOG_ERR("st25dv device not detected %s : 0x%02x", data->dev_i2c->name,
				ST25DV_SYSTEM_AREA_ADDR);
			break;
		}

		LOG_DBG("st25dv device  %s ok", data->dev_i2c->name);
	} while (0);

	return result;
}

static struct nfc_tag_driver_api st25dv_api = {
	.init = st25dv_tag_init,
	.set_type = st25dv_tag_set_type,
	.get_type = st25dv_tag_get_type,
	.start = st25dv_tag_start,
	.stop = st25dv_tag_stop,
	.set_ndef = st25dv_tag_set_ndef,
	.cmd = st25dv_tag_cmd,
};

#define ST25DV_DEFINE(i)                                                                           \
                                                                                                   \
	static struct st25dvxxkc_data st25dv_data##i = {                                           \
		.dev_i2c = DEVICE_DT_GET(DT_INST_BUS(i)),                                          \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(i, st25dv_init, NULL, &st25dv_data##i, NULL, POST_KERNEL,            \
			      CONFIG_ST25DVXXKC_INIT_PRIORITY, &st25dv_api);

DT_INST_FOREACH_STATUS_OKAY(ST25DV_DEFINE)
