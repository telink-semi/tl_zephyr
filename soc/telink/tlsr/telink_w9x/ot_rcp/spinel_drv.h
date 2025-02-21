/*
 * Copyright (c) 2025 Telink Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef SPINEL_DRV_H
#define SPINEL_DRV_H

#include <zephyr/kernel.h>

#include "spinel.h"

#define SPINEL_DRV_RECEPTION_DATA_HEADER_LEN         3

struct spinel_drv_data {
	uint8_t inst;
	uint8_t t_id;
};

void spinel_drv_init(struct spinel_drv_data *spinel_drv, uint8_t inst);
int spinel_drv_send_reset(struct spinel_drv_data *spinel_drv,
	spinel_tx_cb tx_cb, const void *ctx, uint8_t type);
bool spinel_drv_check_reset(struct spinel_drv_data *spinel_drv,
	const uint8_t *data, size_t data_size);
int spinel_drv_send_cmd(struct spinel_drv_data *spinel_drv,
	spinel_tx_cb tx_cb, const void *ctx, uint32_t cmd, uint32_t prop, const char *fmt, ...);
bool spinel_drv_reception_data(struct spinel_drv_data *spinel_drv,
	const uint8_t *data, size_t data_size);
int spinel_drv_send_get_ieee_eui64(struct spinel_drv_data *spinel_drv,
	spinel_tx_cb tx_cb, const void *ctx);
bool spinel_drv_check_get_ieee_eui64(struct spinel_drv_data *spinel_drv,
	const uint8_t *data, size_t data_size, uint8_t ieee_eui64[8]);

#endif /* SPINEL_DRV_H */
