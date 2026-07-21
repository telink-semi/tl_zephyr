/*
 * Copyright (c) 2021-2026 Telink Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT telink_b9x_zb

#include "rf.h"
#include "stimer.h"
#include "tl_rf_power.h"

#define LOG_MODULE_NAME ieee802154_b9x
#if defined(CONFIG_IEEE802154_DRIVER_LOG_LEVEL)
#define LOG_LEVEL CONFIG_IEEE802154_DRIVER_LOG_LEVEL
#else
#define LOG_LEVEL LOG_LEVEL_NONE
#endif

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

#include <zephyr/random/random.h>
#include <zephyr/net/ieee802154_radio.h>
#include <zephyr/irq.h>
#include <zephyr/drivers/interrupt_controller/riscv_plic.h>
#if defined(CONFIG_NET_L2_OPENTHREAD)
#include <zephyr/net/openthread.h>
#endif

#include <zephyr/pm/device.h>
#include <zephyr/pm/policy.h>

#include "ieee802154_b9x.h"

#include "ieee802154_b9x_frame.c"

#if defined(CONFIG_IEEE802154_B9X_MAC_FLASH)
#include <zephyr/drivers/flash.h>
#include <zephyr/storage/flash_map.h>

static const struct device *flash_device =
	DEVICE_DT_GET(DT_CHOSEN(zephyr_flash_controller));
#endif /* CONFIG_IEEE802154_B9X_MAC_FLASH */

#ifdef CONFIG_OPENTHREAD_FTD
/* B9X radio source match table structure */
static struct b9x_src_match_table src_match_table;
#endif /* CONFIG_OPENTHREAD_FTD */

#if !defined(CONFIG_OPENTHREAD_THREAD_VERSION_1_1)
/* B9X radio ACK table structure */
static struct b9x_enh_ack_table enh_ack_table;

/* mac keys data */
static struct b9x_mac_keys mac_keys;
#endif

/* B9X data structure */
static struct b9x_data data = {
#ifdef CONFIG_OPENTHREAD_FTD
	.src_match_table = &src_match_table,
#endif /* CONFIG_OPENTHREAD_FTD */
#if !defined(CONFIG_OPENTHREAD_THREAD_VERSION_1_1)
	.enh_ack_table = &enh_ack_table,
	/* mac keys data */
	.mac_keys = &mac_keys,
#endif
};

#ifdef CONFIG_OPENTHREAD_FTD

/* clean radio search match table */
ALWAYS_INLINE static void b9x_src_match_table_clean(struct b9x_src_match_table *table)
{
	memset(table, 0, sizeof(struct b9x_src_match_table));
}

/* Search in radio search match table */
ALWAYS_INLINE static bool b9x_src_match_table_search(const struct b9x_src_match_table *table,
						     const uint8_t *addr, bool ext)
{
	bool result = false;

	for (size_t i = 0; i < 2 * CONFIG_OPENTHREAD_MAX_CHILDREN; i++) {
		if (table->item[i].valid && table->item[i].ext == ext &&
			!memcmp(table->item[i].addr, addr,
				ext ? IEEE802154_FRAME_LENGTH_ADDR_EXT :
				IEEE802154_FRAME_LENGTH_ADDR_SHORT)) {
			result = true;
			break;
		}
	}

	return result;
}

/* Add to radio search match table */
ALWAYS_INLINE static void b9x_src_match_table_add(struct b9x_src_match_table *table,
						  const uint8_t *addr, bool ext)
{
	if (!b9x_src_match_table_search(table, addr, ext)) {
		for (size_t i = 0; i < 2 * CONFIG_OPENTHREAD_MAX_CHILDREN; i++) {
			if (!table->item[i].valid) {
				table->item[i].ext = ext;
				memcpy(table->item[i].addr, addr,
					ext ? IEEE802154_FRAME_LENGTH_ADDR_EXT :
					IEEE802154_FRAME_LENGTH_ADDR_SHORT);
				table->item[i].valid = true;
				break;
			}
		}
	}
}

/* Remove from radio search match table */
ALWAYS_INLINE static void b9x_src_match_table_remove(struct b9x_src_match_table *table,
						     const uint8_t *addr, bool ext)
{
	for (size_t i = 0; i < 2 * CONFIG_OPENTHREAD_MAX_CHILDREN; i++) {
		if (table->item[i].valid && table->item[i].ext == ext &&
			!memcmp(table->item[i].addr, addr,
				ext ? IEEE802154_FRAME_LENGTH_ADDR_EXT :
				IEEE802154_FRAME_LENGTH_ADDR_SHORT)) {
			table->item[i].valid = false;
			table->item[i].ext = false;
			memset(table->item[i].addr, 0,
				ext ? IEEE802154_FRAME_LENGTH_ADDR_EXT :
				IEEE802154_FRAME_LENGTH_ADDR_SHORT);
			break;
		}
	}
}

/* Remove all entries from radio search match table */
ALWAYS_INLINE static void b9x_src_match_table_remove_group(struct b9x_src_match_table *table,
							   bool ext)
{
	for (size_t i = 0; i < 2 * CONFIG_OPENTHREAD_MAX_CHILDREN; i++) {
		if (table->item[i].valid && table->item[i].ext == ext) {
			table->item[i].valid = false;
			table->item[i].ext = false;
			memset(table->item[i].addr, 0,
				ext ? IEEE802154_FRAME_LENGTH_ADDR_EXT :
				IEEE802154_FRAME_LENGTH_ADDR_SHORT);
		}
	}
}

/*
 * Check frame possible require to set pending bit
 * data request command or data
 * frame should be valid
 */
ALWAYS_INLINE static bool b9x_require_pending_bit(const struct ieee802154_frame *frame)
{
	bool result = false;

	if (frame->general.valid) {
		if (frame->general.type == IEEE802154_FRAME_FCF_TYPE_DATA) {
			result = true;
		} else if (frame->general.type == IEEE802154_FRAME_FCF_TYPE_CMD) {
			if (!frame->sec_header ||
				frame->general.ver < IEEE802154_FRAME_FCF_VER_2015 ||
				(frame->sec_header[0] & IEEE802154_FRAME_SECCTRL_SEC_LEVEL_MASK) <
					IEEE802154_FRAME_SECCTRL_SEC_LEVEL_4) {
				const uint8_t *cmd_id = frame->payload_ie ?
					b9x_ieee802154_get_data(frame->payload,
					frame->payload_len) : frame->payload;
				if (cmd_id && *cmd_id == B9X_CMD_ID_DATA_REQ) {
					result = true;
				}
			} else {
				/* TODO: temporary solution. need to decrypt payload */
				result = true;
			}
		}
	}
	return result;
}

#endif /* CONFIG_OPENTHREAD_FTD */

#if !defined(CONFIG_OPENTHREAD_THREAD_VERSION_1_1)

/* clean radio search match table */
ALWAYS_INLINE static void b9x_enh_ack_table_clean(struct b9x_enh_ack_table *table)
{
	memset(table, 0, sizeof(struct b9x_enh_ack_table));
}

/* Search in enhanced ack table */
ALWAYS_INLINE static int b9x_enh_ack_table_search(const struct b9x_enh_ack_table *table,
						  const uint8_t *addr_short,
						  const uint8_t *addr_ext)
{
	int result = -1;

	for (size_t i = 0; i < CONFIG_OPENTHREAD_MAX_CHILDREN; i++) {
		if (table->item[i].valid &&
			(!memcmp(table->item[i].addr_short, addr_short,
				IEEE802154_FRAME_LENGTH_ADDR_SHORT) ||
			!memcmp(table->item[i].addr_ext, addr_ext,
				IEEE802154_FRAME_LENGTH_ADDR_EXT))) {
			result = i;
			break;
		}
	}

	return result;
}

/* Add to enhanced ack table */
ALWAYS_INLINE static void b9x_enh_ack_table_add(struct b9x_enh_ack_table *table,
						const uint8_t *addr_short, const uint8_t *addr_ext,
						const struct ieee802154_header_ie *ie_header)
{
	int idx = b9x_enh_ack_table_search(table, addr_short, addr_ext);

	if (idx == -1) {
		for (size_t i = 0; i < CONFIG_OPENTHREAD_MAX_CHILDREN; i++) {
			if (!table->item[i].valid) {
				idx = i;
				memcpy(table->item[idx].addr_short, addr_short,
					IEEE802154_FRAME_LENGTH_ADDR_SHORT);
				memcpy(table->item[idx].addr_ext, addr_ext,
					IEEE802154_FRAME_LENGTH_ADDR_EXT);
				table->item[idx].valid = true;
				break;
			}
		}
	}
	if (idx != -1) {
		memcpy(&table->item[idx].ie_header, ie_header,
			sizeof(struct ieee802154_header_ie));
	}
}

/* Remove from enhanced ack table */
ALWAYS_INLINE static void b9x_enh_ack_table_remove(struct b9x_enh_ack_table *table,
						   const uint8_t *addr_short,
						   const uint8_t *addr_ext)
{
	for (size_t i = 0; i < CONFIG_OPENTHREAD_MAX_CHILDREN; i++) {
		if (table->item[i].valid &&
			!memcmp(table->item[i].addr_short, addr_short,
				IEEE802154_FRAME_LENGTH_ADDR_SHORT) &&
			!memcmp(table->item[i].addr_ext, addr_ext,
				IEEE802154_FRAME_LENGTH_ADDR_EXT)) {
			table->item[i].valid = false;
			memset(table->item[i].addr_short, 0,
				IEEE802154_FRAME_LENGTH_ADDR_SHORT);
			memset(table->item[i].addr_ext, 0,
				IEEE802154_FRAME_LENGTH_ADDR_EXT);
			memset(&table->item[i].ie_header, 0,
				sizeof(struct ieee802154_header_ie));
			break;
		}
	}
}

/* Clean mac keys data */
ALWAYS_INLINE static void b9x_mac_keys_data_clean(struct b9x_mac_keys *mac_keys_data)
{
	memset(mac_keys_data, 0, sizeof(struct b9x_mac_keys));
}

ALWAYS_INLINE static const uint8_t *b9x_mac_keys_get(const struct b9x_mac_keys *mac_keys_data,
						     uint8_t key_id)
{
	const uint8_t *result = NULL;

	if (key_id) {
		for (size_t i = 0; i < B9X_MAC_KEYS_ITEMS; i++) {
			if (mac_keys_data->item[i].key_id == key_id) {
				result = mac_keys_data->item[i].key;
				break;
			}
		}
	}
	return result;
}

ALWAYS_INLINE static uint32_t b9x_mac_keys_frame_cnt_get(const struct b9x_mac_keys *mac_keys_data,
							 uint8_t key_id)
{
	uint32_t result = 0;

	if (key_id) {
		for (size_t i = 0; i < B9X_MAC_KEYS_ITEMS; i++) {
			if (mac_keys_data->item[i].key_id == key_id) {
				if (mac_keys_data->item[i].frame_cnt_local) {
					result = mac_keys_data->item[i].frame_cnt;
				} else {
					result = mac_keys_data->frame_cnt;
				}
				break;
			}
		}
	}
	return result;
}

ALWAYS_INLINE static void b9x_mac_keys_frame_cnt_inc(struct b9x_mac_keys *mac_keys_data,
						     uint8_t key_id)
{
	if (key_id) {
		for (size_t i = 0; i < B9X_MAC_KEYS_ITEMS; i++) {
			if (mac_keys_data->item[i].key_id == key_id) {
				if (mac_keys_data->item[i].frame_cnt_local) {
					mac_keys_data->item[i].frame_cnt++;
				} else {
					mac_keys_data->frame_cnt++;
				}
				break;
			}
		}
	}
}

#endif

#ifdef CONFIG_OPENTHREAD_CSL_RECEIVER
ALWAYS_INLINE static uint16_t b9x_get_csl_phase(struct b9x_data *b9x)
{
	uint32_t csl_period_us = b9x->csl_period * 160;
	int64_t cur_time_us = k_ticks_to_us_floor64(k_uptime_ticks());
	int64_t diff_us = b9x->csl_sample_time_us - cur_time_us;

	diff_us = diff_us < 0 ? diff_us % csl_period_us + csl_period_us : diff_us % csl_period_us;

	return (uint16_t)(diff_us / 160 + 1);
}
#endif /* CONFIG_OPENTHREAD_CSL_RECEIVER */

/* Disable power management by device */
ALWAYS_INLINE static void b9x_disable_pm(struct b9x_data *b9x)
{
#ifdef CONFIG_PM_DEVICE
	if (atomic_test_and_set_bit(&b9x->current_pm_lock, 0) == 0) {
		pm_policy_state_lock_get(PM_STATE_SUSPEND_TO_IDLE, PM_ALL_SUBSTATES);
	}
	if (atomic_test_and_set_bit(&b9x->current_pm_lock, 1) == 0) {
		pm_policy_state_lock_get(PM_STATE_STANDBY, PM_ALL_SUBSTATES);
	}
#else
	ARG_UNUSED(b9x);
#endif /* CONFIG_PM_DEVICE */
}

/* Enable power management by device */
ALWAYS_INLINE static void b9x_enable_pm(struct b9x_data *b9x)
{
#ifdef CONFIG_PM_DEVICE
	if (atomic_test_and_clear_bit(&b9x->current_pm_lock, 0) == 1) {
		pm_policy_state_lock_put(PM_STATE_SUSPEND_TO_IDLE, PM_ALL_SUBSTATES);
	}
	if (atomic_test_and_clear_bit(&b9x->current_pm_lock, 1) == 1) {
		pm_policy_state_lock_put(PM_STATE_STANDBY, PM_ALL_SUBSTATES);
	}
#else
	ARG_UNUSED(b9x);
#endif /* CONFIG_PM_DEVICE */
}

/* Set filter PAN ID */
ALWAYS_INLINE static int b9x_set_pan_id(const struct device *dev, uint16_t pan_id)
{
	struct b9x_data *b9x = dev->data;
	uint8_t pan_id_le[IEEE802154_FRAME_LENGTH_PANID];

	sys_put_le16(pan_id, pan_id_le);
	memcpy(b9x->filter_pan_id, pan_id_le, IEEE802154_FRAME_LENGTH_PANID);

	return 0;
}

/* Set filter short address */
ALWAYS_INLINE static int b9x_set_short_addr(const struct device *dev, uint16_t short_addr)
{
	struct b9x_data *b9x = dev->data;
	uint8_t short_addr_le[IEEE802154_FRAME_LENGTH_ADDR_SHORT];

	sys_put_le16(short_addr, short_addr_le);
	memcpy(b9x->filter_short_addr, short_addr_le, IEEE802154_FRAME_LENGTH_ADDR_SHORT);

	return 0;
}

/* Set filter IEEE address */
ALWAYS_INLINE static int b9x_set_ieee_addr(const struct device *dev, const uint8_t *ieee_addr)
{
	struct b9x_data *b9x = dev->data;

	memcpy(b9x->filter_ieee_addr, ieee_addr, IEEE802154_FRAME_LENGTH_ADDR_EXT);

	return 0;
}

/* Filter PAN ID, short address and IEEE address */
ALWAYS_INLINE static bool b9x_run_filter(const struct device *dev,
					 const struct ieee802154_frame *frame)
{
	struct b9x_data *b9x = dev->data;
	bool result = false;

	do {
		if (frame->dst_panid != NULL) {
			if (memcmp(frame->dst_panid, b9x->filter_pan_id,
					IEEE802154_FRAME_LENGTH_PANID) != 0 &&
				memcmp(frame->dst_panid, B9X_BROADCAST_ADDRESS,
					IEEE802154_FRAME_LENGTH_PANID) != 0) {
				break;
			}
		}
		if (frame->dst_addr != NULL) {
			if (frame->dst_addr_ext) {
				if ((net_if_get_link_addr(b9x->iface)->len !=
						IEEE802154_FRAME_LENGTH_ADDR_EXT) ||
					memcmp(frame->dst_addr, b9x->filter_ieee_addr,
						IEEE802154_FRAME_LENGTH_ADDR_EXT) != 0) {
					break;
				}
			} else {
				if (memcmp(frame->dst_addr, B9X_BROADCAST_ADDRESS,
						IEEE802154_FRAME_LENGTH_ADDR_SHORT) != 0 &&
					memcmp(frame->dst_addr, b9x->filter_short_addr,
						IEEE802154_FRAME_LENGTH_ADDR_SHORT) != 0) {
					break;
				}
			}
		}
		result = true;
	} while (0);

	return result;
}

/* Get MAC address */
ALWAYS_INLINE static uint8_t *b9x_get_mac(const struct device *dev)
{
	struct b9x_data *b9x = dev->data;

#if defined(CONFIG_IEEE802154_B9X_MAC_RANDOM)
	sys_rand_get(b9x->mac_addr, sizeof(b9x->mac_addr));

	/*
	 * Clear bit 0 to ensure it isn't a multicast address and set
	 * bit 1 to indicate address is locally administered and may
	 * not be globally unique.
	 */
	b9x->mac_addr[0] = (b9x->mac_addr[0] & ~0x01) | 0x02;
#elif defined(CONFIG_IEEE802154_B9X_MAC_FLASH)
	(void) flash_read(flash_device, FIXED_PARTITION_OFFSET(vendor_partition)
			+ IEEE802154_B9X_FLASH_MAC_OFFSET, b9x->mac_addr,
			IEEE802154_FRAME_LENGTH_ADDR_EXT);
#else /* CONFIG_IEEE802154_B9X_MAC_STATIC */
	/* Vendor Unique Identifier */
	b9x->mac_addr[0] = 0xC4;
	b9x->mac_addr[1] = 0x19;
	b9x->mac_addr[2] = 0xD1;
	b9x->mac_addr[3] = 0x00;

	/* Extended Unique Identifier */
	b9x->mac_addr[4] = CONFIG_IEEE802154_B9X_MAC4;
	b9x->mac_addr[5] = CONFIG_IEEE802154_B9X_MAC5;
	b9x->mac_addr[6] = CONFIG_IEEE802154_B9X_MAC6;
	b9x->mac_addr[7] = CONFIG_IEEE802154_B9X_MAC7;
#endif /* IEEE802154_B9X_MAC_TYPE */

	return b9x->mac_addr;
}

/* Convert RSSI to LQI */
ALWAYS_INLINE static uint8_t b9x_convert_rssi_to_lqi(int8_t rssi)
{
	uint32_t lqi32 = 0;

	/* check for MIN value */
	if (rssi < B9X_RSSI_TO_LQI_MIN) {
		return 0;
	}

	/* convert RSSI to LQI */
	lqi32 = B9X_RSSI_TO_LQI_SCALE * (rssi - B9X_RSSI_TO_LQI_MIN);

	/* check for MAX value */
	if (lqi32 > 0xFF) {
		lqi32 = 0xFF;
	}

	return (uint8_t)lqi32;
}

/* Update RSSI and LQI parameters */
ALWAYS_INLINE static void b9x_update_rssi_and_lqi(const struct device *dev, struct net_pkt *pkt)
{
	struct b9x_data *b9x = dev->data;
	int8_t rssi;
	uint8_t lqi;

	rssi = ((signed char)(b9x->rx_buffer
			      [b9x->rx_buffer[B9X_LENGTH_OFFSET] + B9X_RSSI_OFFSET])) - 110;
	lqi = b9x_convert_rssi_to_lqi(rssi);

	net_pkt_set_ieee802154_lqi(pkt, lqi);
	net_pkt_set_ieee802154_rssi_dbm(pkt, rssi);
}

/* Prepare TX buffer */
ALWAYS_INLINE static void b9x_set_tx_payload(const struct device *dev, uint8_t *payload,
					     uint8_t payload_len)
{
	struct b9x_data *b9x = dev->data;
	unsigned char rf_data_len;
	unsigned int rf_tx_dma_len;

	rf_data_len = payload_len + 1;
	rf_tx_dma_len = rf_tx_packet_dma_len(rf_data_len);
	b9x->tx_buffer[0] = rf_tx_dma_len & 0xff;
	b9x->tx_buffer[1] = (rf_tx_dma_len >> 8) & 0xff;
	b9x->tx_buffer[2] = (rf_tx_dma_len >> 16) & 0xff;
	b9x->tx_buffer[3] = (rf_tx_dma_len >> 24) & 0xff;
	b9x->tx_buffer[4] = payload_len + IEEE802154_FCS_LENGTH;
	memcpy(b9x->tx_buffer + B9X_PAYLOAD_OFFSET, payload, payload_len);
}

/* Handle acknowledge packet */
ALWAYS_INLINE static void b9x_handle_ack(const struct device *dev, const void *buf, size_t buf_len,
					 uint64_t rx_time)
{
	struct b9x_data *b9x = dev->data;
	struct net_pkt *ack_pkt = net_pkt_rx_alloc_with_buffer(
		b9x->iface, buf_len, AF_UNSPEC, 0, K_NO_WAIT);

	do {
		if (!ack_pkt) {
			LOG_ERR("No free packet available.");
			break;
		}
		if (net_pkt_write(ack_pkt, buf, buf_len) < 0) {
			LOG_ERR("Failed to write to a packet.");
			break;
		}
		b9x_update_rssi_and_lqi(dev, ack_pkt);
#if defined(CONFIG_NET_PKT_TIMESTAMP) && defined(CONFIG_NET_PKT_TXTIME)
		net_pkt_set_timestamp_ns(ack_pkt, rx_time * NSEC_PER_USEC);
#endif /* CONFIG_NET_PKT_TIMESTAMP && CONFIG_NET_PKT_TXTIME */
		net_pkt_cursor_init(ack_pkt);
		if (ieee802154_handle_ack(b9x->iface, ack_pkt) != NET_OK) {
			LOG_INF("ACK packet not handled - releasing.");
		}
		k_sem_give(&b9x->ack_wait);
	} while (0);

	if (ack_pkt) {
		net_pkt_unref(ack_pkt);
	}
}

/* Send acknowledge packet */
ALWAYS_INLINE static void b9x_send_ack(const struct device *dev, struct ieee802154_frame *frame)
{
	struct b9x_data *b9x = dev->data;
	uint8_t ack_buf[64];
	size_t ack_len;

	rf_set_txmode();
	rf_clr_irq_mask(FLD_RF_IRQ_TX); /* clear TX interrupt mask bit */
	unsigned int t0 = stimer_get_tick();

#if !defined(CONFIG_OPENTHREAD_THREAD_VERSION_1_1)
	const uint8_t *key = NULL;
	uint32_t frame_cnt = 0;
	uint8_t sec_header[] = {
		IEEE802154_FRAME_SECCTRL_SEC_LEVEL_5 | IEEE802154_FRAME_SECCTRL_KEY_ID_MODE_1,
		0,
		0,
		0,
		0,
		1};

	if (frame->general.ver == IEEE802154_FRAME_FCF_VER_2015) {
		if (frame->general.se_bit) {
			key = b9x_mac_keys_get(b9x->mac_keys, 1);
			if (key) {
				b9x_mac_keys_frame_cnt_inc(b9x->mac_keys, 1);
				frame_cnt = b9x_mac_keys_frame_cnt_get(b9x->mac_keys, 1);
				sec_header[1] = frame_cnt;
				sec_header[2] = frame_cnt >> 8;
				sec_header[3] = frame_cnt >> 16;
				sec_header[4] = frame_cnt >> 24;
				frame->sec_header = sec_header;
				frame->sec_header_len = sizeof(sec_header);
			} else {
				frame->general.se_bit = false;
			}
		}

		if (frame->payload) {
#ifdef CONFIG_OPENTHREAD_CSL_RECEIVER
			if (b9x->csl_period > 0) {
				(void)b9x_ieee802154_ie_csl_commit(
					(uint8_t *)frame->payload, frame->payload_len,
					b9x->csl_period, b9x_get_csl_phase(b9x));
			}
#endif /* CONFIG_OPENTHREAD_CSL_RECEIVER */
		}
	}
#endif

	if (b9x_ieee802154_frame_build(frame, ack_buf, sizeof(ack_buf), &ack_len)) {
#if !defined(CONFIG_OPENTHREAD_THREAD_VERSION_1_1)
		if (frame->general.se_bit) {
			if (!ieee802154_b9x_crypto_encrypt(
				    key, b9x->filter_ieee_addr, frame_cnt,
				    IEEE802154_FRAME_SECCTRL_SEC_LEVEL_5, ack_buf,
				    ack_len - THREAD_FRAME_SECCTRL_MIC_LENGTH, NULL, 0, NULL,
				    &ack_buf[ack_len - THREAD_FRAME_SECCTRL_MIC_LENGTH],
				    THREAD_FRAME_SECCTRL_MIC_LENGTH)) {
				LOG_WRN("encrypt ack failed");
			}
		}
#endif
		b9x_set_tx_payload(dev, ack_buf, ack_len);
		unsigned int t1 = (stimer_get_tick() - t0) / SYSTEM_TIMER_TICK_1US;

		if (t1 < CONFIG_IEEE802154_B9X_SET_TXRX_DELAY_US) {
			delay_us(CONFIG_IEEE802154_B9X_SET_TXRX_DELAY_US - t1);
		}
		rf_tx_pkt(b9x->tx_buffer);
		while (!rf_get_irq_status(FLD_RF_IRQ_TX) &&
		       !clock_time_exceed(stimer_get_tick(), 1000)) {
		}
		rf_clr_irq_status(FLD_RF_IRQ_TX);
		rf_set_irq_mask(FLD_RF_IRQ_TX);
	} else {
		LOG_ERR("Failed to create ACK.");
	}
}

/* RX IRQ handler */
ALWAYS_INLINE static void b9x_rf_rx_isr(const struct device *dev)
{
	struct b9x_data *b9x = dev->data;
	int status = -EINVAL;
	struct net_pkt *pkt = NULL;
	struct ieee802154_frame frame = {};

#if defined(CONFIG_NET_PKT_TIMESTAMP) && defined(CONFIG_NET_PKT_TXTIME)
	uint64_t rx_time = k_ticks_to_us_near64(k_uptime_ticks());
	uint32_t rx_time_radio = stimer_get_tick();
#endif /* CONFIG_NET_PKT_TIMESTAMP && CONFIG_NET_PKT_TXTIME */

	dma_chn_dis(DMA1);
	rf_clr_irq_status(FLD_RF_IRQ_RX);

	do {
		uint8_t length = rf_zigbee_get_payload_len(b9x->rx_buffer);

		if ((length < B9X_PAYLOAD_MIN) || (length > B9X_PAYLOAD_MAX)) {
			if (b9x->event_handler) {
				enum ieee802154_rx_fail_reason reason =
					IEEE802154_RX_FAIL_NOT_RECEIVED;

				b9x->event_handler(dev, IEEE802154_EVENT_RX_FAILED,
					(void *)&reason);
			}
			break;
		}
		if (!rf_zigbee_packet_crc_ok(b9x->rx_buffer)) {
			if (b9x->event_handler) {
				enum ieee802154_rx_fail_reason reason =
					IEEE802154_RX_FAIL_INVALID_FCS;

				b9x->event_handler(dev, IEEE802154_EVENT_RX_FAILED,
					(void *)&reason);
			}
			break;
		}
#if defined(CONFIG_NET_PKT_TIMESTAMP) && defined(CONFIG_NET_PKT_TXTIME)
		rx_time_radio -= ZB_RADIO_TIMESTAMP_GET(b9x->rx_buffer);
		rx_time -= rx_time_radio / SYSTEM_TIMER_TICK_1US;
#endif /* CONFIG_NET_PKT_TIMESTAMP && CONFIG_NET_PKT_TXTIME */
		uint8_t *payload = (b9x->rx_buffer + B9X_PAYLOAD_OFFSET);

		if (IS_ENABLED(CONFIG_IEEE802154_RAW_MODE) ||
			IS_ENABLED(CONFIG_NET_L2_OPENTHREAD)) {
			b9x_ieee802154_frame_parse(payload, length - B9X_FCS_LENGTH, &frame);
		} else {
			length -= B9X_FCS_LENGTH;
			b9x_ieee802154_frame_parse(payload, length, &frame);
		}
		if (!frame.general.valid) {
			LOG_ERR("Invalid frame\n");
			if (b9x->event_handler) {
				enum ieee802154_rx_fail_reason reason =
					IEEE802154_RX_FAIL_NOT_RECEIVED;

				b9x->event_handler(dev, IEEE802154_EVENT_RX_FAILED,
					(void *)&reason);
			}
			break;
		}
		if (frame.general.type == IEEE802154_FRAME_FCF_TYPE_ACK) {
			if (b9x->ack_handler_en) {
				if (b9x->ack_sn == *frame.sn) {
#if defined(CONFIG_NET_PKT_TIMESTAMP) && defined(CONFIG_NET_PKT_TXTIME)
					b9x_handle_ack(dev, payload, length, rx_time);
#else
					b9x_handle_ack(dev, payload, length, 0);
#endif /* CONFIG_NET_PKT_TIMESTAMP && CONFIG_NET_PKT_TXTIME */
					b9x->ack_handler_en = false;
				}
			}
			break;
		}
		if (!b9x_run_filter(dev, &frame)) {
			LOG_DBG("Packet received is not addressed to me.");
			if (b9x->event_handler) {
				enum ieee802154_rx_fail_reason reason =
					IEEE802154_RX_FAIL_ADDR_FILTERED;

				b9x->event_handler(dev, IEEE802154_EVENT_RX_FAILED,
					(void *)&reason);
			}
			break;
		}
		bool frame_pending = false;

		if (frame.general.ack_req) {
#ifdef CONFIG_OPENTHREAD_FTD
			if (b9x_require_pending_bit(&frame)) {
				if (frame.src_addr) {
					if (!b9x->src_match_table->enabled ||
						b9x_src_match_table_search(b9x->src_match_table,
							frame.src_addr, frame.src_addr_ext)) {
						frame_pending = true;
					}
				}
			}
#endif /* CONFIG_OPENTHREAD_FTD */
			bool enh_ack = (frame.general.ver == IEEE802154_FRAME_FCF_VER_2015);
			uint8_t *ack_ie_header = NULL;
			size_t ack_ie_header_len = 0;
			bool ack_se_bit = false;
#if !defined(CONFIG_OPENTHREAD_THREAD_VERSION_1_1)
			if (enh_ack) {
				ack_se_bit = frame.general.se_bit ? true : false;
				int idx = b9x_enh_ack_table_search(b9x->enh_ack_table,
					frame.src_addr_ext ? NULL : frame.src_addr,
					frame.src_addr_ext ? frame.src_addr : NULL);
				if (idx >= 0) {
					ack_ie_header =
						(uint8_t *)&b9x->enh_ack_table->item[idx].ie_header;
					ack_ie_header_len =
						b9x->enh_ack_table->item[idx].ie_header.length +
						IEEE802154_FRAME_IE_HEADER_LENGTH;
				}
			}
#endif
			struct ieee802154_frame ack_frame = {
				.general = {.valid = true,
					    .ver = enh_ack ? IEEE802154_FRAME_FCF_VER_2015
							   : IEEE802154_FRAME_FCF_VER_2003,
					    .type = IEEE802154_FRAME_FCF_TYPE_ACK,
					    .fp_bit = frame_pending,
					    .se_bit = ack_se_bit},
				.sn = frame.sn,
				.dst_panid = enh_ack ? (frame.src_panid ? frame.src_panid
									: frame.dst_panid)
						     : NULL,
				.dst_addr = enh_ack ? frame.src_addr : NULL,
				.dst_addr_ext = enh_ack ? frame.src_addr_ext : false,
				.payload = ack_ie_header,
				.payload_len = ack_ie_header_len,
				.payload_ie = true};
			b9x_send_ack(dev, &ack_frame);
		}
		pkt = net_pkt_rx_alloc_with_buffer(b9x->iface, length, AF_UNSPEC, 0, K_NO_WAIT);
		if (!pkt) {
			LOG_ERR("No pkt available.");
			if (b9x->event_handler) {
				enum ieee802154_rx_fail_reason reason =
					IEEE802154_RX_FAIL_OTHER;

				b9x->event_handler(dev, IEEE802154_EVENT_RX_FAILED,
					(void *)&reason);
			}
			break;
		}
		net_pkt_set_ieee802154_ack_fpb(pkt, frame_pending);
		if (net_pkt_write(pkt, payload, length)) {
			LOG_ERR("Failed to write to a packet.");
			if (b9x->event_handler) {
				enum ieee802154_rx_fail_reason reason =
					IEEE802154_RX_FAIL_OTHER;

				b9x->event_handler(dev, IEEE802154_EVENT_RX_FAILED,
					(void *)&reason);
			}
			break;
		}
		b9x_update_rssi_and_lqi(dev, pkt);
#if defined(CONFIG_NET_PKT_TIMESTAMP) && defined(CONFIG_NET_PKT_TXTIME)
		net_pkt_set_timestamp_ns(pkt, rx_time * NSEC_PER_USEC);
#endif /* CONFIG_NET_PKT_TIMESTAMP && CONFIG_NET_PKT_TXTIME */
		status = net_recv_data(b9x->iface, pkt);
		if (status < 0) {
			LOG_ERR("RCV Packet dropped by NET stack: %d", status);
			if (b9x->event_handler) {
				enum ieee802154_rx_fail_reason reason =
					IEEE802154_RX_FAIL_OTHER;

				b9x->event_handler(dev, IEEE802154_EVENT_RX_FAILED,
					(void *)&reason);
			}
		}
	} while (0);

	if (status < 0 && pkt != NULL) {
		net_pkt_unref(pkt);
	}
	rf_set_rxmode();
	dma_chn_en(DMA1);
}

/* TX IRQ handler */
ALWAYS_INLINE static void b9x_rf_tx_isr(const struct device *dev)
{
	struct b9x_data *b9x = dev->data;

	/* clear irq status */
	rf_clr_irq_status(FLD_RF_IRQ_TX);

	/* set to rx mode */
	rf_set_rxmode();

	/* release tx semaphore */
	k_sem_give(&b9x->tx_wait);
}

/* IRQ handler */
__GENERIC_SECTION(.ram_code) static void b9x_rf_isr(const struct device *dev)
{
	if (rf_get_irq_status(FLD_RF_IRQ_RX)) {
		b9x_rf_rx_isr(dev);
	} else if (rf_get_irq_status(FLD_RF_IRQ_TX)) {
		b9x_rf_tx_isr(dev);
	} else {
		rf_clr_irq_status(FLD_RF_IRQ_ALL);
	}
}

volatile bool b9x_rf_zigbee_250K_mode;

ALWAYS_INLINE static int b9x_start_radio(struct b9x_data *b9x)
{
	b9x_disable_pm(b9x);
	/* check if RF is already started */
	if (!b9x->is_started) {
#ifdef CONFIG_DYNAMIC_INTERRUPTS
		irq_connect_dynamic(DT_INST_IRQN(0), DT_INST_IRQ(0, priority),
				    (void (*)(const void *))b9x_rf_isr, DEVICE_DT_INST_GET(0), 0);
#endif /* CONFIG_DYNAMIC_INTERRUPTS */
		if (!b9x_rf_zigbee_250K_mode) {
			rf_mode_init();
			rf_set_zigbee_250K_mode();
			b9x_rf_zigbee_250K_mode = true;
		}
		rf_set_rx_maxlen(144);
		rf_set_tx_dma(1, B9X_TRX_LENGTH);
		rf_set_rx_dma(b9x->rx_buffer, 0, B9X_TRX_LENGTH);
		if (b9x->current_channel != B9X_TX_CH_NOT_SET) {
			rf_set_chn(B9X_LOGIC_CHANNEL_TO_PHYSICAL(b9x->current_channel));
		}
		if (b9x->current_dbm != B9X_TX_PWR_NOT_SET) {
			rf_set_power_level(tl_tx_pwr_lt[b9x->current_dbm - TL_TX_POWER_MIN]);
		}
		rf_set_irq_mask(FLD_RF_IRQ_RX | FLD_RF_IRQ_TX);
		riscv_plic_set_priority(DT_INST_IRQN(0), DT_INST_IRQ(0, priority));
		riscv_plic_irq_enable(DT_INST_IRQN(0));
		rf_set_rxmode();
		b9x->is_started = true;
	}
	return 0;
}

ALWAYS_INLINE static int b9x_stop_radio(struct b9x_data *b9x)
{
	/* check if RF is already stopped */
	if (b9x->is_started) {
		riscv_plic_irq_disable(DT_INST_IRQN(0));
		rf_set_tx_rx_off();
#ifdef CONFIG_PM_DEVICE
		rf_baseband_reset();
		rf_reset_dma();
		b9x_rf_zigbee_250K_mode = false;
#endif /* CONFIG_PM_DEVICE */
		b9x->is_started = false;
	}
	b9x_enable_pm(b9x);

	return 0;
}

ALWAYS_INLINE static int b9x_set_channel_radio(struct b9x_data *b9x, uint16_t channel)
{
	if (channel < 11 || channel > 26) {
		return -EINVAL;
	}

	if (b9x->current_channel != channel) {
		b9x->current_channel = channel;
		if (b9x->is_started) {
			rf_set_chn(B9X_LOGIC_CHANNEL_TO_PHYSICAL(channel));
			rf_set_txmode();
			rf_set_rxmode();
		}
	}
	return 0;
}

#ifdef CONFIG_OPENTHREAD_CSL_RECEIVER

static void b9x_csl_rx_work(struct k_work *item)
{
	struct b9x_data *b9x =
		CONTAINER_OF(k_work_delayable_from_work(item), struct b9x_data, csl_rx_work);

	if (b9x->csl_rx_duration_us) {
		uint16_t current_channel = b9x->current_channel;

		(void)b9x_set_channel_radio(b9x, b9x->csl_rx_channel);
		(void)b9x_start_radio(b9x);
		(void)k_work_reschedule(&b9x->csl_rx_work, K_USEC(b9x->csl_rx_duration_us));
		b9x->csl_rx_duration_us = 0;
		b9x->csl_rx_channel = current_channel;
	} else {
		(void)b9x_stop_radio(b9x);
		(void)b9x_set_channel_radio(b9x, b9x->csl_rx_channel);
	}
}

#endif /* CONFIG_OPENTHREAD_CSL_RECEIVER */

/* Driver initialization */
static int b9x_init(const struct device *dev)
{
	struct b9x_data *b9x = dev->data;

	/* init semaphores */
	k_sem_init(&b9x->tx_wait, 0, 1);
	k_sem_init(&b9x->ack_wait, 0, 1);

	/* init IRQs */
#ifndef CONFIG_DYNAMIC_INTERRUPTS
	IRQ_CONNECT(DT_INST_IRQN(0), DT_INST_IRQ(0, priority), b9x_rf_isr, DEVICE_DT_INST_GET(0),
		    0);
#endif /* not CONFIG_DYNAMIC_INTERRUPTS */

	/* init data variables */
	b9x->is_started = false;
	b9x->ack_handler_en = false;
	b9x->ack_sending = false;
	b9x->current_channel = B9X_TX_CH_NOT_SET;
	b9x->current_dbm = B9X_TX_PWR_NOT_SET;
#ifdef CONFIG_OPENTHREAD_FTD
	b9x_src_match_table_clean(b9x->src_match_table);
	b9x->src_match_table->enabled = true;
#endif /* CONFIG_OPENTHREAD_FTD */
	b9x->event_handler = NULL;
#if !defined(CONFIG_OPENTHREAD_THREAD_VERSION_1_1)
	b9x_enh_ack_table_clean(b9x->enh_ack_table);
	b9x_mac_keys_data_clean(b9x->mac_keys);
#endif
#ifdef CONFIG_OPENTHREAD_CSL_RECEIVER
	k_work_init_delayable(&b9x->csl_rx_work, b9x_csl_rx_work);
	b9x->csl_rx_duration_us = 0;
	b9x->csl_rx_channel = B9X_TX_CH_NOT_SET;
#endif /* CONFIG_OPENTHREAD_CSL_RECEIVER */
	return 0;
}

/* API implementation: iface_init */
static void b9x_iface_init(struct net_if *iface)
{
	const struct device *dev = net_if_get_device(iface);
	struct b9x_data *b9x = dev->data;
	uint8_t *mac = b9x_get_mac(dev);

	net_if_set_link_addr(iface, mac, IEEE802154_FRAME_LENGTH_ADDR_EXT, NET_LINK_IEEE802154);

	b9x->iface = iface;

	ieee802154_init(iface);
}

/* API implementation: get_capabilities */
static enum ieee802154_hw_caps b9x_get_capabilities(const struct device *dev)
{
	ARG_UNUSED(dev);
	enum ieee802154_hw_caps caps = IEEE802154_HW_FCS |
		IEEE802154_HW_FILTER |
		IEEE802154_HW_TX_RX_ACK;

#if defined(CONFIG_NET_PKT_TIMESTAMP) && defined(CONFIG_NET_PKT_TXTIME)
	caps |= IEEE802154_HW_TXTIME;
#endif /* CONFIG_NET_PKT_TIMESTAMP && CONFIG_NET_PKT_TXTIME */
#ifdef CONFIG_OPENTHREAD_CSL_RECEIVER
	caps |= IEEE802154_HW_RXTIME;
#endif /* CONFIG_OPENTHREAD_CSL_RECEIVER */
#if !defined(CONFIG_OPENTHREAD_THREAD_VERSION_1_1)
	caps |= IEEE802154_HW_TX_SEC;
#endif
	return caps;
}

/* API implementation: cca */
static int b9x_cca(const struct device *dev)
{
	ARG_UNUSED(dev);

	signed int rssi = 0, cnt = 0;
	unsigned int t1 = stimer_get_tick();

	rf_set_rxmode();
	delay_us(85);

	do {
		rssi += rf_get_rssi();
		cnt++;
	} while (!clock_time_exceed(t1, B9X_CCA_TIME_MAX_US));
	rssi /= cnt;
	if (rssi > CONFIG_IEEE802154_B9X_CCA_RSSI_THRESHOLD) {
		return -EBUSY;
	} else {
		return 0;
	}
	return -EBUSY;
}

/* API implementation: set_channel */
static int b9x_set_channel(const struct device *dev, uint16_t channel)
{
	return b9x_set_channel_radio(dev->data, channel);
}

/* API implementation: filter */
static int b9x_filter(const struct device *dev,
		      bool set,
		      enum ieee802154_filter_type type,
		      const struct ieee802154_filter *filter)
{
	if (!set) {
		return -ENOTSUP;
	}

	if (type == IEEE802154_FILTER_TYPE_IEEE_ADDR) {
		return b9x_set_ieee_addr(dev, filter->ieee_addr);
	} else if (type == IEEE802154_FILTER_TYPE_SHORT_ADDR) {
		return b9x_set_short_addr(dev, filter->short_addr);
	} else if (type == IEEE802154_FILTER_TYPE_PAN_ID) {
		return b9x_set_pan_id(dev, filter->pan_id);
	}

	return -ENOTSUP;
}

/* API implementation: set_txpower */
static int b9x_set_txpower(const struct device *dev, int16_t dbm)
{
	struct b9x_data *b9x = dev->data;

	/* check for supported Min/Max range */
	if (dbm < TL_TX_POWER_MIN) {
		dbm = TL_TX_POWER_MIN;
	} else if (dbm > TL_TX_POWER_MAX) {
		dbm = TL_TX_POWER_MAX;
	}

	if (b9x->current_dbm != dbm) {
		b9x->current_dbm = dbm;
		/* set TX power */
		if (b9x->is_started) {
			rf_set_power_level(tl_tx_pwr_lt[dbm - TL_TX_POWER_MIN]);
		}
	}

	return 0;
}

/* API implementation: start */
static int b9x_start(const struct device *dev)
{
	return b9x_start_radio(dev->data);
}

/* API implementation: stop */
static int b9x_stop(const struct device *dev)
{
	return b9x_stop_radio(dev->data);
}

/* API implementation: tx */
#if defined(CONFIG_IEEE802154_B9X_ACTIVE_STAGE_OPTIMIZATION)
__GENERIC_SECTION(.ram_code)
#endif
static int b9x_tx(const struct device *dev, enum ieee802154_tx_mode mode, struct net_pkt *pkt,
		  struct net_buf *frag)
{
	ARG_UNUSED(pkt);

	int status = 0;
	struct b9x_data *b9x = dev->data;

	/* check for supported mode */
#if defined(CONFIG_NET_PKT_TIMESTAMP) && defined(CONFIG_NET_PKT_TXTIME)
	if (mode != IEEE802154_TX_MODE_DIRECT &&
		mode != IEEE802154_TX_MODE_TXTIME_CCA) {
#else
	if (mode != IEEE802154_TX_MODE_DIRECT) {
#endif /* CONFIG_NET_PKT_TIMESTAMP && CONFIG_NET_PKT_TXTIME */
		LOG_WRN("TX mode %d not supported", mode);
		return -ENOTSUP;
	}

#if !defined(CONFIG_OPENTHREAD_THREAD_VERSION_1_1)

	struct ieee802154_frame frame;
	uint8_t key_id = 0;

	b9x_ieee802154_frame_parse(frag->data, frag->len, &frame);

	do {

		uint8_t enc_rounds = 1;
#ifdef CONFIG_OPENTHREAD_CSL_RECEIVER
		uint8_t *ie_csl_pos = NULL;
#endif /* CONFIG_OPENTHREAD_CSL_RECEIVER */
		if (net_pkt_ieee802154_mac_hdr_rdy(pkt)) {
			if (frame.payload_ie &&
			    b9x_ieee802154_ie_csl_search(frame.payload, frame.payload_len)) {
				enc_rounds = 2;
			} else {
				LOG_WRN("The packet is encrypted and sent directly\n");
				break;
			}
		}

		net_pkt_set_ieee802154_frame_secured(pkt, false);
		net_pkt_set_ieee802154_mac_hdr_rdy(pkt, false);

		if (!frame.general.valid) {
			LOG_WRN("invalid frame\n");
			break;
		}

		if (!frame.sec_header) {
			break;
		}

		const uint8_t sec_level =
				frame.sec_header[0] & IEEE802154_FRAME_SECCTRL_SEC_LEVEL_MASK;

		if (sec_level == IEEE802154_FRAME_SECCTRL_SEC_LEVEL_0) {
			break;
		}

		net_pkt_set_ieee802154_frame_secured(pkt, true);

		const uint8_t *src_addr = frame.src_addr_ext ? frame.src_addr :
			b9x->filter_ieee_addr;

		if (!src_addr) {
			LOG_WRN("no extended source address");
			break;
		}

		switch (frame.sec_header[0] & IEEE802154_FRAME_SECCTRL_KEY_ID_MODE_MASK) {
		case IEEE802154_FRAME_SECCTRL_KEY_ID_MODE_1:
			key_id = frame.sec_header[IEEE802154_FRAME_LENGTH_SEC_HEADER_MODE_1];
			break;
		case IEEE802154_FRAME_SECCTRL_KEY_ID_MODE_2:
			key_id = frame.sec_header[IEEE802154_FRAME_LENGTH_SEC_HEADER_MODE_2];
			break;
		case IEEE802154_FRAME_SECCTRL_KEY_ID_MODE_3:
			key_id = frame.sec_header[IEEE802154_FRAME_LENGTH_SEC_HEADER_MODE_3];
			break;
		default:
			break;
		}

		if (key_id == THREAD_DEFAULT_KEY_ID_MODE_2_KEY_INDEX) {
			key_id = 0;
			break;
		}

		const uint8_t *key = b9x_mac_keys_get(b9x->mac_keys, key_id);

		if (!key) {
			key_id = 0;
			LOG_WRN("security key not found");
			break;
		}

		if (enc_rounds == 1) {
			b9x_mac_keys_frame_cnt_inc(b9x->mac_keys, key_id);
		}

		uint8_t *frame_cnt =
			(uint8_t *)&frame.sec_header[IEEE802154_FRAME_LENGTH_SEC_HEADER];
		uint32_t fc = b9x_mac_keys_frame_cnt_get(b9x->mac_keys, key_id);

		frame_cnt[0] = fc;
		frame_cnt[1] = fc >> 8;
		frame_cnt[2] = fc >> 16;
		frame_cnt[3] = fc >> 24;

		net_pkt_set_ieee802154_mac_hdr_rdy(pkt, true);

		const uint8_t tag_size[] = {4, 8, 16};

		switch (sec_level) {
		case IEEE802154_FRAME_SECCTRL_SEC_LEVEL_5:
		case IEEE802154_FRAME_SECCTRL_SEC_LEVEL_6:
		case IEEE802154_FRAME_SECCTRL_SEC_LEVEL_7:
			do {

				size_t tag_len =
					tag_size[sec_level - IEEE802154_FRAME_SECCTRL_SEC_LEVEL_5];
				const uint8_t *open_data = frame.header;
				uint8_t *private_data = (uint8_t *)frame.payload;
				uint8_t *tag_data = frame.payload ?
					(uint8_t *)&frame.payload[frame.payload_len] : NULL;

				if (private_data && tag_data &&
					tag_data - private_data >= tag_len) {
					/* Adjust tag */
					tag_data -= tag_len;
					private_data = (tag_data > private_data) ?
						private_data : NULL;
				} else {
					key_id = 0;
					LOG_WRN("invalid payload length MIC");
					break;
				}
#ifdef CONFIG_OPENTHREAD_CSL_RECEIVER
				if (b9x->csl_period > 0 && frame.payload_ie) {
					if (private_data) {
						ie_csl_pos = b9x_ieee802154_ie_csl_search(
							private_data, tag_data - private_data);
					} else {
						key_id = 0;
						LOG_WRN("failed to insert CSL IE");
						break;
					}
				}
#endif /* CONFIG_OPENTHREAD_CSL_RECEIVER */
				if (frame.payload_ie) {
					/* IE header should be open */
					if (private_data) {
						private_data = (uint8_t *)b9x_ieee802154_get_data(
							private_data, tag_data - private_data);
						private_data = (private_data &&
							tag_data > private_data) ?
								private_data : NULL;
					} else {
						key_id = 0;
						LOG_WRN("invalid payload length IE");
						break;
					}

				}

				if (frame.general.ver < IEEE802154_FRAME_FCF_VER_2015 &&
					frame.general.type == IEEE802154_FRAME_FCF_TYPE_CMD) {
					/* command id should be open
					 * if frame version less than 2015
					 */
					if (private_data) {
						private_data++;
						private_data = (private_data &&
							tag_data > private_data) ?
								private_data : NULL;
					} else {
						key_id = 0;
						LOG_WRN("invalid payload length CID");
						break;
					}
				}

				/* here open_data && tag_data - valid, private_data possible NULL */
				for (uint8_t i = 0; i < enc_rounds; i++) {
#ifdef CONFIG_OPENTHREAD_CSL_RECEIVER
					if (i == enc_rounds - 1) {
						b9x_ieee802154_ie_csl_commit_at(
							ie_csl_pos, b9x->csl_period,
							b9x_get_csl_phase(b9x));
					}
#endif /* CONFIG_OPENTHREAD_CSL_RECEIVER */
					if (!ieee802154_b9x_crypto_encrypt(
						    key, src_addr,
						    b9x_mac_keys_frame_cnt_get(b9x->mac_keys,
									       key_id),
						    sec_level, open_data,
						    private_data ? private_data - open_data
								 : tag_data - open_data,
						    private_data,
						    private_data ? tag_data - private_data : 0,
						    private_data, tag_data, tag_len)) {
						key_id = 0;
						LOG_WRN("encrypt failed %u", sec_level);
						break;
					}
				}
			} while (0);
			break;
		default:
			key_id = 0;
			LOG_WRN("unsupported security level %u", sec_level);
			break;
		}

	} while (0);

#endif

	/* prepare tx buffer */
	b9x_set_tx_payload(dev, frag->data, frag->len);

	/* reset semaphores */
	k_sem_reset(&b9x->tx_wait);
	k_sem_reset(&b9x->ack_wait);

	/* start transmission */
	rf_set_txmode();

#if defined(CONFIG_NET_PKT_TIMESTAMP) && defined(CONFIG_NET_PKT_TXTIME)
	if (mode == IEEE802154_TX_MODE_TXTIME_CCA) {
		k_sleep(K_TIMEOUT_ABS_TICKS(k_ns_to_ticks_ceil64(net_pkt_timestamp_ns(pkt))));
	} else
#endif /* CONFIG_NET_PKT_TIMESTAMP && CONFIG_NET_PKT_TXTIME */
	{
		delay_us(CONFIG_IEEE802154_B9X_SET_TXRX_DELAY_US);
	}
	rf_tx_pkt(b9x->tx_buffer);
	if (b9x->event_handler) {
		b9x->event_handler(dev, IEEE802154_EVENT_TX_STARTED, (void *)frag);
	}

	/* wait for tx done */
	if (k_sem_take(&b9x->tx_wait, K_MSEC(B9X_TX_WAIT_TIME_MS)) != 0) {
		rf_set_rxmode();
		status = -EIO;
	} else if ((frag->data[0] & IEEE802154_FRAME_FCF_ACK_REQ_MASK) ==
		   IEEE802154_FRAME_FCF_ACK_REQ_ON) {
		b9x->ack_sn = frag->data[IEEE802154_FRAME_LENGTH_FCF];
		b9x->ack_handler_en = true;
	}

	/* wait for ACK if requested */
	if (!status && b9x->ack_handler_en) {
		if (k_sem_take(&b9x->ack_wait, K_MSEC(B9X_ACK_WAIT_TIME_MS)) != 0) {
			b9x->ack_handler_en = false;
			status = -ENOMSG;
		}
	}

	return status;
}

/* API implementation: ed_scan */
static int b9x_ed_scan(const struct device *dev, uint16_t duration,
		       energy_scan_done_cb_t done_cb)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(duration);
	ARG_UNUSED(done_cb);

	/* ed_scan not supported */

	return -ENOTSUP;
}

/* API implementation: configure */
static int b9x_configure(const struct device *dev,
			 enum ieee802154_config_type type,
			 const struct ieee802154_config *config)
{
	struct b9x_data *b9x = dev->data;
	int result = 0;

	switch (type) {
#ifdef CONFIG_OPENTHREAD_FTD
	case IEEE802154_CONFIG_AUTO_ACK_FPB:
		if (config->auto_ack_fpb.mode == IEEE802154_FPB_ADDR_MATCH_THREAD) {
			b9x->src_match_table->enabled = config->auto_ack_fpb.enabled;
		} else {
			result = -ENOTSUP;
		}
		break;
	case IEEE802154_CONFIG_ACK_FPB:
		if (config->ack_fpb.addr) {
			if (config->ack_fpb.enabled) {
				b9x_src_match_table_add(b9x->src_match_table,
					config->ack_fpb.addr, config->ack_fpb.extended);
			} else {
				b9x_src_match_table_remove(b9x->src_match_table,
					config->ack_fpb.addr, config->ack_fpb.extended);
			}
		} else if (!config->ack_fpb.enabled) {
			b9x_src_match_table_remove_group(b9x->src_match_table,
				config->ack_fpb.extended);
		} else {
			result = -ENOTSUP;
		}
		break;
#endif /* CONFIG_OPENTHREAD_FTD */
#ifdef CONFIG_OPENTHREAD_CSL_RECEIVER
		case IEEE802154_CONFIG_EXPECTED_RX_TIME: {
			b9x->csl_sample_time_us = config->expected_rx_time / NSEC_PER_USEC;
		} break;
		case IEEE802154_CONFIG_RX_SLOT: {
			uint64_t rx_start_us = config->rx_slot.start / NSEC_PER_USEC;

			b9x->csl_rx_duration_us = config->rx_slot.duration / NSEC_PER_USEC;
			b9x->csl_rx_channel = config->rx_slot.channel;

			uint64_t now_us = k_ticks_to_us_near64(k_uptime_ticks());
			uint64_t delay_us = (rx_start_us > now_us) ? (rx_start_us - now_us) : 0;
			/* reduce by 1 tick, better to turn radio earlier (resolution is 1 tick) */
			delay_us =
				(delay_us > USEC_PER_SEC / CONFIG_SYS_CLOCK_TICKS_PER_SEC)
					? delay_us - USEC_PER_SEC / CONFIG_SYS_CLOCK_TICKS_PER_SEC
					: 0;
			(void)k_work_reschedule(&b9x->csl_rx_work, K_USEC(delay_us));
		} break;
		case IEEE802154_CONFIG_CSL_PERIOD: {
			b9x->csl_period = config->csl_period;
		} break;
#endif /* CONFIG_OPENTHREAD_CSL_RECEIVER */
		case IEEE802154_CONFIG_EVENT_HANDLER:
			b9x->event_handler = config->event_handler;
			break;
#if !defined(CONFIG_OPENTHREAD_THREAD_VERSION_1_1)
	case IEEE802154_CONFIG_MAC_KEYS:
		{
			uint32_t cnt = b9x->mac_keys->frame_cnt;

			b9x_mac_keys_data_clean(b9x->mac_keys);
			b9x->mac_keys->frame_cnt = cnt;
			for (size_t i = 0; config->mac_keys[i].key_value; i++) {
				if (i < B9X_MAC_KEYS_ITEMS) {
					memcpy(b9x->mac_keys->item[i].key,
						config->mac_keys[i].key_value,
						IEEE802154_CRYPTO_LENGTH_AES_BLOCK);
					b9x->mac_keys->item[i].frame_cnt_local =
					config->mac_keys[i].frame_counter_per_key;
					b9x->mac_keys->item[i].key_id =
						*config->mac_keys[i].key_id;
				} else {
					LOG_WRN("can't save key id %u",
						*config->mac_keys[i].key_id);
				}
			}
		}
		break;
	case IEEE802154_CONFIG_FRAME_COUNTER:
		b9x->mac_keys->frame_cnt = config->frame_counter;
		break;
	case IEEE802154_CONFIG_ENH_ACK_HEADER_IE:
		{
			uint8_t short_addr[IEEE802154_FRAME_LENGTH_ADDR_SHORT];
			uint8_t ext_addr[IEEE802154_FRAME_LENGTH_ADDR_EXT];

			sys_put_le16(config->ack_ie.short_addr, short_addr);
			sys_memcpy_swap(ext_addr, config->ack_ie.ext_addr,
				IEEE802154_FRAME_LENGTH_ADDR_EXT);
			if (!config->ack_ie.purge_ie) {
				if (config->ack_ie.header_ie &&
					config->ack_ie.header_ie->length) {
					b9x_enh_ack_table_add(b9x->enh_ack_table,
						short_addr, ext_addr,
						config->ack_ie.header_ie);
				} else {
					b9x_enh_ack_table_remove(b9x->enh_ack_table,
						short_addr, ext_addr);
				}

			} else {
				b9x_enh_ack_table_remove(b9x->enh_ack_table,
					short_addr, ext_addr);
			}
		}
		break;
#endif
	default:
		LOG_WRN("Unhandled cfg %d", type);
		result = -ENOTSUP;
		break;
	}

	return result;
}

/* driver-allocated attribute memory - constant across all driver instances */
IEEE802154_DEFINE_PHY_SUPPORTED_CHANNELS(drv_attr, 11, 26);

/* API implementation: attr_get */
static int b9x_attr_get(const struct device *dev, enum ieee802154_attr attr,
			struct ieee802154_attr_value *value)
{
	ARG_UNUSED(dev);

	return ieee802154_attr_get_channel_page_and_range(
		attr, IEEE802154_ATTR_PHY_CHANNEL_PAGE_ZERO_OQPSK_2450_BPSK_868_915,
		&drv_attr.phy_supported_channels, value);
}

/* API implementation: get_sch_acc */
static uint8_t b9x_get_sch_acc(const struct device *dev)
{
	ARG_UNUSED(dev);

	return CONFIG_IEEE802154_B9X_DELAY_TRX_ACC;
}

/* IEEE802154 driver APIs structure */
static const struct ieee802154_radio_api b9x_radio_api = {
	.iface_api.init = b9x_iface_init,
	.get_capabilities = b9x_get_capabilities,
	.cca = b9x_cca,
	.set_channel = b9x_set_channel,
	.filter = b9x_filter,
	.set_txpower = b9x_set_txpower,
	.start = b9x_start,
	.stop = b9x_stop,
	.tx = b9x_tx,
	.ed_scan = b9x_ed_scan,
	.configure = b9x_configure,
	.attr_get = b9x_attr_get,
	.get_sch_acc = b9x_get_sch_acc,
};


#if defined(CONFIG_NET_L2_IEEE802154)
#define L2 IEEE802154_L2
#define L2_CTX_TYPE NET_L2_GET_CTX_TYPE(IEEE802154_L2)
#define MTU 125
#elif defined(CONFIG_NET_L2_OPENTHREAD)
#define L2 OPENTHREAD_L2
#define L2_CTX_TYPE NET_L2_GET_CTX_TYPE(OPENTHREAD_L2)
#define MTU 1280
#endif

/* IEEE802154 driver registration */
#if defined(CONFIG_NET_L2_IEEE802154) || defined(CONFIG_NET_L2_OPENTHREAD)
NET_DEVICE_DT_INST_DEFINE(0, b9x_init, NULL, &data, NULL,
			  CONFIG_IEEE802154_B9X_INIT_PRIO,
			  &b9x_radio_api, L2, L2_CTX_TYPE, MTU);
#else
DEVICE_DT_INST_DEFINE(0, b9x_init, NULL, &data, NULL,
		      POST_KERNEL, CONFIG_IEEE802154_B9X_INIT_PRIO,
		      &b9x_radio_api);
#endif
