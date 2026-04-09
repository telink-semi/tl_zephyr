/*
 * Copyright (c) 2026 Telink Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(main, LOG_LEVEL_DBG);

#include <zephyr/input/input.h>

static void input_callback(struct input_event *evt)
{
	LOG_INF("%s key (%u %u) %s", evt->dev->name, (evt->code >> 8) & 0xff, evt->code & 0xff,
		evt->value ? "pressed" : "released");
}

INPUT_LISTENER_CB_DEFINE(DEVICE_DT_GET(DT_NODELABEL(tlx_kscan)), input_callback);

int main(void)
{
	LOG_INF("press any key");
	return 0;
}
