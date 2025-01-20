/*
 * Copyright (c) 2025 Telink Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "hdlc_interface.hpp"

#include <zephyr/drivers/uart.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(HDLC, LOG_LEVEL_INF); /* TODO: use level from config? */

namespace ot {
namespace Spinel {

const struct device *const HdlcInterface::m_uart_dev =
	DEVICE_DT_GET_OR_NULL(DT_CHOSEN(zephyr_ot_uart));

const size_t HdlcInterface::m_uart_buffer_size = 128; /* TODO: set from kConfig */

HdlcInterface::HdlcInterface() : m_uart_open(false), m_receive_frame_callback(nullptr),
    m_receive_frame_context(nullptr), m_receive_frame_buffer(nullptr)
{
	LOG_INF("Created");
}

HdlcInterface::~HdlcInterface()
{
	LOG_INF("Destroyed");
	Deinit();
}

void HdlcInterface::serial_cb(const struct device *dev, void *user_data)
{
	if (!uart_irq_update(dev)) {
		return;
	}
	if (!uart_irq_rx_ready(dev)) {
		return;
	}

	uint8_t bt;

	while (uart_fifo_read(dev, &bt, sizeof(bt)) == sizeof(bt)) {
		if (k_msgq_put(m_msgq, &bt, K_NO_WAIT)) {
			LOG_ERR("OT Spinel UART data lost");
		}
	}
}

otError HdlcInterface::Init(ReceiveFrameCallback aCallback,
	void *aCallbackContext, RxFrameBuffer &aFrameBuffer)
{
	otError result = OT_ERROR_FAILED;

	do {
		if (m_uart_open) {
			break;
		}
		if (!m_uart_dev) {
			LOG_ERR("OT Spinel no UART");
			break;
		}
		if (!device_is_ready(m_uart_dev)) {
			LOG_ERR("OT Spinel UART not ready");
			break;
		}
		if (k_msgq_alloc_init(m_msgq, sizeof(uint8_t), m_uart_buffer_size)) {
			LOG_ERR("OT Spinel can't allocate ISR buffer");
			break;
		}
		if (uart_irq_callback_user_data_set(m_uart_dev, serial_cb, nullptr)) {
			LOG_ERR("OT Spinel can't register ISR callback");
			k_msgq_cleanup(m_msgq);
			break;
		}
		uart_irq_rx_enable(m_uart_dev);
		m_uart_open = true;
	} while (0);

	if (m_uart_open) {
		m_receive_frame_callback = aCallback;
		m_receive_frame_context = aCallbackContext;
		m_receive_frame_buffer = &aFrameBuffer;
		result = OT_ERROR_NONE;
	}

	return result;
}

void HdlcInterface::Deinit(void)
{
	if (m_uart_open) {
		uart_irq_rx_disable(m_uart_dev);
		(void)uart_irq_callback_user_data_set(m_uart_dev, nullptr, nullptr);
		k_msgq_purge(m_msgq);
		(void)k_msgq_cleanup(m_msgq);
		m_uart_open = false;
	}
	m_receive_frame_callback = nullptr;
	m_receive_frame_context = nullptr;
	m_receive_frame_buffer = nullptr;
}

otError HdlcInterface::SendFrame(const uint8_t *aFrame, uint16_t aLength)
{
	return OT_ERROR_NOT_IMPLEMENTED;
}

otError HdlcInterface::WaitForFrame(uint64_t aTimeoutUs)
{
	return OT_ERROR_NOT_IMPLEMENTED;
}

void HdlcInterface::UpdateFdSet(void *aMainloopContext)
{
}

void HdlcInterface::Process(const void *aMainloopContext)
{
}

uint32_t HdlcInterface::GetBusSpeed(void) const
{
	return 0;
}

otError HdlcInterface::HardwareReset(void)
{
	return OT_ERROR_NOT_IMPLEMENTED;
}

const otRcpInterfaceMetrics *HdlcInterface::GetRcpInterfaceMetrics(void) const
{
	return 0;
}

} // namespace Spinel
} // namespace ot
