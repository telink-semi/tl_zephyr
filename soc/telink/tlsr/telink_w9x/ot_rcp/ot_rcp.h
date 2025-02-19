/*
 * Copyright (c) 2025 Telink Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef OPENTHREAD_RCP_H
#define OPENTHREAD_RCP_H

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/sys/ring_buffer.h>

#include "hdlc_coder.h"

struct openthread_rcp_data {
	const struct device *uart;
	struct k_work work;
	struct ring_buf rb;
	uint8_t rb_data[CONFIG_TELINK_W91_OT_RCP_RX_BUFFER_SIZE];
	struct hdlc_coder hdlc;
};

int openthread_rcp_init(struct openthread_rcp_data *ot_rcp, const struct device *uart);
int openthread_rcp_deinit(struct openthread_rcp_data *ot_rcp);
int openthread_rcp_reset(struct openthread_rcp_data *ot_rcp);

#endif /* OPENTHREAD_RCP_H */
