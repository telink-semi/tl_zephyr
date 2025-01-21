/*
 * Copyright (c) 2025 Telink Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef OT_HDLC_INTERFACE_HPP_
#define OT_HDLC_INTERFACE_HPP_

#include <zephyr/kernel.h>

#include <lib/spinel/openthread-spinel-config.h>
#include <lib/spinel/spinel_interface.hpp>
#include <lib/hdlc/hdlc.hpp>

namespace ot {
namespace Spinel {

class HdlcInterface : public SpinelInterface
{
public:
	explicit HdlcInterface(const struct device *const uart_dev);
	~HdlcInterface() override;

	otError Init(ReceiveFrameCallback aCallback,
		void *aCallbackContext, RxFrameBuffer &aFrameBuffer) override;
	void Deinit(void) override;
	otError SendFrame(const uint8_t *aFrame, uint16_t aLength) override;
	otError WaitForFrame(uint64_t aTimeoutUs) override;
	void UpdateFdSet(void *aMainloopContext) override;
	void Process(const void *aMainloopContext) override;
	uint32_t GetBusSpeed(void) const override;
	otError HardwareReset(void) override;
	const otRcpInterfaceMetrics *GetRcpInterfaceMetrics(void) const override;

private:
	static const size_t  k_uart_buffer_size;

	static void serial_cb(const struct device *dev, void *user_data);
	static void handle_hdlc_frame(void *aContext, otError aError);

	bool                            m_uart_open;
	const struct device *const      m_uart_dev;
	struct k_msgq                  *m_msgq;
	Hdlc::Decoder                   m_hdlc_decoder;
	ReceiveFrameCallback            m_receive_frame_callback;
	void                           *m_receive_frame_context;
	RxFrameBuffer                  *m_receive_frame_buffer;
	otRcpInterfaceMetrics           m_rcp_interface_metrics;

	HdlcInterface(const HdlcInterface &) = delete;
	HdlcInterface &operator=(const HdlcInterface &) = delete;
};

} // namespace Spinel
} // namespace ot

#endif // OT_HDLC_INTERFACE_HPP_
