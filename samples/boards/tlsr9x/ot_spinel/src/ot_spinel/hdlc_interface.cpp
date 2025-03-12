/*
 * Copyright (c) 2025 Telink Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "hdlc_interface.hpp"

#include <zephyr/drivers/uart.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(HDLC, LOG_LEVEL_INF); // TODO: use level from config?

namespace ot {
namespace Spinel {

const size_t HdlcInterface::k_uart_buffer_size = 128; // TODO: set from kConfig

HdlcInterface::HdlcInterface(const struct device *const uart_dev) :
	m_uart_open(false),
	m_uart_dev(uart_dev),
	m_msgq(),
	m_hdlc_decoder(),
	m_receive_frame_callback(nullptr),
	m_receive_frame_context(nullptr),
	m_receive_frame_buffer(nullptr),
	m_rcp_interface_metrics()
{
	memset(&m_rcp_interface_metrics, 0, sizeof(m_rcp_interface_metrics));
	m_rcp_interface_metrics.mRcpInterfaceType = kSpinelInterfaceTypeHdlc;
}

HdlcInterface::~HdlcInterface()
{
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
	struct k_msgq *msgq = (struct k_msgq *)user_data;

	while (uart_fifo_read(dev, &bt, sizeof(bt)) == sizeof(bt)) {
		if (k_msgq_put(msgq, &bt, K_NO_WAIT)) {
			LOG_ERR("OT Spinel UART data lost");
		}
	}
}

void HdlcInterface::handle_hdlc_frame(void *aContext, otError aError)
{
	HdlcInterface *interface = (HdlcInterface *)aContext;

	interface->m_rcp_interface_metrics.mTransferredFrameCount++;
	if (aError == OT_ERROR_NONE) {
		interface->m_rcp_interface_metrics.mRxFrameCount++;
		interface->m_rcp_interface_metrics.mRxFrameByteCount +=
			interface->m_receive_frame_buffer->GetLength();
		interface->m_rcp_interface_metrics.mTransferredValidFrameCount++;
		interface->m_receive_frame_callback(interface->m_receive_frame_context);
	} else {
		interface->m_rcp_interface_metrics.mTransferredGarbageFrameCount++;
		interface->m_receive_frame_buffer->DiscardFrame();
		LOG_WRN("OT Spinel error decoding HDLC frame: %s", otThreadErrorToString(aError));
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
		if (k_msgq_alloc_init(&m_msgq, sizeof(uint8_t), k_uart_buffer_size)) {
			LOG_ERR("OT Spinel can't allocate ISR buffer");
			break;
		}
		if (uart_irq_callback_user_data_set(m_uart_dev, serial_cb, &m_msgq)) {
			LOG_ERR("OT Spinel can't register ISR callback");
			(void)k_msgq_cleanup(&m_msgq);
			break;
		}
		uart_irq_rx_enable(m_uart_dev);
		m_uart_open = true;
	} while (0);

	if (m_uart_open) {
		m_hdlc_decoder.Init(aFrameBuffer, handle_hdlc_frame, this);
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
		k_msgq_purge(&m_msgq);
		(void)k_msgq_cleanup(&m_msgq);
		m_uart_open = false;
	}
	m_hdlc_decoder.Reset();
	m_receive_frame_callback = nullptr;
	m_receive_frame_context = nullptr;
	m_receive_frame_buffer = nullptr;
}

otError HdlcInterface::SendFrame(const uint8_t *aFrame, uint16_t aLength)
{
	otError result = OT_ERROR_FAILED;
	FrameBuffer<kMaxFrameSize> *tx_buf = new FrameBuffer<kMaxFrameSize>;

	do {
		if (!m_uart_open) {
			LOG_ERR("OT Spinel UART not opened");
			break;
		}
		if (!tx_buf) {
			LOG_ERR("OT Spinel can't allocate encoder buffer");
			break;
		}
		Hdlc::Encoder encoder(*tx_buf);

		if ((result = encoder.BeginFrame()) != OT_ERROR_NONE) {
			LOG_ERR("OT Spinel can't begin encode");
			break;
		}
		if ((result = encoder.Encode(aFrame, aLength)) != OT_ERROR_NONE) {
			LOG_ERR("OT Spinel can't encode");
			break;
		}
		if ((result = encoder.EndFrame()) != OT_ERROR_NONE) {
			LOG_ERR("OT Spinel can't finish encode");
			break;
		}
		uint8_t *data = tx_buf->GetFrame();
		uint16_t data_length = tx_buf->GetLength();

		for(uint16_t i = 0; i < data_length; i++) {
			uart_poll_out(m_uart_dev, data[i]);
		}
		if (IsSpinelResetCommand(aFrame, aLength)) {
			result = HardwareReset();
		}
	} while (0);

	if (tx_buf) {
		delete tx_buf;
	}
	m_rcp_interface_metrics.mTransferredFrameCount++;
	if (result == OT_ERROR_NONE) {
		m_rcp_interface_metrics.mTxFrameCount++;
		m_rcp_interface_metrics.mTxFrameByteCount += aLength;
		m_rcp_interface_metrics.mTransferredValidFrameCount++;
	} else {
		m_rcp_interface_metrics.mTransferredGarbageFrameCount++;
	}

	return result;
}

otError HdlcInterface::WaitForFrame(uint64_t aTimeoutUs)
{
	k_timepoint_t timepoint = sys_timepoint_calc(K_USEC(aTimeoutUs));
	otError result = OT_ERROR_FAILED;

	if (m_uart_open) {
		result = OT_ERROR_RESPONSE_TIMEOUT;
		uint64_t rx_frame_cnt = m_rcp_interface_metrics.mRxFrameCount;

		while (!sys_timepoint_expired(timepoint)) {
			uint8_t bt;
			if (!k_msgq_get(&m_msgq, &bt, sys_timepoint_timeout(timepoint))) {
				m_hdlc_decoder.Decode(&bt, sizeof(bt));
				result = OT_ERROR_NONE;
				if (m_rcp_interface_metrics.mRxFrameCount > rx_frame_cnt) {
					break;
				}
			}
		}
	} else {
		LOG_ERR("OT Spinel UART not opened");
	}

	return result;
}

void HdlcInterface::UpdateFdSet(void *aMainloopContext)
{
	// Nothing to do here for this platform
}

void HdlcInterface::Process(const void *aMainloopContext)
{
	// Nothing to do here for this platform
}

uint32_t HdlcInterface::GetBusSpeed(void) const
{
	uint32_t result = 0;

	if (m_uart_open) {
		struct uart_config cfg;

		if (!uart_config_get(m_uart_dev, &cfg)) {
			result = cfg.baudrate;
		}
	} else {
		LOG_ERR("OT Spinel UART not opened");
	}

	return result;
}

otError HdlcInterface::HardwareReset(void)
{
	otError result = OT_ERROR_FAILED;

	if (m_uart_open) {
		k_msgq_purge(&m_msgq);
		m_hdlc_decoder.Reset();
		result = OT_ERROR_NONE;
	} else {
		LOG_ERR("OT Spinel UART not opened");
	}

	return result;
}

const otRcpInterfaceMetrics *HdlcInterface::GetRcpInterfaceMetrics(void) const
{
	return &m_rcp_interface_metrics;
}

} // namespace Spinel
} // namespace ot
