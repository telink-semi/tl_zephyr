/*
 * Copyright (c) 2025 Telink
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/init.h>
#include <zephyr/net/net_config.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(net_main, LOG_LEVEL_DBG);

int main(void)
{
	LOG_INF("***** Network CLI on Zephyr (%s) *****", net_if_get_default()->config.name);

	return 0;
}

#if !CONFIG_NET_CONFIG_AUTO_INIT

static int init_application(void)
{
	const char *app_info = "Initializing network";

	STRUCT_SECTION_FOREACH(net_if, iface) {
		if (net_if_is_up(iface)) {
			(void)net_config_init_app(net_if_get_device(iface), app_info);
		} else {
			LOG_INF("interface: %s is down\n", iface->config.name);
		}
	}
	return 0;
}

SYS_INIT(init_application, APPLICATION, CONFIG_NET_CONFIG_INIT_PRIO);

#endif /* !CONFIG_NET_CONFIG_AUTO_INIT */
