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
#include <zephyr/drivers/uart.h>

#include <ot_rcp/ot_rcp.h>

#define W91_ZB_MAC_ADDR_MAX_LENGTH 						8

struct w91_zb_config {
	const struct device *uart_dev;
	const char *uart_pins_str;
};

struct w91_zb_data {
	struct net_if *iface;
	struct openthread_rcp_data ot_rcp;
};

#if 0  /* TODO: just to suppress warning for now */
static void w91_zb_ack(uint8_t *data, size_t data_len, const void *ctx)
{
	const struct device *dev = (const struct device *)ctx;

	LOG_DBG("%s (%p)", __func__, dev);
}

static void w91_zb_rx(uint8_t *data, size_t data_len, const void *ctx)
{
	const struct device *dev = (const struct device *)ctx;

	LOG_DBG("%s (%p)", __func__, dev);
}
#endif

static void w91_zb_iface_init(struct net_if *iface)
{
	LOG_DBG("%s", __func__);
	const struct device *dev = net_if_get_device(iface);
	struct w91_zb_data *data = dev->data;

	data->iface = iface;

	/* get MAC address from RCP */
	static uint8_t mac[W91_ZB_MAC_ADDR_MAX_LENGTH];

	/* TODO: get IEEE EUI-64 RCP to mac */
	if (net_if_set_link_addr(data->iface, mac, sizeof(mac), NET_LINK_IEEE802154)) {
		LOG_ERR("set MAC failed");
	}
	ieee802154_init(data->iface);
}

static enum ieee802154_hw_caps w91_zb_get_capabilities(const struct device *dev)
{
	LOG_DBG("%s", __func__);
	enum ieee802154_hw_caps radio_caps = IEEE802154_HW_TX_RX_ACK;   /* required by Zephyr */

	return radio_caps;
}

static int w91_zb_cca(const struct device *dev)
{
	LOG_DBG("%s", __func__);
	return 0;
}

static int w91_zb_set_channel(const struct device *dev, uint16_t channel)
{
	LOG_DBG("%s", __func__);
	return 0;
}

static int w91_zb_filter(const struct device *dev, bool set,
	enum ieee802154_filter_type type, const struct ieee802154_filter *filter)
{
	LOG_DBG("%s", __func__);
	return 0;
}

static int w91_zb_set_txpower(const struct device *dev, int16_t dbm)
{
	LOG_DBG("%s", __func__);
	return 0;
}

static int w91_zb_tx(const struct device *dev, enum ieee802154_tx_mode mode,
	struct net_pkt *pkt, struct net_buf *frag)
{
	LOG_DBG("%s", __func__);
	return 0;
}

static int w91_zb_start(const struct device *dev)
{
	LOG_DBG("%s", __func__);
	return 0;
}

static int w91_zb_stop(const struct device *dev)
{
	LOG_DBG("%s", __func__);
	return 0;
}

static int w91_zb_continuous_carrier(const struct device *dev)
{
	LOG_DBG("%s", __func__);
	return 0;
}

static int w91_zb_configure(const struct device *dev, enum ieee802154_config_type type,
	const struct ieee802154_config *config)
{
	LOG_DBG("%s", __func__);
	return 0;
}

static int w91_zb_ed_scan(const struct device *dev, uint16_t duration,
	energy_scan_done_cb_t done_cb)
{
	LOG_DBG("%s", __func__);
	return 0;
}

static net_time_t w91_zb_get_time(const struct device *dev)
{
	LOG_DBG("%s", __func__);
	return 0;
}

static uint8_t w91_zb_get_sch_acc(const struct device *dev)
{
	LOG_DBG("%s", __func__);
	return 0;
}

static int w91_zb_attr_get(const struct device *dev, enum ieee802154_attr attr,
	struct ieee802154_attr_value *value)
{
	LOG_DBG("%s", __func__);
	return 0;
}

static int w91_zb_init(const struct device *dev)
{
	LOG_DBG("%s", __func__);

	int result = 0;
	const struct w91_zb_config *cfg = (const struct w91_zb_config *)dev->config;
	struct w91_zb_data *data = (struct w91_zb_data *)dev->data;

	do {
		if (!device_is_ready(cfg->uart_dev)) {
			LOG_ERR("spinel serial not ready");
			result = -EIO;
			break;
		}
		LOG_INF("spinel on %s", cfg->uart_dev->name);
		struct uart_config uart_cfg;

		if (!uart_config_get(cfg->uart_dev, &uart_cfg)) {
			static const char *uart_data_bits_str[] = {"5", "6", "7", "8", "9"};
			static const char *uart_parity_bits_str[] = {"none", "odd", "even", "mark", "space"};
			static const char *uart_stop_bits_str[] = {"0.5", "1", "1.5", "2"};
			static const char *uart_flow_ctrl_str[] = {"none", "rts-cts", "dtr-dsr", "rs-485"};

			LOG_INF("uart: %u %s %s %s %s", uart_cfg.baudrate,
				uart_cfg.data_bits < ARRAY_SIZE(uart_data_bits_str) ?
					uart_data_bits_str[uart_cfg.data_bits] : "invalid",
				uart_cfg.parity < ARRAY_SIZE(uart_parity_bits_str) ?
					uart_parity_bits_str[uart_cfg.parity] : "invalid",
				uart_cfg.stop_bits < ARRAY_SIZE(uart_stop_bits_str) ?
					uart_stop_bits_str[uart_cfg.stop_bits] : "invalid",
				uart_cfg.flow_ctrl < ARRAY_SIZE(uart_flow_ctrl_str) ?
					uart_flow_ctrl_str[uart_cfg.flow_ctrl] : "invalid");
			LOG_INF("pins: %s", cfg->uart_pins_str);
		} else {
			LOG_ERR("spinel serial config fail");
			result = -EIO;
			break;
		}

		if (openthread_rcp_init(&data->ot_rcp, cfg->uart_dev, dev)) {
			LOG_ERR("spinel init fail");
			result = -EIO;
			break;
		}
		if (openthread_rcp_reset(&data->ot_rcp)) {
			LOG_ERR("rcp reset fail");
			result = -EIO;
			break;
		}
	} while (0);

	return result;
}

static const struct ieee802154_radio_api w91_zb_drv_api = {
	.iface_api.init = w91_zb_iface_init,
	.get_capabilities = w91_zb_get_capabilities,
	.cca = w91_zb_cca,
	.set_channel = w91_zb_set_channel,
	.filter = w91_zb_filter,
	.set_txpower = w91_zb_set_txpower,
	.tx = w91_zb_tx,
	.start = w91_zb_start,
	.stop = w91_zb_stop,
	.continuous_carrier = w91_zb_continuous_carrier,
	.configure = w91_zb_configure,
	.ed_scan = w91_zb_ed_scan,
	.get_time = w91_zb_get_time,
	.get_sch_acc = w91_zb_get_sch_acc,
	.attr_get = w91_zb_attr_get,
};

#define DT_PROP_BY_IDX_DT_NODE_FULL_NAME(node_id, prop, idx)    \
	DT_NODE_FULL_NAME(DT_PROP_BY_IDX(node_id, prop, idx))

#if CONFIG_NET_L2_IEEE802154 || CONFIG_NET_L2_OPENTHREAD

#if CONFIG_NET_L2_IEEE802154 && CONFIG_NET_L2_OPENTHREAD
#error Networks IEEE802.15.4 & openthread are not supported at the same time
#elif CONFIG_NET_L2_IEEE802154
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
		.uart_dev = DEVICE_DT_GET(DT_INST_PROP(n, serial)),     \
		.uart_pins_str = "[" DT_FOREACH_PROP_ELEM_SEP(          \
			DT_INST_PROP(n, serial), pinctrl_0,                 \
				DT_PROP_BY_IDX_DT_NODE_FULL_NAME, (", ")) "]"   \
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
		.uart_dev = DEVICE_DT_GET(DT_INST_PROP(n, serial)),     \
		.uart_pins_str = "[" DT_FOREACH_PROP_ELEM_SEP(          \
			DT_INST_PROP(n, serial), pinctrl_0,                 \
				DT_PROP_BY_IDX_DT_NODE_FULL_NAME, (", ")) "]"   \
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
