/*
 * Copyright (c) 2025 Telink Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <hdlc_interface.hpp>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(test_main, LOG_LEVEL_INF);

int main(void)
{
	LOG_INF("main started");
	ot::Spinel::HdlcInterface hdlc;

	return 0;
}
