/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include "tl_common.h"
#include "stack/2p4g/tpll/tpll.h"
#include "stack/multicore_comm/service/service_d25f.h"
#include <zephyr/kernel.h>
#include "tpll_event.h"
#include <tlx_bt.h>
#include "stack/2p4g/tl_tpll/tl_tpll.h"
#define LOG_LEVEL LOG_LEVEL_DBG
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(tpll_prx);

volatile uint32_t loop;
volatile u8 debug_data[16] = {0x29, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77};


extern uint32_t hci_recv_cnt;

int main(void)
{
	int status;

	printf("Telink %s TPLL PRX Sample\n", CONFIG_BOARD_TARGET);

	mcc_d25f_register_shm_recv_cb(TLK_SHM_MSG_2P4G, pri_hci_handler_d25f);

	status = tlx_bt_controller_init();
	if (status) {
		LOG_ERR("Bluetooth controller init failed %d", status);
		return status;
	}

    uint8_t base_address_0[4] = {0x41, 0x76, 0x71, 0xe7};
    uint8_t base_address_1[4] = {0xe7, 0xe7, 0xe7, 0xe7};
    uint8_t addr_prefix[8] = {0x29, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77};

	//setting
	d25f_2p4g_set_mode(TPLL_MODE_PRX);
	d25f_2p4g_set_bitrate(TPLL_BITRATE_2MBPS);
	d25f_2p4g_apply_config();
	d25f_2p4g_set_access_code_len(ADDRESS_WIDTH_5BYTES);
	d25f_2p4g_set_access_code(TPLL_PIPE0, (u8 *)base_address_0, ADDRESS_WIDTH_4BYTES);
	d25f_2p4g_set_access_code(TPLL_PIPE1, (u8 *)base_address_1, ADDRESS_WIDTH_4BYTES);
	d25f_2p4g_set_prefixes(0x08, addr_prefix, sizeof(addr_prefix));
	d25f_2p4g_set_rf_channel(14);
	d25f_2p4g_enable_pipes(0xff);
	d25f_2p4g_set_status(TPLL_STATUS_RX_BUSY);
	d25f_2p4g_start_rx();
	irq_enable(1);


	while (1) {

		k_sem_take(&tpll_event_sem, K_FOREVER);

		if (d25f_2p4g_get_last_event() == N22_2P4G_CMD_START_RX_STA) {
			tl_tpll_payload_t *payload = d25f_2p4g_get_rx_payload();
			printf("RX data len=%d: ", payload->length);
			for (int i = 0; i < payload->length && i < 32; i++) {
				printf("%02x ", payload->data[i]);
			}
			printf("\n");
			loop++;
		}
	}

	return 0;
}