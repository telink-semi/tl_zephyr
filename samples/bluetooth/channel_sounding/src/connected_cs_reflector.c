/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 * Copyright (c) 2025 Telink Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/sys/byteorder.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/cs.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/uuid.h>

#define RAS_DBG 0

#if RAS_DBG
#define RAS_PRINTK(...) printk(__VA_ARGS__)
#else
#define RAS_PRINTK(...)
#endif

/*
 * Architecture:
 * - RAS GATT service is completely implemented in Zephyr using native Zephyr GATT API.
 * - CS physical layer (LL/PHY/RTT) is handled by Telink controller.
 * - RAS protocol (GATT characteristics, Control Point commands, notify) is handled
 *   completely by Zephyr application code.
 */

/* ── RAS Service UUIDs ─────────────────────────────────────────────── */
#define BT_UUID_RAS_SERVICE_VAL           0x185B
#define BT_UUID_RAS_FEATURE_VAL           0x2C14
#define BT_UUID_RAS_REAL_TIME_DATA_VAL    0x2C15
#define BT_UUID_RAS_ON_DEMAND_DATA_VAL    0x2C16
#define BT_UUID_RAS_CONTROL_POINT_VAL     0x2C17
#define BT_UUID_RAS_DATA_READY_VAL        0x2C18
#define BT_UUID_RAS_DATA_OVERWRITTEN_VAL  0x2C19

#define BT_UUID_RAS_SERVICE               BT_UUID_DECLARE_16(BT_UUID_RAS_SERVICE_VAL)
#define BT_UUID_RAS_FEATURE               BT_UUID_DECLARE_16(BT_UUID_RAS_FEATURE_VAL)
#define BT_UUID_RAS_REAL_TIME_DATA        BT_UUID_DECLARE_16(BT_UUID_RAS_REAL_TIME_DATA_VAL)
#define BT_UUID_RAS_ON_DEMAND_DATA        BT_UUID_DECLARE_16(BT_UUID_RAS_ON_DEMAND_DATA_VAL)
#define BT_UUID_RAS_CONTROL_POINT         BT_UUID_DECLARE_16(BT_UUID_RAS_CONTROL_POINT_VAL)
#define BT_UUID_RAS_DATA_READY            BT_UUID_DECLARE_16(BT_UUID_RAS_DATA_READY_VAL)
#define BT_UUID_RAS_DATA_OVERWRITTEN      BT_UUID_DECLARE_16(BT_UUID_RAS_DATA_OVERWRITTEN_VAL)

/* ── RAS State ──────────────────────────────────────────────────────── */
enum {
	RAS_IDX_SVC,
	RAS_IDX_FEATURE_CHAR,
	RAS_IDX_FEATURE_VAL,
	RAS_IDX_RT_DATA_CHAR,
	RAS_IDX_RT_DATA_VAL,
	RAS_IDX_RT_DATA_CCC,
	RAS_IDX_OD_DATA_CHAR,
	RAS_IDX_OD_DATA_VAL,
	RAS_IDX_OD_DATA_CCC,
	RAS_IDX_CP_CHAR,
	RAS_IDX_CP_VAL,
	RAS_IDX_CP_CCC,
	RAS_IDX_DR_CHAR,
	RAS_IDX_DR_VAL,
	RAS_IDX_DR_CCC,
	RAS_IDX_DO_CHAR,
	RAS_IDX_DO_VAL,
	RAS_IDX_DO_CCC,
	RAS_IDX_MAX,
};

/* RAS feature bitmask (Core Spec v6.0 Vol 3B 7.3) */
static uint8_t ras_feature =
	(1 << 0) |	/* realTimeProcedureDataSupport */
	(1 << 1) |	/* getLostProcedureDataSegmentsSupport */
	(1 << 2);	/* abortOperationSupport */

/* CCC enable flags */
static uint16_t ras_realtime_ccc;
static uint16_t ras_ondemand_ccc;
static uint16_t ras_controlpoint_ccc;
static uint16_t ras_dataready_ccc;
static uint16_t ras_dataoverwritten_ccc;

/* Latest procedure counter */
static uint16_t latest_procedure_counter;

/* ── CS config ─────────────────────────────────────────────────────── */
#define CS_CONFIG_ID     0
#define NUM_MODE_0_STEPS 1

/* ── Semaphores ────────────────────────────────────────────────────── */
static K_SEM_DEFINE(sem_remote_capabilities_obtained, 0, 1);
static K_SEM_DEFINE(sem_config_created, 0, 1);
static K_SEM_DEFINE(sem_cs_security_enabled, 0, 1);
static K_SEM_DEFINE(sem_procedure_done, 0, 1);
static K_SEM_DEFINE(sem_connected, 0, 1);

/* ── Connection ────────────────────────────────────────────────────── */
static struct bt_conn *connection;

/* ── RAS Ranging Data structures ───────────────────────────────────── */

#define RAS_RANGING_HEADER_LEN  4
#define RAS_SUBEVENT_HEADER_LEN 8
#define RAS_SEG_HEADER_LEN      1


#define RAS_PROCEDURE_MAX_SIZE 2000

#define RAS_PROCEDURE_SLOTS 1

struct ras_procedure_slot {
	uint8_t  data[RAS_PROCEDURE_MAX_SIZE];
	uint16_t len;
	uint16_t procedure_counter;
	uint8_t  config_id;
	bool     in_use;
};

static struct ras_procedure_slot ras_proc_slots[RAS_PROCEDURE_SLOTS];
static int ras_current_slot;
static uint16_t ras_current_procedure_counter;
static bool ras_first_subevent;

static int8_t ras_selected_tx_power;
static uint8_t ras_seg_counter;

/* ── Advertising ───────────────────────────────────────────────────── */
static const char sample_str[] = "CS RAS Reflector";

static const struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
	BT_DATA_BYTES(BT_DATA_UUID16_SOME,
		      BT_UUID_RAS_SERVICE_VAL & 0xFF,
		      (BT_UUID_RAS_SERVICE_VAL >> 8) & 0xFF),
	BT_DATA(BT_DATA_NAME_COMPLETE, sample_str, sizeof(sample_str) - 1),
};

/* Forward declaration - ras_attrs used by subevent_result_cb and GATT callbacks */
static struct bt_gatt_attr ras_attrs[];

/* ── RAS helper: build segmentation header ─────────────────────────── */
static uint8_t ras_build_seg_header(bool first_seg, bool last_seg, uint8_t seg_idx)
{
	uint8_t hdr = 0;

	if (first_seg) {
		hdr |= (1 << 0);
	}
	if (last_seg) {
		hdr |= (1 << 1);
	}
	hdr |= ((seg_idx & 0x3F) << 2);
	return hdr;
}

/* ── RAS helper: build Ranging Header ──────────────────────────────── */
static void ras_build_ranging_header(uint8_t *buf, uint16_t procedure_counter,
				     uint8_t config_id, int8_t tx_power,
				     uint8_t num_antenna_paths)
{
	uint16_t word0 = (procedure_counter & 0x0FFF) | ((config_id & 0x0F) << 12);

	buf[0] = (uint8_t)(word0 >> 0);
	buf[1] = (uint8_t)(word0 >> 8);
	buf[2] = (uint8_t)tx_power;

	buf[3] = 0;
	for (uint8_t i = 0; i < num_antenna_paths && i < 4; i++) {
		buf[3] |= (1 << i);
	}
}

/* ── RAS helper: build Subevent Header ─────────────────────────────── */
static void ras_build_subevent_header(uint8_t *buf,
				      struct bt_conn_le_cs_subevent_result *result)
{
	buf[0] = (result->header.start_acl_conn_event >> 0) & 0xFF;
	buf[1] = (result->header.start_acl_conn_event >> 8) & 0xFF;

	buf[2] = (result->header.frequency_compensation >> 0) & 0xFF;
	buf[3] = (result->header.frequency_compensation >> 8) & 0xFF;

	buf[4] = ((result->header.procedure_done_status & 0x0F) << 0) |
		 ((result->header.subevent_done_status & 0x0F) << 4);

	buf[5] = ((result->header.procedure_abort_reason & 0x0F) << 0) |
		 ((result->header.subevent_abort_reason & 0x0F) << 4);

	buf[6] = (uint8_t)result->header.reference_power_level;

	buf[7] = result->header.num_steps_reported;
}

/* ── RAS helper: convert HCI step data to RAS Subevent Data ─────── */
static uint16_t ras_convert_step_data(uint8_t *buf,
				      struct bt_conn_le_cs_subevent_result *result)
{
	struct net_buf_simple *sbuf = result->step_data_buf;
	uint8_t num_steps = result->header.num_steps_reported;
	bool subevent_aborted = (result->header.subevent_done_status ==
				 BT_CONN_LE_CS_SUBEVENT_ABORTED);
	uint8_t abort_step = result->header.abort_step;
	uint8_t *wptr = buf;
	uint16_t offset = 0;

	if (!sbuf || sbuf->len == 0 || num_steps == 0) {
		return 0;
	}

	for (uint8_t i = 0; i < num_steps; i++) {
		bool step_aborted = subevent_aborted && (i >= abort_step);

		if (offset + 3 > sbuf->len) {
			*wptr++ = 0x80;
			break;
		}

		uint8_t step_mode = sbuf->data[offset];
		uint8_t step_data_len = sbuf->data[offset + 2];

		*wptr++ = (step_mode & 0x03) | (step_aborted ? 0x80 : 0x00);

		offset += 3;

		if (!step_aborted && step_data_len > 0) {
			if (offset + step_data_len <= sbuf->len) {
				memcpy(wptr, &sbuf->data[offset], step_data_len);
				wptr += step_data_len;
			}
			offset += step_data_len;
		} else if (step_data_len > 0) {
			offset += step_data_len;
		}
	}

	return (uint16_t)(wptr - buf);
}
static uint16_t ras_build_subevent_data(uint8_t *buf,
					struct bt_conn_le_cs_subevent_result *result,
					bool is_first)
{
	uint8_t *wptr = buf;

	if (is_first) {
		ras_build_ranging_header(wptr,
					 result->header.procedure_counter,
					 result->header.config_id,
					 ras_selected_tx_power,
					 result->header.num_antenna_paths);
		wptr += RAS_RANGING_HEADER_LEN;
	}

	ras_build_subevent_header(wptr, result);
	wptr += RAS_SUBEVENT_HEADER_LEN;

	if (result->step_data_buf && result->step_data_buf->len > 0) {
		wptr += ras_convert_step_data(wptr, result);
	}

	return (uint16_t)(wptr - buf);
}

/* ── RAS data sending with work queue ──────────────────────────────── */
#define MAX_RAS_DATA_SIZE  2000
#define RAS_CHUNK_BUF_SIZE 248

static uint8_t ras_work_buf[MAX_RAS_DATA_SIZE];
static uint16_t ras_work_len;
static const struct bt_gatt_attr *ras_work_attr;
static struct bt_conn *ras_work_conn;
static bool ras_work_is_first;
static bool ras_work_is_last;
static uint8_t ras_work_seg_start;
static volatile bool ras_sending;

static struct k_work ras_send_work;
static uint8_t ras_chunk_buf[RAS_CHUNK_BUF_SIZE];

static void ras_send_work_handler(struct k_work *work)
{
	uint16_t mtu = bt_gatt_get_mtu(ras_work_conn);
	uint16_t max_payload = (mtu > 3) ? (mtu - 3) : 20;
	uint16_t offset = 0;
	uint8_t seg_idx = ras_work_seg_start;

	max_payload -= RAS_SEG_HEADER_LEN;

	while (offset < ras_work_len) {
		uint16_t remaining = ras_work_len - offset;
		uint16_t chunk_len = MIN(remaining, max_payload);
		bool first_seg = (ras_work_is_first && offset == 0);
		bool last_seg = (ras_work_is_last &&
				 offset + chunk_len >= ras_work_len);
		uint8_t seg_hdr = ras_build_seg_header(first_seg, last_seg,
						       seg_idx);
		int err;

		ras_chunk_buf[0] = seg_hdr;
		memcpy(&ras_chunk_buf[1], &ras_work_buf[offset], chunk_len);

		err = bt_gatt_notify(ras_work_conn, ras_work_attr,
				     ras_chunk_buf, 1 + chunk_len);
		if (err) {
			RAS_PRINTK("RAS: chunk fail err=%d seg=%u\n", err, seg_idx);
			break;
		}

		RAS_PRINTK("RAS: chunk ok seg=%u first=%d last=%d off=%u len=%u\n",
		       seg_idx, first_seg, last_seg, offset, chunk_len);

		offset += chunk_len;
		seg_idx = (seg_idx + 1) & 0x3F;
	}

	ras_seg_counter = seg_idx;
	RAS_PRINTK("RAS: send done, total=%u seg_counter=%u\n",
	       ras_work_len, ras_seg_counter);

	bt_conn_unref(ras_work_conn);
	ras_work_conn = NULL;
	ras_sending = false;
}

static void ras_notify_with_frag(struct bt_conn *conn,
				 const struct bt_gatt_attr *attr,
				 const void *data, uint16_t len,
				 bool is_proc_first, bool is_proc_last)
{
	if (ras_sending) {
		RAS_PRINTK("RAS: send busy, dropping data\n");
		return;
	}

	if (len > MAX_RAS_DATA_SIZE) {
		RAS_PRINTK("RAS: data too large %u > %u, truncating\n",
		       len, MAX_RAS_DATA_SIZE);
		len = MAX_RAS_DATA_SIZE;
	}

	memcpy(ras_work_buf, data, len);
	ras_work_len = len;
	ras_work_attr = attr;
	ras_work_conn = bt_conn_ref(conn);
	ras_work_is_first = is_proc_first;
	ras_work_is_last = is_proc_last;
	ras_work_seg_start = ras_seg_counter;
	ras_sending = true;

	RAS_PRINTK("RAS: submit work %u bytes first=%d last=%d\n",
	       len, is_proc_first, is_proc_last);

	k_work_submit(&ras_send_work);
}

/* ── CS subevent result ────────────────────────────────────────────── */
static uint8_t ras_subevent_buf[2000];

static void subevent_result_cb(struct bt_conn *conn,
			       struct bt_conn_le_cs_subevent_result *result)
{
	uint16_t proc_counter = result->header.procedure_counter;
	uint16_t subevent_len;
	bool is_first;

	RAS_PRINTK("RAS: subevent proc=%u cfg=%u steps=%u AP=%u done=%u\n",
	       proc_counter,
	       result->header.config_id,
	       result->header.num_steps_reported,
	       result->header.num_antenna_paths,
	       result->header.procedure_done_status);

	if (!ras_first_subevent ||
	    proc_counter != ras_current_procedure_counter) {
		ras_current_procedure_counter = proc_counter;
		ras_first_subevent = true;
		ras_seg_counter = 0;

		ras_current_slot = -1;
		for (int i = 0; i < RAS_PROCEDURE_SLOTS; i++) {
			if (!ras_proc_slots[i].in_use) {
				ras_current_slot = i;
				break;
			}
		}
		if (ras_current_slot < 0) {
			ras_current_slot = 0;
			RAS_PRINTK("RAS: overwriting oldest slot\n");
		}

		ras_proc_slots[ras_current_slot].in_use = true;
		ras_proc_slots[ras_current_slot].len = 0;
		ras_proc_slots[ras_current_slot].procedure_counter = proc_counter;
		ras_proc_slots[ras_current_slot].config_id = result->header.config_id;

		RAS_PRINTK("RAS: new procedure %u, slot %d\n", proc_counter, ras_current_slot);
	}

	is_first = ras_first_subevent;
	ras_first_subevent = false;

	subevent_len = ras_build_subevent_data(ras_subevent_buf, result, is_first);

	if (ras_current_slot >= 0) {
		struct ras_procedure_slot *slot = &ras_proc_slots[ras_current_slot];
		uint16_t new_len = slot->len + subevent_len;

		if (new_len <= RAS_PROCEDURE_MAX_SIZE) {
			memcpy(&slot->data[slot->len], ras_subevent_buf, subevent_len);
			slot->len = new_len;
		} else {
			RAS_PRINTK("RAS: slot %d overflow, dropping subevent\n", ras_current_slot);
		}
	}

	if (ras_realtime_ccc & BT_GATT_CCC_NOTIFY) {
		bool is_proc_last =
			(result->header.procedure_done_status ==
			 BT_CONN_LE_CS_PROCEDURE_COMPLETE);

		ras_notify_with_frag(conn,
				     &ras_attrs[RAS_IDX_RT_DATA_VAL],
				     ras_subevent_buf,
				     subevent_len,
				     is_first,
				     is_proc_last);
	}

	latest_procedure_counter = proc_counter;

	if (result->header.procedure_done_status == BT_CONN_LE_CS_PROCEDURE_COMPLETE) {
		RAS_PRINTK("RAS: procedure %u complete, %u bytes in slot %d\n",
		       proc_counter,
		       ras_proc_slots[ras_current_slot].len,
		       ras_current_slot);

		if (ras_dataready_ccc & BT_GATT_CCC_NOTIFY) {
			uint8_t dr_data[2];

			sys_put_le16(latest_procedure_counter & 0x0FFF, dr_data);
			bt_gatt_notify(conn,
				       &ras_attrs[RAS_IDX_DR_VAL],
				       dr_data, sizeof(dr_data));
			RAS_PRINTK("RAS: notified Data Ready procedure=%u\n",
			       latest_procedure_counter);
		}
		k_sem_give(&sem_procedure_done);
	}
}

/* ── RAS GATT callbacks ─────────────────────────────────────────────── */
static ssize_t ras_gatt_read(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			     void *buf, uint16_t len, uint16_t offset)
{
	if (attr == &ras_attrs[RAS_IDX_FEATURE_VAL]) {
		return bt_gatt_attr_read(conn, attr, buf, len, offset,
					 &ras_feature, sizeof(ras_feature));
	}
	RAS_PRINTK("RAS GATT read: handle 0x%x len=%u offset=%u\n",
	       attr->handle, len, offset);
	return 0;
}

static void ras_gatt_ccc_cfg_changed(const struct bt_gatt_attr *attr, uint16_t value)
{
	RAS_PRINTK("RAS CCC changed: handle 0x%x val=0x%04x\n", attr->handle, value);

	if (attr == &ras_attrs[RAS_IDX_RT_DATA_CCC]) {
		ras_realtime_ccc = value;
	} else if (attr == &ras_attrs[RAS_IDX_OD_DATA_CCC]) {
		ras_ondemand_ccc = value;
	} else if (attr == &ras_attrs[RAS_IDX_CP_CCC]) {
		ras_controlpoint_ccc = value;
	} else if (attr == &ras_attrs[RAS_IDX_DR_CCC]) {
		ras_dataready_ccc = value;
	} else if (attr == &ras_attrs[RAS_IDX_DO_CCC]) {
		ras_dataoverwritten_ccc = value;
	}
}

static ssize_t ras_gatt_cp_write(struct bt_conn *conn,
				 const struct bt_gatt_attr *attr,
				 const void *buf, uint16_t len,
				 uint16_t offset, uint8_t flags)
{
	const uint8_t *data = buf;
	uint8_t opcode = data[0];
	static uint8_t cp_rsp[3];

	RAS_PRINTK("RAS Control Point: opcode=0x%02x len=%u\n", opcode, len);

	switch (opcode) {
	case 0x01:
		RAS_PRINTK("  => Set RAS Configuration\n");
		break;
	case 0x02:
		RAS_PRINTK("  => RAS Control Point Command\n");
		break;
	case 0x03:
		RAS_PRINTK("  => Get Procedure Data\n");
		break;
	case 0x04:
		RAS_PRINTK("  => Abort RAS Procedure\n");
		break;
	default:
		RAS_PRINTK("  => Unknown opcode\n");
		break;
	}

	if (ras_controlpoint_ccc & BT_GATT_CCC_INDICATE) {
		static struct bt_gatt_indicate_params ind_params;

		cp_rsp[0] = opcode;
		cp_rsp[1] = 0x00; /* success */
		cp_rsp[2] = 0x00;

		ind_params.attr = (struct bt_gatt_attr *)attr;
		ind_params.data = cp_rsp;
		ind_params.len = sizeof(cp_rsp);
		ind_params.func = NULL;
		ind_params.destroy = NULL;

		bt_gatt_indicate(conn, &ind_params);
		RAS_PRINTK("  => Indication sent\n");
	}
	return len;
}

/* ── RAS GATT service (Zephyr native) ──────────────────────────────── */
static struct bt_gatt_attr ras_attrs[] = {
	BT_GATT_PRIMARY_SERVICE(BT_UUID_RAS_SERVICE),

	BT_GATT_CHARACTERISTIC(BT_UUID_RAS_FEATURE,
			       BT_GATT_CHRC_READ,
			       BT_GATT_PERM_READ,
			       ras_gatt_read, NULL, NULL),

	BT_GATT_CHARACTERISTIC(BT_UUID_RAS_REAL_TIME_DATA,
			       BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY,
			       BT_GATT_PERM_READ,
			       ras_gatt_read, NULL, NULL),
	BT_GATT_CCC(ras_gatt_ccc_cfg_changed,
		    BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),

	BT_GATT_CHARACTERISTIC(BT_UUID_RAS_ON_DEMAND_DATA,
			       BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY,
			       BT_GATT_PERM_READ,
			       ras_gatt_read, NULL, NULL),
	BT_GATT_CCC(ras_gatt_ccc_cfg_changed,
		    BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),

	BT_GATT_CHARACTERISTIC(BT_UUID_RAS_CONTROL_POINT,
			       BT_GATT_CHRC_WRITE | BT_GATT_CHRC_INDICATE,
			       BT_GATT_PERM_WRITE,
			       NULL, ras_gatt_cp_write, NULL),
	BT_GATT_CCC(ras_gatt_ccc_cfg_changed,
		    BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),

	BT_GATT_CHARACTERISTIC(BT_UUID_RAS_DATA_READY,
			       BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY,
			       BT_GATT_PERM_READ,
			       ras_gatt_read, NULL, NULL),
	BT_GATT_CCC(ras_gatt_ccc_cfg_changed,
		    BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),

	BT_GATT_CHARACTERISTIC(BT_UUID_RAS_DATA_OVERWRITTEN,
			       BT_GATT_CHRC_NOTIFY,
			       BT_GATT_PERM_NONE,
			       NULL, NULL, NULL),
	BT_GATT_CCC(ras_gatt_ccc_cfg_changed,
		    BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),
};

static struct bt_gatt_service ras_service = BT_GATT_SERVICE(ras_attrs);

/* ── MTU exchange ──────────────────────────────────────────────────── */
static void mtu_exchange_cb(struct bt_conn *conn, uint8_t err,
			    struct bt_gatt_exchange_params *params)
{
	printk("MTU exchange %s (%u)\n", err == 0U ? "success" : "failed",
	       bt_gatt_get_mtu(conn));
}

static void mtu_updated(struct bt_conn *conn, uint16_t tx, uint16_t rx)
{
	printk("MTU updated: TX=%u RX=%u\n", tx, rx);
}

static struct bt_gatt_cb gatt_callbacks = {
	.att_mtu_updated = mtu_updated,
};

/* ── Connection callbacks ──────────────────────────────────────────── */
static void connected_cb(struct bt_conn *conn, uint8_t err)
{
	char addr[BT_ADDR_LE_STR_LEN];
	int sec_err;

	(void)bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));
	printk("Connected to %s (err 0x%02X)\n", addr, err);

	__ASSERT(connection == NULL, "Unexpected connected callback");

	if (err) {
		bt_conn_unref(conn);
		connection = NULL;
		return;
	}

	connection = bt_conn_ref(conn);

	static struct bt_gatt_exchange_params mtu_exchange_params;

	mtu_exchange_params.func = mtu_exchange_cb;
	err = bt_gatt_exchange_mtu(connection, &mtu_exchange_params);
	if (err) {
		printk("MTU exchange failed (err %d)\n", err);
	}

	sec_err = bt_conn_set_security(connection, BT_SECURITY_L2);
	if (sec_err) {
		printk("Failed to request security (err %d)\n", sec_err);
		bt_conn_disconnect(connection, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
		bt_conn_unref(connection);
		connection = NULL;
		return;
	}

	k_sem_give(&sem_connected);
}

static void disconnected_cb(struct bt_conn *conn, uint8_t reason)
{
	printk("Disconnected (reason 0x%02X)\n", reason);
	bt_conn_unref(conn);
	connection = NULL;
}

static void remote_capabilities_cb(struct bt_conn *conn,
				   struct bt_conn_le_cs_capabilities *params)
{
	ARG_UNUSED(params);
	printk("CS capability exchange completed.\n");
	k_sem_give(&sem_remote_capabilities_obtained);
}

static void config_created_cb(struct bt_conn *conn, struct bt_conn_le_cs_config *config)
{
	printk("CS config creation complete. ID: %d\n", config->id);
	k_sem_give(&sem_config_created);
}

static void security_changed_cb(struct bt_conn *conn, bt_security_t level,
				 enum bt_security_err err)
{
	if (err) {
		printk("Security failed: level %u err %d\n", level, err);
		return;
	}
	printk("Security changed: level %u\n", level);
}

static void pairing_complete_cb(struct bt_conn *conn, bool bonded)
{
	printk("Pairing complete: %s\n", bonded ? "bonded" : "not bonded");
}

static void pairing_failed_cb(struct bt_conn *conn, enum bt_security_err reason)
{
	printk("Pairing failed: reason %d\n", reason);
}

static void security_enabled_cb(struct bt_conn *conn)
{
	printk("CS security enabled.\n");
	k_sem_give(&sem_cs_security_enabled);
}

static void procedure_enabled_cb(struct bt_conn *conn,
				 struct bt_conn_le_cs_procedure_enable_complete *params)
{
	printk("CS procedures %s, selected_tx_power=%d dBm\n",
	       params->state ? "enabled" : "disabled",
	       params->selected_tx_power);

	if (params->selected_tx_power != 0x7F) {
		ras_selected_tx_power = params->selected_tx_power;
	} else {
		ras_selected_tx_power = 0;
	}
}

static struct bt_conn_auth_info_cb conn_auth_info_callbacks = {
	.pairing_complete = pairing_complete_cb,
	.pairing_failed = pairing_failed_cb,
};

BT_CONN_CB_DEFINE(conn_callbacks) = {
	.connected = connected_cb,
	.disconnected = disconnected_cb,
	.security_changed = security_changed_cb,
	.le_cs_remote_capabilities_available = remote_capabilities_cb,
	.le_cs_config_created = config_created_cb,
	.le_cs_security_enabled = security_enabled_cb,
	.le_cs_procedure_enabled = procedure_enabled_cb,
	.le_cs_subevent_data_available = subevent_result_cb,
};

/* ── Main ──────────────────────────────────────────────────────────── */
int main(void)
{
	int err;

	RAS_PRINTK("Starting CS RAS Reflector Demo (Zephyr GATT)\n");

	k_work_init(&ras_send_work, ras_send_work_handler);

	err = bt_enable(NULL);
	if (err) {
		printk("Bluetooth init failed (err %d)\n", err);
		return 0;
	}

	// Load bonded keys from flash so reconnects skip re-pairing.
	if (IS_ENABLED(CONFIG_SETTINGS)) {
		settings_load();
	}

	bt_conn_auth_info_cb_register(&conn_auth_info_callbacks);
	bt_gatt_cb_register(&gatt_callbacks);

	printk("Bluetooth initialized\n");

	err = bt_gatt_service_register(&ras_service);
	if (err) {
		RAS_PRINTK("RAS service register failed (err %d)\n", err);
		return 0;
	}
	RAS_PRINTK("RAS service registered (Zephyr GATT)\n");

	err = bt_le_adv_start(BT_LE_ADV_PARAM(BT_LE_ADV_OPT_CONN,
					       BT_GAP_ADV_FAST_INT_MIN_1,
					       BT_GAP_ADV_FAST_INT_MAX_1, NULL),
			      ad, ARRAY_SIZE(ad), NULL, 0);
	if (err) {
		printk("Advertising failed to start (err %d)\n", err);
		return 0;
	}
	printk("Advertising started as '%s'\n", sample_str);

	k_sem_take(&sem_connected, K_FOREVER);

	k_sem_take(&sem_cs_security_enabled, K_FOREVER);

	const struct bt_le_cs_set_default_settings_param default_settings = {
		.enable_initiator_role = false,
		.enable_reflector_role = true,
		.cs_sync_antenna_selection = BT_LE_CS_ANTENNA_SELECTION_OPT_REPETITIVE,
		.max_tx_power = BT_HCI_OP_LE_CS_MAX_MAX_TX_POWER,
	};

	err = bt_le_cs_set_default_settings(connection, &default_settings);
	if (err) {
		printk("Failed to configure default CS settings (err %d)\n", err);
	}

	return 0;
}
