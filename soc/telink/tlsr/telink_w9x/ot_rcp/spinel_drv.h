/*
 * Copyright (c) 2025 Telink Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef SPINEL_DRV_H
#define SPINEL_DRV_H

#include <zephyr/kernel.h>

#include "spinel.h"

int spinel_drv_send_reset(uint8_t inst, spinel_tx_cb tx_cb, const void *ctx, uint8_t type);
bool spinel_drv_check_reset(const void *ctx, uint8_t *data, size_t data_size);
int spinel_drv_send_cmd(uint8_t inst, int8_t t_id, spinel_tx_cb tx_cb, const void *ctx,
	uint32_t cmd, uint32_t prop, const char *fmt, ...);

#endif /* SPINEL_DRV_H */
