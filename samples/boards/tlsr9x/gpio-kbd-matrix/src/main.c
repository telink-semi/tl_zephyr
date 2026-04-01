/*
 * Copyright (c) 2026 Telink Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(main, LOG_LEVEL_DBG);

#include <zephyr/input/input.h>

static void input_callback(struct input_event *evt, void *user_data)
{
	LOG_INF("key code %u %s", evt->code, evt->value ? "pressed" : "released");
}

INPUT_CALLBACK_DEFINE(DEVICE_DT_GET(DT_NODELABEL(kbd_keymap)), input_callback, NULL);

int main(void)
{
	LOG_INF("press any key");
}
