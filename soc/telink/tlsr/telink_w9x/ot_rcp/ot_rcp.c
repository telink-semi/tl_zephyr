/*
 * Copyright (c) 2025 Telink Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "ot_rcp.h"
#include "spinel_drv.h"

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
		hdlc_coder_inp_poll(&ot_rcp->hdlc, bt);
	}
}

static void openthread_rcp_spinel_transmission(uint8_t bt, const void *ctx)
{
	struct openthread_rcp_data *ot_rcp = (struct openthread_rcp_data *)ctx;

	hdlc_coder_out_poll(&ot_rcp->hdlc, bt);
}

static void openthread_rcp_hdlc_transmission(uint8_t bt, const void *ctx)
{
	struct openthread_rcp_data *ot_rcp = (struct openthread_rcp_data *)ctx;

	uart_poll_out(ot_rcp->uart, bt);
}

static void openthread_rcp_reception(uint8_t bt, const void *ctx)
{
	struct openthread_rcp_data *ot_rcp = (struct openthread_rcp_data *)ctx;

	LOG_INF("spinel rx [%p] %02x", ot_rcp, bt);
	/* TODO: unpack using spinel */
}

static void openthread_rcp_reception_done(bool data_valid, const void *ctx)
{
	if (data_valid) {
		struct openthread_rcp_data *ot_rcp = (struct openthread_rcp_data *)ctx;

		LOG_INF("spinel rx finish [%p]", ot_rcp);
		/* TODO: analyze received spinel data */
	}
}

/************************************************************************
 * RCP interface functions
 ************************************************************************/

int openthread_rcp_init(struct openthread_rcp_data *ot_rcp, const struct device *uart)
{
	int result = 0;

	do {
		if (!device_is_ready(uart)) {
			LOG_ERR("spinel serial not ready");
			result = -EIO;
			break;
		}

		ot_rcp->uart = uart;
		k_work_init(&ot_rcp->work, openthread_rcp_reception_work);
		ring_buf_init(&ot_rcp->rb, sizeof(ot_rcp->rb_data), ot_rcp->rb_data);
		hdlc_coder_init(&ot_rcp->hdlc, ot_rcp);
		hdlc_coder_out_data_set(&ot_rcp->hdlc, openthread_rcp_hdlc_transmission);
		hdlc_coder_inp_data_set(&ot_rcp->hdlc, openthread_rcp_reception);
		hdlc_coder_inp_finish_set(&ot_rcp->hdlc, openthread_rcp_reception_done);

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

	int ret = spinel_drv_send_reset(0, openthread_rcp_spinel_transmission, ot_rcp,
		SPINEL_RESET_STACK);

	if (ret < 0) {
		LOG_ERR("Failed to send spinel reset command (err = %d)", ret);
		return ret;
	}

	hdlc_coder_out_finish(&ot_rcp->hdlc, true);

#if 0
	int8_t t_id = 0;

	// Test semd get protocol version command
	ret = spinel_drv_send_cmd(0, t_id, openthread_rcp_spinel_transmission, ot_rcp,
		SPINEL_CMD_PROP_VALUE_GET, SPINEL_PROP_PROTOCOL_VERSION, NULL);

	if (ret < 0) {
		LOG_ERR("Failed to send spinel command (err = %d)", ret);
		return ret;
	}

	hdlc_coder_out_finish(&ot_rcp->hdlc, true);

	// Test stream raw command
	bool test_bool = true;
	uint8_t test_uint8 = 0x10;
	uint16_t test_uint16 = 0x3322;
	uint32_t test_uint32 = 0x77665544;
	uint64_t test_uint64 = 0xFFEEDDCCBBAA9988;
	unsigned int test_uint = 128; // 80 01
	const char *test_utf8 = "Hello"; // 48 65 6C 6C 6F 00

	uint8_t test_data_wlen[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
	ret = spinel_drv_send_cmd(0, t_id, openthread_rcp_spinel_transmission, ot_rcp,
		SPINEL_CMD_PROP_VALUE_SET, SPINEL_PROP_STREAM_RAW,
		SPINEL_DATATYPE_BOOL_S
		SPINEL_DATATYPE_UINT8_S
		SPINEL_DATATYPE_UINT16_S
		SPINEL_DATATYPE_UINT32_S
		SPINEL_DATATYPE_UINT64_S
		SPINEL_DATATYPE_UINT_PACKED_S
		SPINEL_DATATYPE_UTF8_S
		SPINEL_DATATYPE_DATA_WLEN_S,
		test_bool, test_uint8, test_uint16, test_uint32, test_uint64,
		test_uint, test_utf8, test_data_wlen, sizeof(test_data_wlen));

	if (ret < 0) {
		LOG_ERR("Failed to send spinel data (err = %d)", ret);
		return ret;
	}

	hdlc_coder_out_finish(&ot_rcp->hdlc, true);

#endif

	/* TODO: wait for response */

	return 0;
}
