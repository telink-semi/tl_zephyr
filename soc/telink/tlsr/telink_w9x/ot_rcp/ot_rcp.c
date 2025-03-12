/*
 * Copyright (c) 2025 Telink Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "ot_rcp.h"
#include <stdlib.h>

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

static void openthread_rcp_reception_byte(uint8_t bt, const void *ctx)
{
	struct openthread_rcp_data *ot_rcp = (struct openthread_rcp_data *)ctx;

	if (!ot_rcp->spinel_rx_buffer.data) {
		ot_rcp->spinel_rx_buffer.data = malloc(CONFIG_TELINK_W91_OT_SPINEL_RX_BUFFER_SIZE);
		ot_rcp->spinel_rx_buffer.data_size = 0;
	}

	if (ot_rcp->spinel_rx_buffer.data) {
		if (ot_rcp->spinel_rx_buffer.data_size < CONFIG_TELINK_W91_OT_SPINEL_RX_BUFFER_SIZE) {
			ot_rcp->spinel_rx_buffer.data[ot_rcp->spinel_rx_buffer.data_size] = bt;
			ot_rcp->spinel_rx_buffer.data_size ++;
		} else {
			LOG_ERR("spinel rx buffer overflows");
		}
	} else {
		LOG_ERR("spinel can't allocate rx buffer");
	}
}

static void openthread_rcp_reception_done(bool data_valid, const void *ctx)
{
	if (data_valid) {
		struct openthread_rcp_data *ot_rcp = (struct openthread_rcp_data *)ctx;

		if (ot_rcp->spinel_rx_buffer.data) {
			const uint8_t *out_data = NULL;
			uint16_t out_data_size = 0;

			ot_rcp->spinel_rx_buffer.data_size -= HDLC_CODER_LENGTH_CRC;
			if (spinel_drv_reception_data(&ot_rcp->spinel_drv,
				ot_rcp->spinel_rx_buffer.data, ot_rcp->spinel_rx_buffer.data_size,
				&out_data, &out_data_size)) {
				if (ot_rcp->reception) {
					ot_rcp->reception((uint8_t *)out_data, out_data_size, ot_rcp->ctx);
				}
				free(ot_rcp->spinel_rx_buffer.data);
			} else {
				k_msgq_put(&ot_rcp->spinel_msgq, &ot_rcp->spinel_rx_buffer, K_FOREVER);
			}
			ot_rcp->spinel_rx_buffer.data = NULL;
		}
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
		hdlc_coder_inp_data_set(&ot_rcp->hdlc, openthread_rcp_reception_byte);
		hdlc_coder_inp_finish_set(&ot_rcp->hdlc, openthread_rcp_reception_done);
		ot_rcp->spinel_rx_buffer.data = NULL;
		k_msgq_init(&ot_rcp->spinel_msgq, (char *)&ot_rcp->spinel_msgq_buffer,
			sizeof(struct openthread_rcp_rx_buffer), ARRAY_SIZE(ot_rcp->spinel_msgq_buffer));
		spinel_drv_init(&ot_rcp->spinel_drv, 0);
		ot_rcp->reception = NULL;
		ot_rcp->ctx = NULL;

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

void openthread_rcp_reception_set(struct openthread_rcp_data *ot_rcp,
	openthread_rcp_reception reception, const void *ctx)
{
	ot_rcp->reception = reception;
	ot_rcp->ctx = ctx;
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
		k_msgq_purge(&ot_rcp->spinel_msgq);
	} while (0);

	return result;
}


#define OPENTHREAD_RCP_HELPER(repsonse_parser, ...)                                                \
	hdlc_coder_out_finish(&ot_rcp->hdlc, result >= 0);                                             \
	if (result >= 0) {                                                                             \
		result = -ETIMEDOUT;                                                                       \
		k_timepoint_t t = sys_timepoint_calc(                                                      \
			K_MSEC(CONFIG_TELINK_W91_OT_SPINEL_RESPONSE_TIMEOUT_MS));                              \
                                                                                                   \
		while (!sys_timepoint_expired(t)) {                                                        \
			struct openthread_rcp_rx_buffer response;                                              \
                                                                                                   \
			if (!k_msgq_get(&ot_rcp->spinel_msgq, &response, sys_timepoint_timeout(t))) {          \
				LOG_HEXDUMP_INF(response.data, response.data_size, "rx");                          \
				if (repsonse_parser(&ot_rcp->spinel_drv,                                           \
						response.data, response.data_size, ##__VA_ARGS__)) {                       \
					result = 0;                                                                    \
				} else {                                                                           \
					LOG_WRN("spinel trash received");                                              \
				}                                                                                  \
				free(response.data);                                                               \
				if (!result) {                                                                     \
					break;                                                                         \
				}                                                                                  \
			}                                                                                      \
		}                                                                                          \
		if (result == -ETIMEDOUT) {                                                                \
			LOG_ERR("spinel response timeout");                                                    \
		}                                                                                          \
	} else {                                                                                       \
		LOG_ERR("spinel encoding error");                                                          \
	}                                                                                              \
	return result;

int openthread_rcp_reset(struct openthread_rcp_data *ot_rcp)
{
	LOG_INF("%s", __func__);

	int result = spinel_drv_send_reset(&ot_rcp->spinel_drv,
		openthread_rcp_spinel_transmission, ot_rcp, SPINEL_RESET_STACK);

	OPENTHREAD_RCP_HELPER(spinel_drv_check_reset);
}

int openthread_rcp_ieee_eui64(struct openthread_rcp_data *ot_rcp, uint8_t ieee_eui64[8])
{
	LOG_INF("%s", __func__);

	int result = spinel_drv_send_get_ieee_eui64(&ot_rcp->spinel_drv,
		openthread_rcp_spinel_transmission, ot_rcp);

	OPENTHREAD_RCP_HELPER(spinel_drv_check_get_ieee_eui64, ieee_eui64);
}

int openthread_rcp_capabilities(struct openthread_rcp_data *ot_rcp,
	enum ieee802154_hw_caps *radio_caps)
{
	LOG_INF("%s", __func__);

	int result = spinel_drv_send_get_capabilities(&ot_rcp->spinel_drv,
		openthread_rcp_spinel_transmission, ot_rcp);

	OPENTHREAD_RCP_HELPER(spinel_drv_check_get_capabilities, radio_caps);
}

int openthread_rcp_enable_src_match(struct openthread_rcp_data *ot_rcp, bool enable)
{
	LOG_INF("%s", __func__);

	int result = spinel_drv_send_enable_src_match(&ot_rcp->spinel_drv,
		openthread_rcp_spinel_transmission, ot_rcp, enable);

	OPENTHREAD_RCP_HELPER(spinel_drv_check_enable_src_match, enable);
}

int openthread_rcp_ack_fpb(struct openthread_rcp_data *ot_rcp, uint16_t addr, bool enable)
{
	LOG_INF("%s", __func__);

	int result = spinel_drv_send_ack_fpb(&ot_rcp->spinel_drv,
		openthread_rcp_spinel_transmission, ot_rcp, addr, enable);

	OPENTHREAD_RCP_HELPER(spinel_drv_check_ack_fpb, addr, enable);
}

int openthread_rcp_ack_fpb_ext(struct openthread_rcp_data *ot_rcp, uint8_t addr[8], bool enable)
{
	LOG_INF("%s", __func__);

	int result = spinel_drv_send_ack_fpb_ext(&ot_rcp->spinel_drv,
		openthread_rcp_spinel_transmission, ot_rcp, addr, enable);

	OPENTHREAD_RCP_HELPER(spinel_drv_check_ack_fpb_ext, addr, enable);
}

static int openthread_rcp_ack_fpb_short_clear(struct openthread_rcp_data *ot_rcp)
{
	LOG_INF("%s", __func__);

	int result = spinel_drv_send_ack_fpb_clear(&ot_rcp->spinel_drv,
		openthread_rcp_spinel_transmission, ot_rcp);

	OPENTHREAD_RCP_HELPER(spinel_drv_check_ack_fpb_clear);
}

static int openthread_rcp_ack_fpb_ext_clear(struct openthread_rcp_data *ot_rcp)
{
	LOG_INF("%s", __func__);

	int result = spinel_drv_send_ack_fpb_ext_clear(&ot_rcp->spinel_drv,
		openthread_rcp_spinel_transmission, ot_rcp);

	OPENTHREAD_RCP_HELPER(spinel_drv_check_ack_fpb_ext_clear);
}

int openthread_rcp_ack_fpb_clear(struct openthread_rcp_data *ot_rcp)
{
	int result = openthread_rcp_ack_fpb_short_clear(ot_rcp);

	if (!result) {
		result = openthread_rcp_ack_fpb_ext_clear(ot_rcp);
	}
	return result;
}

int openthread_rcp_mac_frame_counter(struct openthread_rcp_data *ot_rcp,
	uint32_t frame_counter, bool set_if_larger)
{
	LOG_INF("%s", __func__);

	int result = spinel_drv_send_mac_frame_counter(&ot_rcp->spinel_drv,
		openthread_rcp_spinel_transmission, ot_rcp, frame_counter, set_if_larger);

	OPENTHREAD_RCP_HELPER(spinel_drv_check_mac_frame_counter, frame_counter);
}

int openthread_rcp_panid(struct openthread_rcp_data *ot_rcp, uint16_t pan_id)
{
	LOG_INF("%s", __func__);

	int result = spinel_drv_send_panid(&ot_rcp->spinel_drv,
		openthread_rcp_spinel_transmission, ot_rcp, pan_id);

	OPENTHREAD_RCP_HELPER(spinel_drv_check_panid, pan_id);
}

int openthread_rcp_short_addr(struct openthread_rcp_data *ot_rcp, uint16_t addr)
{
	LOG_INF("%s", __func__);

	int result = spinel_drv_send_short_addr(&ot_rcp->spinel_drv,
		openthread_rcp_spinel_transmission, ot_rcp, addr);

	OPENTHREAD_RCP_HELPER(spinel_drv_check_short_addr, addr);
}

int openthread_rcp_ext_addr(struct openthread_rcp_data *ot_rcp, uint8_t addr[8])
{
	LOG_INF("%s", __func__);

	int result = spinel_drv_send_ext_addr(&ot_rcp->spinel_drv,
		openthread_rcp_spinel_transmission, ot_rcp, addr);

	OPENTHREAD_RCP_HELPER(spinel_drv_check_ext_addr, addr);
}

int openthread_rcp_tx_power(struct openthread_rcp_data *ot_rcp, int8_t pwr_dbm)
{
	LOG_INF("%s", __func__);

	int result = spinel_drv_send_tx_power(&ot_rcp->spinel_drv,
		openthread_rcp_spinel_transmission, ot_rcp, pwr_dbm);

	OPENTHREAD_RCP_HELPER(spinel_drv_check_tx_power, pwr_dbm);
}

int openthread_rcp_enable(struct openthread_rcp_data *ot_rcp, bool enable)
{
	LOG_INF("%s", __func__);

	int result = spinel_drv_send_rcp_enable(&ot_rcp->spinel_drv,
		openthread_rcp_spinel_transmission, ot_rcp, enable);

	OPENTHREAD_RCP_HELPER(spinel_drv_check_rcp_enable, enable);
}

int openthread_rcp_receive_on(struct openthread_rcp_data *ot_rcp, uint8_t channel)
{
	LOG_INF("%s", __func__);

	int result = spinel_drv_send_receive_on(&ot_rcp->spinel_drv,
		openthread_rcp_spinel_transmission, ot_rcp, channel);

	OPENTHREAD_RCP_HELPER(spinel_drv_check_receive_on, channel);
}
