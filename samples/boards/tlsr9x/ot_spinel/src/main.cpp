/*
 * Copyright (c) 2025 Telink Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

// #define TEST_INTERFACE

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(test_main, LOG_LEVEL_INF);

#ifdef TEST_INTERFACE

#include <spinel_manager.hpp>
using namespace ot::Spinel;

void on_rx(void *aContext) {
	SpinelInterface::RxFrameBuffer *buf =
		static_cast<SpinelInterface::RxFrameBuffer *>(aContext);

	LOG_HEXDUMP_INF(buf->GetFrame(), buf->GetLength(), "received");
	buf->DiscardFrame();
}

int main(void)
{
	LOG_INF("main started");

	SpinelInterface::RxFrameBuffer *rx_buf =
		new SpinelInterface::RxFrameBuffer;

	otError ret =
		SpinelManager::GetInstance()->GetSpinelInterface().Init(on_rx, rx_buf, *rx_buf);
	LOG_INF("hdlc.Init %d", ret);
	LOG_INF("hdlc.GetBusSpeed %u",
		SpinelManager::GetInstance()->GetSpinelInterface().GetBusSpeed());

	uint8_t data[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};

	ret = SpinelManager::GetInstance()->GetSpinelInterface().SendFrame(data, sizeof(data));
	LOG_INF("hdlc.SendFrame %d", ret);
	ret = SpinelManager::GetInstance()->GetSpinelInterface().SendFrame(&data[1], sizeof(data) - 1);
	LOG_INF("hdlc.SendFrame %d", ret);

	ret = SpinelManager::GetInstance()->GetSpinelInterface().WaitForFrame(1 * 1000 * 1000);
	LOG_INF("hdlc.WaitForFrame %d", ret);
	ret = SpinelManager::GetInstance()->GetSpinelInterface().WaitForFrame(1 * 1000 * 1000);
	LOG_INF("hdlc.WaitForFrame %d", ret);

	const otRcpInterfaceMetrics *metrics =
		SpinelManager::GetInstance()->GetSpinelInterface().GetRcpInterfaceMetrics();

	LOG_INF("The RCP interface type: %u", metrics->mRcpInterfaceType);
    LOG_INF("The number of transferred frames: %llu", metrics->mTransferredFrameCount);
    LOG_INF("The number of transferred valid frames: %llu", metrics->mTransferredValidFrameCount);
    LOG_INF("The number of transferred garbage frames: %llu", metrics->mTransferredGarbageFrameCount);
    LOG_INF("The number of received frames: %llu", metrics->mRxFrameCount);
    LOG_INF("The number of received bytes: %llu", metrics->mRxFrameByteCount);
    LOG_INF("The number of transmitted frames: %llu", metrics->mTxFrameCount);
    LOG_INF("The number of transmitted bytes: %llu", metrics->mTxFrameByteCount);

    SpinelManager::GetInstance()->DestroyInstance();

	delete rx_buf;

	return 0;
}

#else

#include <spinel_radio_interface.h>

static void radio_caps_show(otRadioCaps radio_caps)
{
	LOG_INF("radio capabilities:");
	if (radio_caps & OT_RADIO_CAPS_ACK_TIMEOUT) {
		LOG_INF("ACK time event");
	}
	if (radio_caps & OT_RADIO_CAPS_ENERGY_SCAN) {
		LOG_INF("energy scans");
	}
	if (radio_caps & OT_RADIO_CAPS_TRANSMIT_RETRIES) {
		LOG_INF("TX retry logic with collision avoidance (CSMA)");
	}
	if (radio_caps & OT_RADIO_CAPS_CSMA_BACKOFF) {
		LOG_INF("CSMA backoff for frame transmission (but no retry)");
	}
	if (radio_caps & OT_RADIO_CAPS_SLEEP_TO_TX) {
		LOG_INF("direct transition from sleep to TX with CSMA");
	}
	if (radio_caps & OT_RADIO_CAPS_TRANSMIT_SEC) {
		LOG_INF("TX security");
	}
	if (radio_caps & OT_RADIO_CAPS_TRANSMIT_TIMING) {
		LOG_INF("TX at specific time");
	}
	if (radio_caps & OT_RADIO_CAPS_RECEIVE_TIMING) {
		LOG_INF("RX at specific time");
	}
	if (radio_caps & OT_RADIO_CAPS_RX_ON_WHEN_IDLE) {
		LOG_INF("RX on when idle handling");
	}
	LOG_INF("radio capabilities end.");
}

static const char *radio_state_string(otRadioState state)
{
	static const char *radio_state_str[] = {
		"DISABLED", "SLEEP", "RECEIVE", "TRANSMIT", "INVALID" };

	if (state <= OT_RADIO_STATE_TRANSMIT) {
		return radio_state_str[state];
	} else {
		return radio_state_str[ARRAY_SIZE(radio_state_str) - 1];
	}
}

int main(void)
{
	LOG_INF("main started");
	spinel_radio_interface_init();
	LOG_INF("spinel radio inited");
	LOG_INF("RCP bus speed: %u bps", spinel_radio_interface_get_bus_speed());
	LOG_INF("RCP version: %s", spinel_radio_interface_get_version());
	radio_caps_show(spinel_radio_interface_get_radio_caps());
	LOG_INF("radio state: %s", radio_state_string(spinel_radio_interface_get_state()));

	uint8_t ieee_eui64[8];

	if (spinel_radio_interface_get_ieee_eui64(ieee_eui64) == OT_ERROR_NONE) {
		LOG_HEXDUMP_INF(ieee_eui64, sizeof(ieee_eui64), "IEEE EUI-64");
	} else {
		LOG_ERR("read IEEE EUI-64 failed");
	}

	spinel_radio_interface_deinit();
	LOG_INF("main finished");
	return 0;
}

#endif // TEST_INTERFACE
