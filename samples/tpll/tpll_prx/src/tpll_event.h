#ifndef _TPLL_EVENT_H_
#define _TPLL_EVENT_H_

#include <zephyr/kernel.h>
#include "tl_common.h"
#include "stack/2p4g/tl_tpll/tl_tpll.h"

/**
 * @brief       Dual-core 2.4G HCI command protocol definitions.
 *
 * Command packet format (Host core -> N22 core, via shared memory):
 *   byte 0: HCI_TYPE_CMD (0x01)      - type tag
 *   byte 1: cmd                         - sub-command opcode (@ref n22_2p4g_cmd_e)
 *   byte 2: param_len                   - parameter length (excluding header)
 *   byte 3..: param                     - parameter data
 *
 * Response packet format (N22 core -> Host core):
 *   byte 0: HCI_TYPE_CMD (0x01)
 *   byte 1: cmd                         - echoed sub-command opcode
 *   byte 2: status                      - execution status (0 = success, others = error code)
 */
#define D25F_2P4G_CMD_HEADER_LEN     3   /* HCI_TYPE_CMD + cmd + param_len */

typedef enum {
    N22_2P4G_CMD_SET_MODE           = 0x01, /* param: mode(1B) */
    N22_2P4G_CMD_SET_BITRATE        = 0x02, /* param: bitrate(1B) */
    N22_2P4G_CMD_SET_RETRANSMIT     = 0x03, /* param: count(2B LE) + delay_us(2B LE) */
    N22_2P4G_CMD_WRITE_PAYLOAD      = 0x04, /* param: pipe(1B) + data(NB) */
    N22_2P4G_CMD_SET_ACCESS_CODE_LEN= 0x05, /* param: length(1B) */
    N22_2P4G_CMD_SET_ACCESS_CODE    = 0x06, /* param: pipe(1B) + addr(NB) */
    N22_2P4G_CMD_SET_RF_CHANNEL     = 0x07, /* param: channel(1B) */
    N22_2P4G_CMD_SET_PREFIXES       = 0x08, /* param: num_pipes(1B) + prefixes(NB) */
    N22_2P4G_CMD_ENABLE_PIPES       = 0x09, /* param: enable_mask(1B) */
    N22_2P4G_CMD_APPLY_CONFIG       = 0x0A, /* param: none */
    N22_2P4G_CMD_START_RX           = 0x0B, /* param: none */
    N22_2P4G_CMD_START_TX           = 0x0C, /* param: none */
    N22_2P4G_CMD_START_RX_STA       = 0x0D, /* param: none */
    N22_2P4G_CMD_START_TX_STA       = 0x0E, /* param: none */
} d25f_2p4g_cmd_e;

/* TPLL status values returned by d25f_2p4g_get_status() */
#define TPLL_STATUS_IDLE        0x00
#define TPLL_STATUS_TX_BUSY     0x01
#define TPLL_STATUS_RX_BUSY     0x02




typedef enum {
	TPLL_RF_MODE_IDLE = 0,
	TPLL_RF_MODE_TX,
	TPLL_RF_MODE_RX,
} tpll_rf_mode_t;

typedef enum {
	TPLL_RF_CMD_MODE = 0x01,
	TPLL_RF_CMD_FREQ = 0x02,
	TPLL_RF_CMD_CHANNEL = 0x03,
	TPLL_RF_CMD_TX = 0x04,
} tpll_rf_cmd_t;

void pri_hci_handler_d25f(u8 *p, unsigned short int n);

int d25f_2p4g_set_mode(tpll_rf_mode_t mode);
int d25f_2p4g_set_bitrate(uint8_t bitrate);
int d25f_2p4g_set_retransmit(uint16_t count, uint16_t delay_us);
int d25f_2p4g_write_payload(uint8_t pipe, uint8_t *data, uint8_t len);
int d25f_2p4g_set_access_code_len(uint8_t length);
int d25f_2p4g_set_access_code(uint8_t pipe, uint8_t *addr, uint8_t len);
int d25f_2p4g_set_rf_channel(uint8_t channel);
int d25f_2p4g_set_prefixes(uint8_t pipe_count, uint8_t *prefixes, uint8_t len);
int d25f_2p4g_enable_pipes(uint8_t enable_mask);
int d25f_2p4g_apply_config(void);
int d25f_2p4g_start_rx(void);
int d25f_2p4g_start_tx(void);
uint8_t d25f_2p4g_get_status(void);
void d25f_2p4g_set_status(uint8_t status);

/** Semaphore signaled on N22_2P4G_CMD_START_RX_STA or N22_2P4G_CMD_START_TX_STA */
extern struct k_sem tpll_event_sem;

/** Returns the last event type (N22_2P4G_CMD_START_RX_STA / N22_2P4G_CMD_START_TX_STA) */
d25f_2p4g_cmd_e d25f_2p4g_get_last_event(void);

/** Returns pointer to the last received RX payload (valid only after RX_STA event) */
tl_tpll_payload_t *d25f_2p4g_get_rx_payload(void);

#endif /* _TPLL_EVENT_H_ */