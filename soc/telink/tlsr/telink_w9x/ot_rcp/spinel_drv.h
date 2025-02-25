/*
 * Copyright (c) 2025 Telink Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef SPINEL_DRV_H
#define SPINEL_DRV_H

#include <zephyr/kernel.h>
#include <zephyr/net/ieee802154_radio.h>

#include "spinel.h"

struct spinel_t_id {
	uint8_t act_id;
	uint32_t props[SPINEL_MAX_NUMB_TID];
};

struct spinel_drv_data {
	uint8_t inst;
	struct spinel_t_id t_id;
};

void spinel_drv_init(struct spinel_drv_data *spinel_drv, uint8_t inst);
int spinel_drv_send_reset(struct spinel_drv_data *spinel_drv,
	spinel_tx_cb tx_cb, const void *ctx, uint8_t type);
bool spinel_drv_check_reset(struct spinel_drv_data *spinel_drv,
	const uint8_t *data, uint16_t data_size);
bool spinel_drv_reception_data(struct spinel_drv_data *spinel_drv,
	const uint8_t *in_data, uint16_t in_data_size,
	const uint8_t **out_data, uint16_t *p_out_data_size);
int spinel_drv_send_get_ieee_eui64(struct spinel_drv_data *spinel_drv,
	spinel_tx_cb tx_cb, const void *ctx);
bool spinel_drv_check_get_ieee_eui64(struct spinel_drv_data *spinel_drv,
	const uint8_t *data, uint16_t data_size, uint8_t ieee_eui64[8]);
int spinel_drv_send_get_capabilities(struct spinel_drv_data *spinel_drv,
	spinel_tx_cb tx_cb, const void *ctx);
bool spinel_drv_check_get_capabilities(struct spinel_drv_data *spinel_drv,
	const uint8_t *data, uint16_t data_size, enum ieee802154_hw_caps *radio_caps);
int spinel_drv_send_enable_src_match(struct spinel_drv_data *spinel_drv,
	spinel_tx_cb tx_cb, const void *ctx, bool enable);
bool spinel_drv_check_enable_src_match(struct spinel_drv_data *spinel_drv,
	const uint8_t *data, uint16_t data_size);
int spinel_drv_send_ack_fpb(struct spinel_drv_data *spinel_drv,
	spinel_tx_cb tx_cb, const void *ctx, uint16_t addr, bool enable);
bool spinel_drv_check_ack_fpb(struct spinel_drv_data *spinel_drv,
	const uint8_t *data, uint16_t data_size);
int spinel_drv_send_ack_fpb_ext(struct spinel_drv_data *spinel_drv,
	spinel_tx_cb tx_cb, const void *ctx, uint8_t addr[8], bool enable);
bool spinel_drv_check_ack_fpb_ext(struct spinel_drv_data *spinel_drv,
	const uint8_t *data, uint16_t data_size);
int spinel_drv_send_ack_fpb_clear(struct spinel_drv_data *spinel_drv,
	spinel_tx_cb tx_cb, const void *ctx);
bool spinel_drv_check_ack_fpb_clear(struct spinel_drv_data *spinel_drv,
	const uint8_t *data, uint16_t data_size);
int spinel_drv_send_ack_fpb_ext_clear(struct spinel_drv_data *spinel_drv,
	spinel_tx_cb tx_cb, const void *ctx);
bool spinel_drv_check_ack_fpb_ext_clear(struct spinel_drv_data *spinel_drv,
	const uint8_t *data, uint16_t data_size);

#endif /* SPINEL_DRV_H */
