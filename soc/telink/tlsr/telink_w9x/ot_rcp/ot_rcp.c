/*
 * Copyright (c) 2025 Telink Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "ot_rcp.h"

#define LOG_LEVEL CONFIG_IEEE802154_DRIVER_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(ot_rcp);

#include <zephyr/init.h>
#include <zephyr/drivers/uart.h>

/************************************************************************
 * RCP reception work queue
 ************************************************************************/

static K_KERNEL_STACK_DEFINE(openthread_rcp_work_q_stack,
	CONFIG_TELINK_W91_OT_RCP_THREAD_STACK_SIZE);

static struct k_work_q openthread_rcp_work_q;

static int openthread_rcp_work_q_init(void)
{
	struct k_work_queue_config cfg = {
		.name = "rcpworkq",
		.no_yield = false,
		.essential = false
	};

	k_work_queue_start(&openthread_rcp_work_q,
		openthread_rcp_work_q_stack,
		K_KERNEL_STACK_SIZEOF(openthread_rcp_work_q_stack),
		CONFIG_TELINK_W91_OT_RCP_THREAD_PRIORITY, &cfg);
	return 0;
}

SYS_INIT(openthread_rcp_work_q_init, POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);

/************************************************************************
 * RCP internal functions
 ************************************************************************/

static void openthread_rcp_reception_isr(const struct device *dev, void *user_data)
{
	struct openthread_rcp_data *ot_rcp = (struct openthread_rcp_data *)user_data;

	if (!uart_irq_update(dev)) {
		return;
	}
	if (!uart_irq_rx_ready(dev)) {
		return;
	}
	uint8_t bt;

	while (uart_fifo_read(dev, &bt, sizeof(bt)) == sizeof(bt)) {
		if (ring_buf_put(&ot_rcp->rb, &bt, sizeof(bt)) != sizeof(bt)) {
			LOG_ERR("spinel serial rx overflow");
		}
	}
	k_work_submit_to_queue(&openthread_rcp_work_q, &ot_rcp->work);
}

static void openthread_rcp_reception_work(struct k_work *item)
{
	struct openthread_rcp_data *ot_rcp =
		CONTAINER_OF(item, struct openthread_rcp_data, work);
	uint8_t bt;

	while (ring_buf_get(&ot_rcp->rb, &bt, sizeof(bt)) == sizeof(bt)) {
		LOG_INF("spinel rx %02x", bt);
	}
}

/************************************************************************
 * RCP interface functions
 ************************************************************************/

int openthread_rcp_init(struct openthread_rcp_data *ot_rcp,
	const struct device *uart, const void *ctx)
{
	int result = 0;

	do {
		if (!device_is_ready(uart)) {
			LOG_ERR("spinel serial not ready");
			result = -EIO;
			break;
		}

		ot_rcp->uart = uart;
		ot_rcp->ctx = ctx;
		k_work_init(&ot_rcp->work, openthread_rcp_reception_work);
		ring_buf_init(&ot_rcp->rb, sizeof(ot_rcp->rb_data),
			ot_rcp->rb_data);

		if (uart_irq_callback_user_data_set(ot_rcp->uart,
			openthread_rcp_reception_isr, ot_rcp)) {
			LOG_ERR("can't set serial isr");
			result = -EIO;
			break;
		}
		uart_irq_rx_enable(ot_rcp->uart);
	} while (0);

	return result;
}

int openthread_rcp_deinit(struct openthread_rcp_data *ot_rcp)
{
	int result = 0;

	do {
		uart_irq_rx_disable(ot_rcp->uart);
		if (uart_irq_callback_user_data_set(ot_rcp->uart, NULL, NULL)) {
			LOG_ERR("can't reset serial isr");
			result = -EIO;
			break;
		}

		struct k_work_sync work_sync;

		(void) k_work_cancel_sync(&ot_rcp->work, &work_sync);
		ring_buf_reset(&ot_rcp->rb);
	} while (0);

	return result;
}

int openthread_rcp_reset(struct openthread_rcp_data *ot_rcp)
{
	LOG_INF("%s", __func__);
	/* Dummy for now */
	uint8_t reset_cmd[] = {0x7e, 0x80, 0x01, 0x02, 0xea, 0xf0, 0x7e};

	for(uint16_t i = 0; i < sizeof(reset_cmd); i++) {
		uart_poll_out(ot_rcp->uart, reset_cmd[i]);
	}

	return 0;
}
