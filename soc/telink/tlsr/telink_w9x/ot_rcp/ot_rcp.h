/*
 * Copyright (c) 2025 Telink Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef OPENTHREAD_RCP_H
#define OPENTHREAD_RCP_H

#include <zephyr/kernel.h>
#include <zephyr/device.h>

typedef void (*openthread_rcp_ack)(uint8_t *data, size_t data_len, const void *ctx);
typedef void (*openthread_rcp_rx)(uint8_t *data, size_t data_len, const void *ctx);

struct openthread_rcp_data {
	const struct device *uart;
	struct k_sem response_sem;
	openthread_rcp_ack ack;
	openthread_rcp_rx rx;
	const void *ctx;
};

int openthread_rcp_init(struct openthread_rcp_data *ot_rcp);
int openthread_rcp_deinit(struct openthread_rcp_data *ot_rcp);
int openthread_rcp_reset(struct openthread_rcp_data *ot_rcp);

#endif /* OPENTHREAD_RCP_H */
