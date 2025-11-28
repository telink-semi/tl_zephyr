/*
 * Copyright (c) 2023-2025 Telink Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT st_st25dvxxkc

#include <zephyr/kernel.h>

#include <zephyr/device.h>

#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/nfc/st25dv.h>
#include <zephyr/drivers/nfc/nfc_tag.h>
#include <zephyr/drivers/nfc/st25dvxxkc/stm32l4s5i_iot01_nfctag.h>
#include <zephyr/drivers/nfc/st25dvxxkc/tagtype5_wrapper.h>

#include <zephyr/logging/log.h>

#include <zephyr/sys/reboot.h>

LOG_MODULE_REGISTER(st25dvxxkc, CONFIG_ST25DVXXKC_LOG_LEVEL);

static int st25dvxxkc_tag_init(const struct device *dev, nfc_tag_cb_t cb)
{
	/* setup callback */
	struct st25dvxxkc_data *data = dev->data;

	data->nfc_tag_cb = cb;

	if (data->dev_i2c == NULL) {
		printk("Error dev\n");
		return -ENODEV;
	}
	/* Init of the Type Tag 5 component (ST25DV-I2C) */
	if (BSP_NFCTAG_Init(dev, 0) != NFCTAG_OK) {
		return NFCTAG_ERROR;
	}
	return 0;
}

static int st25dvxxkc_tag_set_type(const struct device *dev, enum nfc_tag_type type)
{
	struct st25dvxxkc_data *data = dev->data;

	/* st25dvxxkc only support T5T messages */
	if (type != NFC_TAG_TYPE_T5T) {
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
			printk("Cannot setup ST25\n");
			return NDEF_ERROR;
		}
	}

	/* Type is set */
	data->tag_type = type;
	return 0;
}

static int st25dvxxkc_tag_get_type(const struct device *dev, enum nfc_tag_type *type)
{
	struct st25dvxxkc_data *data = dev->data;
	*type = data->tag_type;
	return 0;
}

static int st25dvxxkc_tag_start(const struct device *dev)
{
	ARG_UNUSED(dev);
	BSP_NFCTAG_ResetRFSleep_Dyn(dev, 0);
	return 0;
}

static int st25dvxxkc_tag_stop(const struct device *dev)
{
	ARG_UNUSED(dev);
	BSP_NFCTAG_SetRFSleep_Dyn(dev, 0);
	return 0;
}

static int st25dvxxkc_tag_set_ndef(const struct device *dev, uint8_t *buf, uint16_t len)
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

static int st25dvxxkc_tag_cmd(const struct device *dev, enum nfc_tag_cmd cmd, uint8_t *buf,
			      uint16_t *buf_len)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(cmd);
	ARG_UNUSED(buf);
	ARG_UNUSED(buf_len);
	return 0;
}

static struct nfc_tag_driver_api _st25dvxxkc_driver_api = {.init = st25dvxxkc_tag_init,
							   .set_type = st25dvxxkc_tag_set_type,
							   .get_type = st25dvxxkc_tag_get_type,
							   .start = st25dvxxkc_tag_start,
							   .stop = st25dvxxkc_tag_stop,
							   .set_ndef = st25dvxxkc_tag_set_ndef,
							   .cmd = st25dvxxkc_tag_cmd};

/**
 * @brief Initialize NTAG driver IC.
 *
 * @param[in] *dev : Pointer to st25dvxxkc device
 * @return         : 0 on success, negative upon error.
 */
static int _st25dvxxkc_init(const struct device *dev)
{
	int rv = 0;
	struct st25dvxxkc_data *data = (struct st25dvxxkc_data *)dev->data;
	const struct st25dvxxkc_cfg *cfg = (const struct st25dvxxkc_cfg *)dev->config;

	LOG_DBG("st25dvxxkc: init");

	/* setup i2c */
	data->parent = (const struct device *)dev;
	data->dev_i2c = cfg->i2c.bus;

	if (data->dev_i2c == NULL) {
		LOG_ERR("Init I2C failed, could not bind, %s", cfg->i2c.bus->name);
		return -ENXIO;
	}

	LOG_DBG("st25dvxxkc: init OK");

	return rv;
}

#define ST25DVXXKC_INIT(inst)                                                                      \
	static struct st25dvxxkc_data st25dvxxkc_data##inst = {0};                                 \
	static const struct st25dvxxkc_cfg st25dvxxkc_cfg##inst = {                                \
		.i2c = I2C_DT_SPEC_INST_GET(inst),                                                 \
	};                                                                                         \
                                                                                                   \
	static int _st25dvxxkc_init##inst(const struct device *dev)                                \
	{                                                                                          \
		return _st25dvxxkc_init(dev);                                                      \
	}                                                                                          \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, _st25dvxxkc_init##inst, NULL, &st25dvxxkc_data##inst,          \
			      &st25dvxxkc_cfg##inst, POST_KERNEL, CONFIG_ST25DVXXKC_INIT_PRIORITY, \
			      &_st25dvxxkc_driver_api);

DT_INST_FOREACH_STATUS_OKAY(ST25DVXXKC_INIT)
