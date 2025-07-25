/*
 * Copyright (c) 2025 Telink Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT telink_rcp_zb

#define LOG_MODULE_NAME ieee802154_telink_rcp
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
#include <zephyr/drivers/timer/system_timer.h>

#include <openthread/platform/radio.h>

#include <ot_rcp/ot_rcp.h>

#define TELINK_RCP_ZB_MAC_ADDR_MAX_LENGTH 8
#define TELINK_RCP_ZB_RADIO_CAPS_VERBOSE  0
#define TELINK_RCP_ZB_FCS_SIZE            2
#define TELINK_RCP_ZB_EXT_ADDR_LENGTH     8

IEEE802154_DEFINE_PHY_SUPPORTED_CHANNELS(telink_rcp_zb_drv_attr, 11, 26);

struct telink_rcp_zb_config {
	const struct device *uart_dev;
	const char *uart_pins_str;
};

struct telink_rcp_zb_data {
	struct net_if *iface;
	struct openthread_rcp_data ot_rcp;
	ieee802154_event_cb_t event_handler;
	enum ieee802154_hw_caps radio_caps;
	bool reception_on;
	uint8_t channel;
	int8_t tx_power;
};

static void telink_rcp_zb_rx(const struct spinel_frame_data *frame, const void *ctx)
{
	const struct device *dev = (const struct device *)ctx;
	struct telink_rcp_zb_data *data = dev->data;
	bool net_data_received = false;
	struct net_pkt *rx_pkt = net_pkt_rx_alloc_with_buffer(data->iface, frame->data_length,
							      AF_UNSPEC, 0, K_NO_WAIT);

	do {
		if (!rx_pkt) {
			if (data->event_handler) {
				data->event_handler(dev, IEEE802154_EVENT_RX_FAILED,
						    (void *)&((enum ieee802154_rx_fail_reason){
							    IEEE802154_RX_FAIL_NOT_RECEIVED}));
			}
			LOG_ERR("can't allocate rx packet");
			break;
		}
		if (net_pkt_write(rx_pkt, frame->data, frame->data_length) < 0) {
			if (data->event_handler) {
				data->event_handler(dev, IEEE802154_EVENT_RX_FAILED,
						    (void *)&((enum ieee802154_rx_fail_reason){
							    IEEE802154_RX_FAIL_NOT_RECEIVED}));
			}
			LOG_ERR("Failed to write rx packet.");
			break;
		}
#if defined(CONFIG_NET_PKT_TIMESTAMP)
		int64_t time_offset;

		openthread_rcp_get_time_offset(&data->ot_rcp, &time_offset);
		net_pkt_set_timestamp_ns(rx_pkt,
					 (frame->rx.timestamp - time_offset) * NSEC_PER_USEC);
#endif /* CONFIG_NET_PKT_TIMESTAMP */
		net_pkt_set_ieee802154_rssi_dbm(rx_pkt, frame->rx.rssi);
		net_pkt_set_ieee802154_lqi(rx_pkt, frame->rx.lqi);
		net_pkt_set_ieee802154_ack_fpb(rx_pkt, frame->rx.frame_pending);
#if defined(CONFIG_NET_L2_OPENTHREAD)
		net_pkt_set_ieee802154_ack_seb(rx_pkt, frame->rx.sec_enh_ack);
#endif
		net_pkt_cursor_init(rx_pkt);
		if (net_recv_data(data->iface, rx_pkt) < 0) {
			if (data->event_handler) {
				data->event_handler(dev, IEEE802154_EVENT_RX_FAILED,
						    (void *)&((enum ieee802154_rx_fail_reason){
							    IEEE802154_RX_FAIL_NOT_RECEIVED}));
			}
			LOG_INF("rx packet not handled");
			break;
		}
		net_data_received = true;
	} while (0);

	if (!net_data_received && rx_pkt) {
		net_pkt_unref(rx_pkt);
	}
}

static void telink_rcp_zb_iface_init(struct net_if *iface)
{
	LOG_DBG("%s", __func__);
	const struct device *dev = net_if_get_device(iface);
	struct telink_rcp_zb_data *data = dev->data;

	data->iface = iface;

	static uint8_t mac[TELINK_RCP_ZB_MAC_ADDR_MAX_LENGTH];

	if (openthread_rcp_ieee_eui64(&data->ot_rcp, mac)) {
		LOG_ERR("read mac failed");
	}
	if (net_if_set_link_addr(data->iface, mac, sizeof(mac), NET_LINK_IEEE802154)) {
		LOG_ERR("set MAC failed");
	}
	if (openthread_rcp_enable(&data->ot_rcp, true)) {
		LOG_ERR("rcp enabling failed");
	}
	ieee802154_init(data->iface);
}

static enum ieee802154_hw_caps telink_rcp_zb_get_capabilities(const struct device *dev)
{
	LOG_DBG("%s", __func__);
	struct telink_rcp_zb_data *data = dev->data;

	if (!data->radio_caps) {
		if (openthread_rcp_capabilities(&data->ot_rcp, &data->radio_caps)) {
			LOG_ERR("read capabilities failed");
		}
#if TELINK_RCP_ZB_RADIO_CAPS_VERBOSE
		static const char *const radio_caps_str[] = {"energy scan",
							     "fcs verification",
							     "hw filter",
							     "promiscuous",
							     "tx csma-ca procedure",
							     "tx rx ack",
							     "tx retransmission",
							     "rx tx ack",
							     "tx time",
							     "tx from sleep",
							     "rx time",
							     "tx security",
							     "rx on when idle"};

		for (size_t i = 0; i < ARRAY_SIZE(radio_caps_str); i++) {
			if (data->radio_caps & BIT(i)) {
				LOG_INF("radio supports: %s", radio_caps_str[i]);
			}
		}
#endif /* TELINK_RCP_ZB_RADIO_CAPS_VERBOSE */
	}
	return data->radio_caps;
}

static int telink_rcp_zb_cca(const struct device *dev)
{
	LOG_WRN("%s not supported", __func__);
	return -ENOTSUP;
}

static int telink_rcp_zb_set_channel(const struct device *dev, uint16_t channel)
{
	LOG_DBG("%s", __func__);
	int result = 0;
	struct telink_rcp_zb_data *data = dev->data;

	channel = MIN(channel, UINT8_MAX);
	if (data->channel != (uint8_t)channel) {
		if (data->reception_on) {
			result = openthread_rcp_channel(&data->ot_rcp, (uint8_t)channel);
			if (!result) {
				data->channel = (uint8_t)channel;
			}
		} else {
			data->channel = (uint8_t)channel;
		}
	}
	return result;
}

static int telink_rcp_zb_filter(const struct device *dev, bool set,
				enum ieee802154_filter_type type,
				const struct ieee802154_filter *filter)
{
	LOG_DBG("%s", __func__);
	int result = -ENOTSUP;
	struct telink_rcp_zb_data *data = dev->data;

	if (set) {
		switch (type) {
		case IEEE802154_FILTER_TYPE_PAN_ID:
			result = openthread_rcp_panid(&data->ot_rcp, filter->pan_id);
			break;
		case IEEE802154_FILTER_TYPE_SHORT_ADDR:
			result = openthread_rcp_short_addr(&data->ot_rcp, filter->short_addr);
			break;
		case IEEE802154_FILTER_TYPE_IEEE_ADDR: {
			uint8_t address[TELINK_RCP_ZB_EXT_ADDR_LENGTH];

			sys_memcpy_swap(address, filter->ieee_addr, sizeof(address));
			result = openthread_rcp_ext_addr(&data->ot_rcp, address);
		} break;
		default:
			LOG_WRN("unhandled filter %u", type);
			break;
		}
	}
	return result;
}

static int telink_rcp_zb_set_txpower(const struct device *dev, int16_t dbm)
{
	LOG_DBG("%s", __func__);
	int result = 0;
	struct telink_rcp_zb_data *data = dev->data;

	dbm = CLAMP(dbm, INT8_MIN, INT8_MAX);
	if (data->tx_power != dbm) {
		result = openthread_rcp_tx_power(&data->ot_rcp, (int8_t)dbm);
		if (!result) {
			data->tx_power = dbm;
		}
	}
	return result;
}

static int telink_rcp_zb_tx(const struct device *dev, enum ieee802154_tx_mode mode,
			    struct net_pkt *pkt, struct net_buf *frag)
{
	LOG_DBG("%s", __func__);
	int result = 0;
	struct telink_rcp_zb_data *data = dev->data;
	struct spinel_frame_data frame = {
		.data = frag->data,
		.data_length = frag->len + TELINK_RCP_ZB_FCS_SIZE,
		.channel = data->channel,
		.tx.time_base = 0,
		.tx.time_offset = 0,
		.tx.header_updated = net_pkt_ieee802154_mac_hdr_rdy(pkt),
		.tx.security_processed = net_pkt_ieee802154_frame_secured(pkt),
		.tx.is_ret = false,
		.tx.csma_ca_enabled = (mode == IEEE802154_TX_MODE_CSMA_CA)};

#if defined(CONFIG_NET_PKT_TXTIME)
	if (mode == IEEE802154_TX_MODE_TXTIME_CCA) {
		int64_t time_offset;
		uint64_t host_time_base_us = k_cyc_to_us_floor64(sys_clock_cycle_get_64());
		uint64_t set_timestamp_us = net_pkt_timestamp_ns(pkt) / NSEC_PER_USEC;

		openthread_rcp_get_time_offset(&data->ot_rcp, &time_offset);
		frame.tx.time_base = (uint32_t)(host_time_base_us + time_offset);
		frame.tx.time_offset = (uint32_t)(set_timestamp_us - host_time_base_us);
	}
#endif /* CONFIG_NET_PKT_TXTIME */

	if (data->event_handler) {
		data->event_handler(dev, IEEE802154_EVENT_TX_STARTED, NULL);
	}
	result = openthread_rcp_transmit(&data->ot_rcp, &frame);
	if (!result && frame.data && frame.data_length) {
		struct net_pkt *ack_pkt = net_pkt_rx_alloc_with_buffer(
			data->iface, frame.data_length, AF_UNSPEC, 0, K_NO_WAIT);

		do {
			if (!ack_pkt) {
				if (data->event_handler) {
					data->event_handler(
						dev, IEEE802154_EVENT_RX_FAILED,
						(void *)&((enum ieee802154_rx_fail_reason){
							IEEE802154_RX_FAIL_NOT_RECEIVED}));
				}
				LOG_ERR("can't allocate ack packet");
				result = -ENOMEM;
				break;
			}
			result = net_pkt_write(ack_pkt, frame.data, frame.data_length);
			if (result < 0) {
				if (data->event_handler) {
					data->event_handler(
						dev, IEEE802154_EVENT_RX_FAILED,
						(void *)&((enum ieee802154_rx_fail_reason){
							IEEE802154_RX_FAIL_NOT_RECEIVED}));
				}
				LOG_ERR("Failed to write ack packet.");
				break;
			}
#if defined(CONFIG_NET_PKT_TIMESTAMP)
			int64_t time_offset;

			openthread_rcp_get_time_offset(&data->ot_rcp, &time_offset);
			net_pkt_set_timestamp_ns(ack_pkt, (frame.rx.timestamp - time_offset) *
								  NSEC_PER_USEC);
#endif /* CONFIG_NET_PKT_TIMESTAMP */
			net_pkt_set_ieee802154_rssi_dbm(ack_pkt, frame.rx.rssi);
			net_pkt_set_ieee802154_lqi(ack_pkt, frame.rx.lqi);
			net_pkt_set_ieee802154_ack_fpb(ack_pkt, frame.rx.frame_pending);
			net_pkt_cursor_init(ack_pkt);
			if (ieee802154_handle_ack(data->iface, ack_pkt) != NET_OK) {
				if (data->event_handler) {
					data->event_handler(
						dev, IEEE802154_EVENT_RX_FAILED,
						(void *)&((enum ieee802154_rx_fail_reason){
							IEEE802154_RX_FAIL_NOT_RECEIVED}));
				}
				LOG_INF("ack packet not handled");
				result = -ENODATA;
				break;
			}
			result = 0;
		} while (0);
		if (ack_pkt) {
			net_pkt_unref(ack_pkt);
		}
		free(frame.data);
	} else {
		if (data->event_handler) {
			data->event_handler(dev, IEEE802154_EVENT_RX_FAILED,
					    (void *)&((enum ieee802154_rx_fail_reason){
						    IEEE802154_RX_FAIL_NOT_RECEIVED}));
		}
	}

	return result;
}

static int telink_rcp_zb_start(const struct device *dev)
{
	LOG_DBG("%s", __func__);
	int result = 0;
	struct telink_rcp_zb_data *data = dev->data;

	if (!data->reception_on) {
		result = openthread_rcp_receive_enable(&data->ot_rcp, true);
		if (!result) {
			data->reception_on = true;
		}
	}
	return result;
}

static int telink_rcp_zb_stop(const struct device *dev)
{
	LOG_DBG("%s", __func__);
	int result = 0;
	struct telink_rcp_zb_data *data = dev->data;

	if (data->reception_on) {
		result = openthread_rcp_receive_enable(&data->ot_rcp, false);
		if (!result) {
			data->reception_on = false;
		}
	}
	return result;
}

static int telink_rcp_zb_configure(const struct device *dev, enum ieee802154_config_type type,
				   const struct ieee802154_config *config)
{
	LOG_DBG("%s", __func__);
	int result = -ENOTSUP;
	struct telink_rcp_zb_data *data = dev->data;

	switch (type) {
	case IEEE802154_CONFIG_AUTO_ACK_FPB:
		result = openthread_rcp_enable_src_match(&data->ot_rcp,
							 config->auto_ack_fpb.enabled);
		break;
	case IEEE802154_CONFIG_ACK_FPB:
		if (config->ack_fpb.extended) {
			if (config->ack_fpb.addr) {
				uint8_t address[TELINK_RCP_ZB_EXT_ADDR_LENGTH];

				sys_memcpy_swap(address, config->ack_fpb.addr, sizeof(address));
				result = openthread_rcp_ack_fpb_ext(&data->ot_rcp, address,
								    config->ack_fpb.enabled);
			} else {
				result = 0;
			}
		} else {
			uint16_t addr = sys_get_le16(config->ack_fpb.addr);

			result = openthread_rcp_ack_fpb(&data->ot_rcp, addr,
							config->ack_fpb.enabled);
		}
		break;
	case IEEE802154_CONFIG_EVENT_HANDLER:
		data->event_handler = config->event_handler;
		break;
	case IEEE802154_CONFIG_FRAME_COUNTER:
		result = openthread_rcp_mac_frame_counter(&data->ot_rcp, config->frame_counter,
							  false);
		break;
	case IEEE802154_CONFIG_FRAME_COUNTER_IF_LARGER:
		result = openthread_rcp_mac_frame_counter(&data->ot_rcp, config->frame_counter,
							  true);
		break;
	case IEEE802154_CONFIG_ENH_ACK_HEADER_IE:
		if (config->ack_ie.header_ie) {
			struct spinel_link_metrics link_metrics = {};
			uint8_t len = config->ack_ie.header_ie->length;
			uint8_t *vendor_data = (uint8_t *)&config->ack_ie.header_ie->content;

			for (uint8_t i = IEEE802154_VENDOR_SPECIFIC_IE_OUI_LEN + 1; i < len; i++) {
				switch (vendor_data[i]) {
				case 0x01:
					link_metrics.rssi = true;
					break;
				case 0x02:
					link_metrics.link_margin = true;
					break;
				case 0x03:
					link_metrics.lqi = true;
					break;
				}
			}
			result = openthread_rcp_link_metrics(&data->ot_rcp,
							     config->ack_ie.short_addr,
							     config->ack_ie.ext_addr, link_metrics);
		}
		break;
	case IEEE802154_CONFIG_MAC_KEYS: {
		struct spinel_mac_keys mac_keys = {
			.key_mode = config->mac_keys[0].key_value
					    ? config->mac_keys[1].key_id_mode << 3
					    : 0,
			.key_id = config->mac_keys[0].key_value ? *config->mac_keys[1].key_id : 0,
		};

		if (config->mac_keys[0].key_value) {
			memcpy(mac_keys.prev_key, config->mac_keys[0].key_value,
			       sizeof(mac_keys.prev_key));
			memcpy(mac_keys.curr_key, config->mac_keys[1].key_value,
			       sizeof(mac_keys.curr_key));
			memcpy(mac_keys.next_key, config->mac_keys[2].key_value,
			       sizeof(mac_keys.next_key));
		}
		result = openthread_rcp_mac_keys(&data->ot_rcp, &mac_keys);
	} break;
	default:
		LOG_WRN("unhandled configuration %u", type);
		break;
	}

	return result;
}

static int telink_rcp_zb_ed_scan(const struct device *dev, uint16_t duration,
				 energy_scan_done_cb_t done_cb)
{
	LOG_WRN("%s not supported", __func__);
	return -ENOTSUP;
}

static net_time_t telink_rcp_zb_get_time(const struct device *dev)
{
	LOG_DBG("%s", __func__);

	return k_cyc_to_ns_floor64(sys_clock_cycle_get_64());
}

static uint8_t telink_rcp_zb_get_sch_acc(const struct device *dev)
{
	LOG_DBG("%s", __func__);
	ARG_UNUSED(dev);

	return CONFIG_IEEE802154_TELINK_RCP_DELAY_TRX_ACC;
}

static int telink_rcp_zb_attr_get(const struct device *dev, enum ieee802154_attr attr,
				  struct ieee802154_attr_value *value)
{
	LOG_DBG("%s", __func__);
	ARG_UNUSED(dev);

	return ieee802154_attr_get_channel_page_and_range(
		attr, IEEE802154_ATTR_PHY_CHANNEL_PAGE_ZERO_OQPSK_2450_BPSK_868_915,
		&telink_rcp_zb_drv_attr.phy_supported_channels, value);
}

static int telink_rcp_zb_init(const struct device *dev)
{
	LOG_DBG("%s", __func__);

	int result = 0;
	const struct telink_rcp_zb_config *cfg = (const struct telink_rcp_zb_config *)dev->config;
	struct telink_rcp_zb_data *data = (struct telink_rcp_zb_data *)dev->data;

	do {
		if (!device_is_ready(cfg->uart_dev)) {
			LOG_ERR("spinel serial not ready");
			result = -EIO;
			break;
		}
		LOG_INF("spinel on %s", cfg->uart_dev->name);
		struct uart_config uart_cfg;

		if (!uart_config_get(cfg->uart_dev, &uart_cfg)) {
			static const char *const uart_data_bits_str[] = {"5", "6", "7", "8", "9"};
			static const char *const uart_parity_bits_str[] = {"none", "odd", "even",
									   "mark", "space"};
			static const char *const uart_stop_bits_str[] = {"0.5", "1", "1.5", "2"};
			static const char *const uart_flow_ctrl_str[] = {"none", "rts-cts",
									 "dtr-dsr", "rs-485"};

			LOG_INF("uart: %u %s %s %s %s", uart_cfg.baudrate,
				uart_cfg.data_bits < ARRAY_SIZE(uart_data_bits_str)
					? uart_data_bits_str[uart_cfg.data_bits]
					: "invalid",
				uart_cfg.parity < ARRAY_SIZE(uart_parity_bits_str)
					? uart_parity_bits_str[uart_cfg.parity]
					: "invalid",
				uart_cfg.stop_bits < ARRAY_SIZE(uart_stop_bits_str)
					? uart_stop_bits_str[uart_cfg.stop_bits]
					: "invalid",
				uart_cfg.flow_ctrl < ARRAY_SIZE(uart_flow_ctrl_str)
					? uart_flow_ctrl_str[uart_cfg.flow_ctrl]
					: "invalid");
			LOG_INF("pins: %s", cfg->uart_pins_str);
		} else {
			LOG_ERR("spinel serial config fail");
			result = -EIO;
			break;
		}

		if (openthread_rcp_init(&data->ot_rcp, cfg->uart_dev)) {
			LOG_ERR("spinel init fail");
			result = -EIO;
			break;
		}
		openthread_rcp_reception_set(&data->ot_rcp, telink_rcp_zb_rx, dev);
		if (openthread_rcp_reset(&data->ot_rcp)) {
			LOG_ERR("rcp reset fail");
			result = -EIO;
			break;
		}
		data->channel = (uint8_t)-1;
		data->tx_power = OT_RADIO_POWER_INVALID;
	} while (0);

	return result;
}

static const struct ieee802154_radio_api telink_rcp_zb_drv_api = {
	.iface_api.init = telink_rcp_zb_iface_init,
	.get_capabilities = telink_rcp_zb_get_capabilities,
	.cca = telink_rcp_zb_cca,
	.set_channel = telink_rcp_zb_set_channel,
	.filter = telink_rcp_zb_filter,
	.set_txpower = telink_rcp_zb_set_txpower,
	.tx = telink_rcp_zb_tx,
	.start = telink_rcp_zb_start,
	.stop = telink_rcp_zb_stop,
	.configure = telink_rcp_zb_configure,
	.ed_scan = telink_rcp_zb_ed_scan,
	.get_time = telink_rcp_zb_get_time,
	.get_sch_acc = telink_rcp_zb_get_sch_acc,
	.attr_get = telink_rcp_zb_attr_get,
};

#define DT_PROP_BY_IDX_DT_NODE_FULL_NAME(node_id, prop, idx)                                       \
	DT_NODE_FULL_NAME(DT_PROP_BY_IDX(node_id, prop, idx))

#if CONFIG_NET_L2_IEEE802154 || CONFIG_NET_L2_OPENTHREAD

#if CONFIG_NET_L2_IEEE802154 && CONFIG_NET_L2_OPENTHREAD
#error Networks IEEE802.15.4 & openthread are not supported at the same time
#elif CONFIG_NET_L2_IEEE802154
#define TELINK_RCP_ZB_L2          IEEE802154_L2
#define TELINK_RCP_ZB_L2_CTX_TYPE NET_L2_GET_CTX_TYPE(IEEE802154_L2)
#define TELINK_RCP_ZB_MTU         125
#elif CONFIG_NET_L2_OPENTHREAD
#define TELINK_RCP_ZB_L2          OPENTHREAD_L2
#define TELINK_RCP_ZB_L2_CTX_TYPE NET_L2_GET_CTX_TYPE(OPENTHREAD_L2)
#define TELINK_RCP_ZB_MTU         1280
#endif /* net L2 select */

#define TELINK_RCP_ZB_DEFINE(n)                                                                    \
                                                                                                   \
	static const struct telink_rcp_zb_config telink_rcp_zb_config_##n = {                      \
		.uart_dev = DEVICE_DT_GET(DT_INST_PROP(n, serial)),                                \
		.uart_pins_str = "[" DT_FOREACH_PROP_ELEM_SEP(DT_INST_PROP(n, serial), pinctrl_0,  \
							      DT_PROP_BY_IDX_DT_NODE_FULL_NAME,    \
							      (", ")) "]"};         \
                                                                                                   \
	static struct telink_rcp_zb_data telink_rpc_zb_data_##n;                                   \
                                                                                                   \
	NET_DEVICE_DT_INST_DEFINE(n, telink_rcp_zb_init, NULL, &telink_rpc_zb_data_##n,            \
				  &telink_rcp_zb_config_##n,                                       \
				  CONFIG_IEEE802154_TELINK_RCP_INIT_PRIO, &telink_rcp_zb_drv_api,  \
				  TELINK_RCP_ZB_L2, TELINK_RCP_ZB_L2_CTX_TYPE, TELINK_RCP_ZB_MTU);

#else

#define TELINK_RCP_ZB_DEFINE(n)                                                                    \
                                                                                                   \
	static const struct telink_rcp_zb_config telink_rcp_zb_config_##n = {                      \
		.uart_dev = DEVICE_DT_GET(DT_INST_PROP(n, serial)),                                \
		.uart_pins_str = "[" DT_FOREACH_PROP_ELEM_SEP(DT_INST_PROP(n, serial), pinctrl_0,  \
							      DT_PROP_BY_IDX_DT_NODE_FULL_NAME,    \
							      (", ")) "]"};         \
                                                                                                   \
	static struct telink_rcp_zb_data telink_rpc_zb_data_##n;                                   \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, telink_rcp_zb_init, NULL, &telink_rpc_zb_data_##n,                \
			      &telink_rcp_zb_config_##n, POST_KERNEL,                              \
			      CONFIG_IEEE802154_TELINK_RCP_INIT_PRIO, &telink_rcp_zb_drv_api);

#endif /* CONFIG_NET_L2_IEEE802154 || CONFIG_NET_L2_OPENTHREAD */

DT_INST_FOREACH_STATUS_OKAY(TELINK_RCP_ZB_DEFINE)
