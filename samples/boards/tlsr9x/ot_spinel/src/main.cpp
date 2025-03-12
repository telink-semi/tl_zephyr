/*
 * Copyright (c) 2025 Telink Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <hdlc_interface.hpp>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(test_main, LOG_LEVEL_INF);

void on_rx(void *aContext) {
	LOG_INF("frame received");
}

int main(void)
{
	LOG_INF("main started");
	ot::Spinel::SpinelInterface *interface =
		new ot::Spinel::HdlcInterface(DEVICE_DT_GET_OR_NULL(DT_CHOSEN(zephyr_ot_uart)));

	ot::Spinel::SpinelInterface::RxFrameBuffer *rx_buf =
		new ot::Spinel::SpinelInterface::RxFrameBuffer;

	otError ret = interface->Init(on_rx, nullptr, *rx_buf);

	LOG_INF("hdlc.Init %d", ret);
	LOG_INF("hdlc.GetBusSpeed %u", interface->GetBusSpeed());

	uint8_t data[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};

	ret = interface->SendFrame(data, sizeof(data));
	LOG_INF("hdlc.SendFrame %d", ret);
	ret = interface->SendFrame(&data[1], sizeof(data) - 1);
	LOG_INF("hdlc.SendFrame %d", ret);

	delete interface;
	delete rx_buf;

	return 0;
}
