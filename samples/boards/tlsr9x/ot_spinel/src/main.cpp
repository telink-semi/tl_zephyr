/*
 * Copyright (c) 2025 Telink Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

// #define TEST_INTERFACE

#include <zephyr/kernel.h>
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

	LOG_INF("radio channel: %u", spinel_radio_interface_get_channel());

	otError err = spinel_radio_interface_energy_scan(16, 10000);

	if (err != OT_ERROR_NONE) {
		LOG_ERR("energy scan failed: %s", otThreadErrorToString(err));
	} else {
		LOG_INF("energy scan done");
	}

	err = spinel_radio_interface_set_cca_energy_detect_threshold(-60);
	if (err != OT_ERROR_NONE) {
		LOG_ERR("set cca energy detect threshold failed: %s", otThreadErrorToString(err));
	} else {
		LOG_INF("set cca energy detect threshold done");
	}

	LOG_INF("RSSI: %u", spinel_radio_interface_get_rssi());

	LOG_INF("radio is %s", spinel_radio_interface_radio_is_enabled() ? "enabled" : "disabled");

#if 0
	err = spinel_radio_interface_disable();
	if (err != OT_ERROR_NONE) {
		LOG_ERR("disable failed: %s", otThreadErrorToString(err));
	} else {
		LOG_INF("disable done");
	}
#endif

	err = spinel_radio_interface_set_ch_max_transmit_power(16, 4);
	if (err != OT_ERROR_NONE) {
		LOG_ERR("set channel max transmit power: %s", otThreadErrorToString(err));
	} else {
		LOG_INF("set channel max transmit power done");
	}

	err = spinel_radio_interface_set_transmit_power(2);
	if (err != OT_ERROR_NONE) {
		LOG_ERR("set transmit power: %s", otThreadErrorToString(err));
	} else {
		LOG_INF("set transmit power done");
	}

	int8_t power;
	err = spinel_radio_interface_get_transmit_power(&power);
	if (err != OT_ERROR_NONE) {
		LOG_ERR("get transmit power: %s", otThreadErrorToString(err));
	} else {
		LOG_INF("get transmit power done %d", power);
	}

	err = spinel_radio_interface_enable(nullptr);
	if (err != OT_ERROR_NONE) {
		LOG_ERR("enable failed: %s", otThreadErrorToString(err));
	} else {
		LOG_INF("enable done");
	}

	LOG_INF("radio is %s", spinel_radio_interface_radio_is_enabled() ? "enabled" : "disabled");

	err = spinel_radio_interface_set_pan_id(0x1418);

	if (err != OT_ERROR_NONE) {
		LOG_ERR("set panid failed: %s", otThreadErrorToString(err));
	} else {
		LOG_INF("set panid done");
	}

	err = spinel_radio_interface_set_short_address(0x5555);

	if (err != OT_ERROR_NONE) {
		LOG_ERR("set short address failed: %s", otThreadErrorToString(err));
	} else {
		LOG_INF("set short address done");
	}

	otExtAddress ext_addr = {
		.m8 = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08}
	};

	err = spinel_radio_interface_set_extended_address(&ext_addr);

	if (err != OT_ERROR_NONE) {
		LOG_ERR("set extended address failed: %s", otThreadErrorToString(err));
	} else {
		LOG_INF("set extended address done");
	}

	err = spinel_radio_interface_receive(16);
	if (err != OT_ERROR_NONE) {
		LOG_ERR("receive failed: %s", otThreadErrorToString(err));
	} else {
		LOG_INF("receive done");
	}

	uint8_t data[] = {0x02, 0x00, 0x0a};
	otRadioFrame frame = {
		.mPsdu = data,
		.mLength = sizeof(data),
		.mChannel = 16,
		.mInfo = {
			.mTxInfo = {
				.mAesKey = nullptr,
				.mIeInfo = nullptr,
				.mTxDelayBaseTime = 0,
				.mTxDelay = 0,
				.mMaxCsmaBackoffs = 1,
				.mMaxFrameRetries = 1,
				.mRxChannelAfterTxDone = 16,
				.mIsHeaderUpdated = true,
				.mIsARetx = false,
				.mCsmaCaEnabled = false,
				.mCslPresent = false,
				.mIsSecurityProcessed = true,
			},
		},
	};

	err = spinel_radio_interface_transmit(&frame);
	if (err != OT_ERROR_NONE) {
		LOG_ERR("transmit failed: %s", otThreadErrorToString(err));
	} else {
		LOG_INF("transmit started");
	}

	if (spinel_radio_interface_is_transmitting())
	{
		while (!spinel_radio_interface_is_transmit_done()) {
			k_msleep(5);
		}
	}

	spinel_radio_interface_deinit();
	LOG_INF("main finished");
	return 0;
}

#endif // TEST_INTERFACE
