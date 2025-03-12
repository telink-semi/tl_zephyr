/*
 * Copyright (c) 2025 Telink Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

// #define TEST_INTERFACE
// #define TEST_RCP_EXCHANGE

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(test_main, LOG_LEVEL_INF);

#if defined(TEST_INTERFACE)

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

#elif defined(TEST_RCP_EXCHANGE)

#include <spinel_radio_interface.h>

static void radio_caps_show(otRadioCaps radio_caps)
{
	static const char *const radio_caps_str[] = {
		"ack-timeout", "energy-scan", "tx-retries", "CSMA-backoff",
		"sleep-to-tx", "tx-security", "tx-timing", "rx-timing",
		"rx-on-when-idle", "tx-frame-power"
	};

	LOG_INF("rcp capabilities:");
	for (size_t i = 0; i < ARRAY_SIZE(radio_caps_str); i++) {
		if (radio_caps & (1 << i)) {
			LOG_INF("%s", radio_caps_str[i]);
		}
	}
	LOG_INF("rcp capabilities end");
}

static const char *radio_state_string(otRadioState state)
{
	static const char *const radio_state_str[] = {
		"disabled", "sleep", "receive", "transmit", "invalid" };

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

	LOG_INF("enable %s", otThreadErrorToString(spinel_radio_interface_enable(nullptr)));
	LOG_INF("sleep %s", otThreadErrorToString(spinel_radio_interface_sleep()));

	LOG_INF("clear src match short entries %s",
		otThreadErrorToString(spinel_radio_interface_clear_src_match_short_entries()));
	LOG_INF("clear src match ext entries %s",
		otThreadErrorToString(spinel_radio_interface_clear_src_match_ext_entries()));

	LOG_INF("set short address %s",
		otThreadErrorToString(spinel_radio_interface_set_short_address(0xfffe)));

	const otExtAddress ext_addr = {
		.m8 = (uint8_t []){0x6e, 0x65, 0xef, 0x8e, 0xf5, 0x2d, 0x27, 0x8a}
	};

	LOG_INF("set ext address %s",
		otThreadErrorToString(spinel_radio_interface_set_extended_address(&ext_addr)));
	LOG_INF("set pan id %s",
		otThreadErrorToString(spinel_radio_interface_set_pan_id(0x1418)));

	LOG_INF("receive %s", otThreadErrorToString(spinel_radio_interface_receive(18)));
#if 0
	for (;;) {
		k_msleep(1000);
		LOG_INF("rx");
	}
#endif
	uint8_t frame_data[] = {
		0x61, 0xd8, 0x6d, 0x18, 0x14, 0xff, 0xff, 0xd6,
		0x82, 0x7b, 0x1b, 0xba, 0x23, 0x43, 0x22, 0x7f,
		0x3b, 0x02, 0xf0, 0x4d, 0x4c, 0x4d, 0x4c, 0xa0,
		0x63, 0x00, 0x15, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x01, 0xc9, 0xd1, 0x22, 0x70,
		0xfc, 0xad, 0xe9, 0xe2, 0xb6, 0x35, 0x60, 0x98,
		0xa5, 0x79, 0x8f, 0xa7, 0xe8, 0xe3, 0x97, 0x31,
		0x14, 0xce, 0x2b, 0x3d, 0xd2
	};
	otRadioFrame frame = {
		.mPsdu = frame_data,
		.mLength = sizeof(frame_data),
		.mChannel = 18,
		.mInfo = {
			.mTxInfo = {
				.mAesKey = nullptr,
				.mIeInfo = nullptr,
				.mTxDelayBaseTime = 0,
				.mTxDelay = 0,
				.mMaxCsmaBackoffs = 4,
				.mMaxFrameRetries = 15,
				.mRxChannelAfterTxDone = 18,
				.mIsHeaderUpdated = false,
				.mIsARetx = false,
				.mCsmaCaEnabled = true,
				.mCslPresent = false,
				.mIsSecurityProcessed = false
			}
		}
	};

	LOG_INF("transmit %s",
		otThreadErrorToString(spinel_radio_interface_transmit(&frame)));

	while(spinel_radio_interface_is_transmitting()) {
		k_msleep(10);
	}

	for (;;) {
		k_msleep(10);
	}

	spinel_radio_interface_deinit();

	LOG_INF("main finished");
	return 0;
}

#else

int main(void)
{
	LOG_INF("main started");
	return 0;
}

#endif // app type
