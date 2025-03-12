/*
 * Copyright (c) 2025 Telink Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT telink_w91_zb

#define LOG_MODULE_NAME ieee802154_w91
#ifdef CONFIG_IEEE802154_DRIVER_LOG_LEVEL
#define LOG_LEVEL CONFIG_IEEE802154_DRIVER_LOG_LEVEL
#else
#define LOG_LEVEL LOG_LEVEL_NONE
#endif /* CONFIG_IEEE802154_DRIVER_LOG_LEVEL */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

#include <zephyr/net/ieee802154_radio.h>
#include <zephyr/net/net_l2.h>
#include <zephyr/net/openthread.h>

struct w91_zb_config {
};

struct w91_zb_data {
};

static int w91_zb_init(const struct device *dev)
{
	LOG_INF("%s", __func__);
	return 0;
}

static const struct ieee802154_radio_api w91_zb_drv_api = {
};

#if CONFIG_NET_L2_IEEE802154 || CONFIG_NET_L2_OPENTHREAD

#if CONFIG_NET_L2_IEEE802154
#define W91_ZB_L2 IEEE802154_L2
#define W91_ZB_L2_CTX_TYPE NET_L2_GET_CTX_TYPE(IEEE802154_L2)
#define W91_ZB_MTU 125
#elif CONFIG_NET_L2_OPENTHREAD
#define W91_ZB_L2 OPENTHREAD_L2
#define W91_ZB_L2_CTX_TYPE NET_L2_GET_CTX_TYPE(OPENTHREAD_L2)
#define W91_ZB_MTU 1280
#endif /* net L2 select */

#define W91_ZB_DEFINE(n)                                        \
                                                                \
	static const struct w91_zb_config w91_zb_config_##n = {     \
	};                                                          \
                                                                \
	static struct w91_zb_data w91_zb_data_##n;                  \
                                                                \
	NET_DEVICE_DT_INST_DEFINE(n, w91_zb_init,                   \
		NULL, &w91_zb_data_##n, &w91_zb_config_##n,             \
		UTIL_INC(CONFIG_TELINK_W91_IPC_DRIVERS_INIT_PRIORITY),  \
		&w91_zb_drv_api, W91_ZB_L2,                             \
		W91_ZB_L2_CTX_TYPE, W91_ZB_MTU);

#else

#define W91_ZB_DEFINE(n)                                        \
                                                                \
	static const struct w91_zb_config w91_zb_config_##n = {     \
	};                                                          \
                                                                \
	static struct w91_zb_data w91_zb_data_##n;                  \
                                                                \
	DEVICE_DT_INST_DEFINE(n, w91_zb_init,                       \
		NULL, &w91_zb_data_##n, &w91_zb_config_##n,             \
		POST_KERNEL,                                            \
		UTIL_INC(CONFIG_TELINK_W91_IPC_DRIVERS_INIT_PRIORITY),  \
		&w91_zb_drv_api);

#endif /* CONFIG_NET_L2_IEEE802154 || CONFIG_NET_L2_OPENTHREAD */

DT_INST_FOREACH_STATUS_OKAY(W91_ZB_DEFINE)
