#include "tl_common.h"
#include "stack/multicore_comm/service/service_d25f.h"
#include "stack/2p4g/tl_tpll/tl_tpll.h"
#include "tpll_event.h"

volatile uint32_t hci_recv_cnt = 0,hci_recv_n22_cnt = 0;
static volatile uint8_t tpll_status = TPLL_STATUS_IDLE;

K_SEM_DEFINE(tpll_event_sem, 0, 1);

static volatile d25f_2p4g_cmd_e last_event;
static tl_tpll_payload_t last_rx_payload;


void pri_hci_handler_d25f(u8 *p, unsigned short int n)
{
	// printf("hci_recv_cnt %d, n %d, d %x  %x\n", hci_recv_cnt, n, p[0], p[1]);


    u8 cmd       = p[1];
    u8 param_len = p[2];
    u8 *param    = &p[3];
    if ((n - D25F_2P4G_CMD_HEADER_LEN) < param_len) {
        return -1;
    }

    switch (cmd)
    {
    case N22_2P4G_CMD_START_RX_STA:
		hci_recv_cnt++;
		last_event = N22_2P4G_CMD_START_RX_STA;
		if (param_len >= sizeof(tl_tpll_payload_t)) {
			memcpy(&last_rx_payload, param, sizeof(tl_tpll_payload_t));
		}
		k_sem_give(&tpll_event_sem);

		hci_recv_n22_cnt = *((uint32_t *)&last_rx_payload.data[188]);
        break;

    case N22_2P4G_CMD_START_TX_STA:
		last_event = N22_2P4G_CMD_START_TX_STA;
		k_sem_give(&tpll_event_sem);
		d25f_2p4g_set_status(TPLL_STATUS_IDLE);
        break;

    default:
        break;
    }


}

static int d25f_2p4g_send_cmd(d25f_2p4g_cmd_e cmd, const u8 *param, u8 param_len)
{
	u8 buf[D25F_2P4G_CMD_HEADER_LEN + 255];

	buf[0] = HCI_TYPE_CMD;
	buf[1] = (u8)cmd;
	buf[2] = param_len;
	if (param && param_len) {
		memcpy(&buf[3], param, param_len);
	}
	return (mcc_d25f_pri_send_msg(buf, D25F_2P4G_CMD_HEADER_LEN + param_len) == SHM_FIFO_SUCCESS)
		       ? 0 : -1;
}

int d25f_2p4g_set_mode(tpll_rf_mode_t mode)
{
	u8 param = (u8)mode;
	return d25f_2p4g_send_cmd(N22_2P4G_CMD_SET_MODE, &param, 1);
}

int d25f_2p4g_set_bitrate(uint8_t bitrate)
{
	u8 param = bitrate;
	return d25f_2p4g_send_cmd(N22_2P4G_CMD_SET_BITRATE, &param, 1);
}

int d25f_2p4g_set_retransmit(uint16_t count, uint16_t delay_us)
{
	u8 param[4];

	param[0] = count & 0xFF;
	param[1] = (count >> 8) & 0xFF;
	param[2] = delay_us & 0xFF;
	param[3] = (delay_us >> 8) & 0xFF;
	return d25f_2p4g_send_cmd(N22_2P4G_CMD_SET_RETRANSMIT, param, 4);
}

int d25f_2p4g_write_payload(uint8_t pipe, uint8_t *data, uint8_t len)
{
	u8 param[256];

	if (data == NULL || len == 0 || len > 255) {
		return -1;
	}
	param[0] = pipe;
	memcpy(&param[1], data, len);
	return d25f_2p4g_send_cmd(N22_2P4G_CMD_WRITE_PAYLOAD, param, 1 + len);
}

int d25f_2p4g_set_access_code_len(uint8_t length)
{
	u8 param = length;
	return d25f_2p4g_send_cmd(N22_2P4G_CMD_SET_ACCESS_CODE_LEN, &param, 1);
}

int d25f_2p4g_set_access_code(uint8_t pipe, uint8_t *addr, uint8_t len)
{
	u8 param[256];

	if (addr == NULL || len == 0 || len > 255) {
		return -1;
	}
	param[0] = pipe;
	memcpy(&param[1], addr, len);
	return d25f_2p4g_send_cmd(N22_2P4G_CMD_SET_ACCESS_CODE, param, 1 + len);
}

int d25f_2p4g_set_rf_channel(uint8_t channel)
{
	u8 param = channel;
	return d25f_2p4g_send_cmd(N22_2P4G_CMD_SET_RF_CHANNEL, &param, 1);
}

int d25f_2p4g_set_prefixes(uint8_t pipe_count, uint8_t *prefixes, uint8_t len)
{
	u8 param[256];

	if (prefixes == NULL || len == 0 || len > 255) {
		return -1;
	}
	param[0] = pipe_count;
	memcpy(&param[1], prefixes, len);
	return d25f_2p4g_send_cmd(N22_2P4G_CMD_SET_PREFIXES, param, 1 + len);
}

int d25f_2p4g_enable_pipes(uint8_t enable_mask)
{
	u8 param = enable_mask;
	return d25f_2p4g_send_cmd(N22_2P4G_CMD_ENABLE_PIPES, &param, 1);
}

int d25f_2p4g_apply_config(void)
{
	return d25f_2p4g_send_cmd(N22_2P4G_CMD_APPLY_CONFIG, NULL, 0);
}


int d25f_2p4g_start_tx(void)
{
	return d25f_2p4g_send_cmd(N22_2P4G_CMD_START_TX, NULL, 0);
}

int d25f_2p4g_start_rx(void)
{
	return d25f_2p4g_send_cmd(N22_2P4G_CMD_START_RX, NULL, 0);
}

void d25f_2p4g_set_status(uint8_t status)
{
	tpll_status = status;
}


uint8_t d25f_2p4g_get_status(void)
{
	return tpll_status;
}

d25f_2p4g_cmd_e d25f_2p4g_get_last_event(void)
{
	return last_event;
}

tl_tpll_payload_t *d25f_2p4g_get_rx_payload(void)
{
	return &last_rx_payload;
}