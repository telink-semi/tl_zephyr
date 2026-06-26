/** @file app_2p4g.h
 *  @brief
 */

/*
 * Copyright (c) 2025 Telink Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifdef __cplusplus
extern "C" {
#endif

#ifndef __APP_2P4G_H__
#define __APP_2P4G_H__

#include "app_common_config.h"

#define KM_SPP_MAX_LEN                                  (64)

typedef void (*p24g_sm_cmd_handler_t)(uint8_t *data, uint16_t len);

/* Generic event header */
typedef struct p24g_evt
{
    uint8_t type;
    uint8_t opcode;
    uint8_t len;
    uint8_t data[0];
} __attribute__((packed)) p24g_evt_t;


typedef struct
{
    uint16_t peer_tag;
    uint8_t paired_dev_type;
    uint8_t rr_idx;
    
    bool save_rr_flag;
    bool rr_chg_flag;
    bool report_rate_set_flag;
    uint8_t dev_status;             /* tpsll_dev_status_e */
    uint8_t report_rate;            /* report_rate_t */

    uint8_t mac[MAC_ADDR_LEN];
    uint8_t peer_mac[MAC_ADDR_LEN];
} app_ctx_t;

extern app_ctx_t app_ctx;

typedef struct
{
    bool is_stk_busy;
} app_dual_core_flag_ctx_t;

extern app_dual_core_flag_ctx_t app_dual_core_flag_ctx;


/**
 * @brief   Register a shared memory (SM) command handler for the 2.4GHz protocol
 *
 * This function registers a handler function for a specified SM command in the
 * 2.4GHz protocol stack. When the given command is received through the SM
 * (shared memory) communication channel, the registered handler will be invoked
 * with the associated data buffer.
 *
 * @param[in] cmd       Command identifier of type @ref p24g_sm_cmd_e
 * @param[in] handler   Callback function pointer of type @ref p24g_sm_cmd_handler_t
 *
 * @return   0: success
 *           Other: fail
 *
 * @note     If a handler for the same command is already registered, it will
 *           be overwritten by the new handler.
 */
uint8_t p24g_register_sm_cmd_handler(p24g_sm_cmd_e cmd, p24g_sm_cmd_handler_t handler);


/**
 * @brief     Shared memory (SM) receive callback for 2.4GHz D25F core
 *
 * This callback function is invoked when the shared memory (SM) interface
 * receives data from another MCU core in an dual-core system. It is
 * part of the inter-core communication mechanism used by the 2.4GHz protocol
 * stack running on the d25f core.
 *
 * @param[in] data  Pointer to the received data buffer
 * @param[in] len   Length of the received data in bytes
 *
 * @note      The buffer pointed by @p data is only valid during the callback
 *            execution. If the data needs to be stored for later processing,
 *            it must be copied to a safe memory area before the callback returns.
 *
 * @return    None
 */
void app_2p4g_d25f_sm_rx_cb(uint8_t *data, uint16_t len);


/**
 * @brief     Send a shared memory (SM) message in the 2.4GHz protocol
 *
 * This function sends a message to the 2.4GHz protocol layer through the
 * shared memory (SM) interface. The SM interface is used for inter-core
 * communication in dual-core systems, enabling fast and low-latency data
 * exchange between the application processor d25f and the RF protocol core n22.
 *
 * @param[in] type  Message type identifier
 * @param[in] op    Message operation code
 * @param[in] data  Pointer to the message data buffer
 * @param[in] len   Length of the message data in bytes
 * 
 * @return    0 if sending is successful, non-zero error code if failed
 *
 * @note      The buffer pointed by @p data must remain valid until the message
 *            is fully copied or processed by the receiving core.
 *
 */
uint8_t p24g_send_sm_msg(uint8_t type, uint8_t op,  uint8_t *data, uint8_t len);



/**
 * @brief     Initialize 2.4GHz application module
 *
 * This function performs initialization of the 2.4GHz application layer,
 * including RF configuration, state variables setup, and registration
 * of necessary callbacks. It should be called once during system startup
 * before any 2.4GHz communication begins.
 *
 * @param[in]  None
 *
 * @return     None
 *
 * @note       Must be called before entering the 2.4GHz main loop or
 *             handling any RF-related events.
 */
void app_2p4g_init(void);



/**
 * @brief     2.4GHz main loop
 *
 * This function runs the main execution loop of the 2.4GHz protocol.
 * It handles packet transmission, reception, and state transitions.
 * The function should be called repeatedly in the system main loop
 * to maintain RF communication and process queued events.
 *
 * @param[in]  None
 *
 * @return     None
 *
 * @note       This function is non-blocking and should be invoked
 *             periodically within the main system loop.
 */
void app_2p4g_main_loop(void);

/**
 * @brief     2.4GHz mailbox callback for keyboard/mouse data
 *
 * This function is invoked when keyboard or mouse data is received
 * through the 2.4GHz mailbox channel. The received data is provided
 * through the input buffer for further processing or forwarding to
 * higher layers.
 *
 * @param[in]  data   Pointer to the received data buffer
 *
 * @return     None
 *
 * @note       This function should be registered as the mailbox callback
 *             for keyboard/mouse data reception in the 2.4GHz stack.
 */
void app_2p4g_mb_km_data_cb(uint8_t* data);


/**
 * @brief     Get the current 2.4GHz device state
 *
 * This function returns the current operating state of the 2.4GHz device,
 * which indicates the RF or connection status such as idle, connected,
 * or disconnected.
 *
 * @return    Current device state of type @ref tpsll_dev_status_e
 *
 * @note      Typically used to check the current communication or RF state
 *            in higher-level application logic.
 */
static inline tpsll_dev_status_e app_d24p_get_state(void)
{
    return app_ctx.dev_status;
}

uint8_t p24g_change_report_rate(report_rate_t report_rate);

void tlk_d25f_to_n22_mode_info(kb_mode_t mode_flag);

void p24g_user_init_normal(void);

#endif // __APP_2P4G_H__

#ifdef __cplusplus
}
#endif
