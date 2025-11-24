/** @file
 *  @brief app_24g.h
 */

/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __APP_2P4G_H__
#define __APP_2P4G_H__

#include "app_common_config.h"

typedef void (*p24g_sm_cmd_handler_t)(uint8_t *data, uint16_t len);

// enum {
//     EMPTY_DATA_CMD=0,
//     PAIR_DATA_CMD=1,
//     RECONNECT_DATA_CMD=2,
//     MOUSE_DATA=3,
//     SPP_DATA=4,
//     SPP_DATA_ACK=5,
//     NORMAL_KB_DATA_CMD=6,
//     CONSUME_KB_DATA_CMD=7,
//     SYSTEM_KB_DATA_CMD=8,
//     ALL_KB_DATA_CMD=9,
// };

/* Definition of mailbox 2.4g packet types */
typedef enum {
    P24G_MB_CMD_NONE = 0x00,
    P24G_MB_CMD_USB  = 0x01,
} p24g_mailbox_cmd_e;

typedef enum {
    P24G_USB_OP_EP_WRITE = 0x00,
    P24G_USB_OP_EP_READ  = 0x01,
} p24g_usb_opcode_e;

typedef enum
{
    CLOCK_CONFIG_1V1_192_96  = 0,
    CLOCK_CONFIG_1V1_96_96,
    CLOCK_CONFIG_1V1_48_48,
    CLOCK_CONFIG_1V_72_36,
    CLOCK_CONFIG_1V_64_32,
    CLOCK_CONFIG_1V_48_24,
} app_clock_config_e;

/* Generic event header */
typedef struct p24g_evt
{
    uint8_t type;
    uint8_t opcode;
    uint8_t len;
    uint8_t data[0];
} __attribute__((packed)) p24g_evt_t;

/* USB packet header */
typedef struct p24g_usb_packet_header
{
    uint8_t ep_addr;
    uint8_t len;
    uint8_t data[0];
} __attribute__((packed)) p24g_usb_pkt_t;

extern volatile p24g_device_status_e g_state;


typedef struct {
    uint8_t rf_mode;
    uint8_t mac[MAC_ADDR_LEN];
} app_ctx_t;
extern app_ctx_t app_ctx;

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
void app_2p4g_d25f_sm_rx_cb(uint8_t *data, uint32_t len);


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
 * @brief     Send SPP (Serial Port Profile) data through 2.4GHz protocol
 *
 * This function sends SPP-format data via the 2.4GHz protocol layer to
 * the connected peer device. SPP data is typically used for generic data
 * transmission (non-HID) between two devices over the 2.4GHz link.
 *
 * @param[in] cmd   SPP data command identifier
 * @param[in] data  Pointer to the SPP data buffer
 * @param[in] len   Length of the SPP data in bytes
 *
 * @return    0 if sending is successful, non-zero error code if failed
 *
 */
uint8_t p24g_send_spp_data(uint8_t cmd, unsigned char *data, unsigned char len);

/**
 * @brief     Enable or disable pairing mode for the 2.4GHz protocol
 *
 * This function enables or disables the pairing mode in the 2.4GHz protocol stack.
 * When pairing mode is enabled, the device will search for and allow connections
 * from compatible peer devices. When disabled, pairing requests will be ignored.
 *
 * @param[in] enable  true to enable pairing mode, false to disable pairing mode
 *
 * @return    0 if the operation is successful, non-zero error code if failed
 *
 * @note      This API only controls the pairing state in the protocol stack.
 * 
 */
uint8_t p24g_enable_pairing(bool enable);


/**
 * @brief     Terminate the current 2.4GHz connection
 *
 * This function is used to actively terminate the existing 2.4GHz wireless
 * connection. It can be called by the application layer when the device
 * needs to disconnect from the host or exit the current session.
 *
 * After a successful disconnection, the device status callback will be invoked
 * with @ref STATE_DISCONNECTED, and the disconnection reason will be reported
 * as @ref P24G_LL_CONN_TERMINATION_BY_LOCAL.
 *
 * @param     None
 *
 * @return    0: success  
 *            Other: failure
 *
 * @note      A reconnection procedure may be required if communication
 *            is needed again.
 */
uint8_t p24g_terminate_connect(void);


/**
 * @brief     Put the 2.4GHz RF module into idle state
 *
 * This function actively transitions the 2.4GHz RF module to idle state,
 * stopping ongoing RF communication and freeing RF resources.
 *
 * After a successful operation, the device status callback will be invoked
 * with @ref STATE_RF_IDLE, indicating that the RF module is now in idle state.
 *
 * @param     None
 *
 * @return    0: success  
 *            Other: failure
 *
 * @note      Use this function when the application needs to temporarily stop
 *            RF communication or release RF resources. Communication must be
 *            re-enabled or reconnected if needed again.
 */
uint8_t p24g_rf_enter_idle(void);


/**
 * @brief     Enable or disable the 2.4GHz automatic reconnection feature
 *
 * This function enables or disables the automatic reconnection mechanism
 * in the 2.4GHz protocol stack. 
 *
 * @param[in] enable    true: enable reconnection  
 *                      false: disable reconnection
 *
 * @return    0: success  
 *            Other: failure (e.g., invalid parameter or operation not allowed)
 *
 * @note      This function should be called after the device has been paired.
 * 
 */
uint8_t p24g_enable_reconn(bool enable);


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
 * @brief     Timer1 interrupt service routine
 *
 * This function is the interrupt handler for Timer1. It is typically used
 * for time-critical tasks such as RF scheduling, connection supervision,
 * or periodic event triggering related to the 2.4GHz communication stack.
 *
 * @param[in]  None
 *
 * @return     None
 *
 * @note       This function must be registered as the Timer1 ISR and should
 *             execute as quickly as possible to avoid interrupt latency.
 */
void app_timer1_irq_handler(void);

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
 * @return    Current device state of type @ref p24g_device_status_e
 *
 * @note      Typically used to check the current communication or RF state
 *            in higher-level application logic.
 */
static inline p24g_device_status_e app_d24p_get_state(void)
{
    return g_state;
}

/**
 * @brief     Set the current 2.4GHz device state
 *
 * This function updates the internal state variable that represents
 * the 2.4GHz device’s current operating status. It is mainly used by
 * internal modules to synchronize the system state after major events
 * such as connection, disconnection, or idle transitions.
 *
 * @param[in] state   New device state of type @ref p24g_device_status_e
 *
 * @return    None
 *
 * @note      This function is intended for internal use only. Application
 *            modules should trigger state transitions via higher-level APIs.
 */
static inline void app_d24p_set_state(p24g_device_status_e state)
{
    g_state = state;
}

/**
 * @brief     Adjust 2.4GHz clock settings according to stack usage scenario
 *
 * This function switches or reconfigures the 2.4GHz RF clock settings based
 * on the current usage scenario of the 2.4G protocol stack. Depending on the
 * stack state and requirements it may:
 *  - select a high-precision clock source or increase clock frequency for
 *    timing-critical modes (e.g. continuous TX/RX, high-rate transfers);
 *  - select a low-power / gated clock configuration for idle or low-activity
 *    modes to save power;
 *  - apply timing/phase adjustments required after mode transitions (wake-up,
 *    role change, channel change, etc.).
 *
 * The function should be called whenever the stack transitions between modes
 * that have different timing/accuracy or power requirements (for example:
 * entering/exiting active RF operation, switching from idle to heavy TX/RX,
 * after wakeup from deep sleep, or when preparing for connection procedures).
 *
 * @param[in]  None
 *
 * @return     None
 *
 * @note      This routine must ensure clock changes are performed safely:
 *            synchronize with ongoing RF operations, avoid abrupt clock changes
 *            during packet transmission/reception, and reconfigure/handover
 *            hardware PLLs or clock dividers as required by the platform.
 */
void app_2p4g_clock_reover(void);

void tlk_d25f_to_n22_mode_info(kb_mode_t mode_flag);

/**
 * @brief       user initialization when MCU power on or wake_up from deepSleep mode
 * @param[in]   none
 * @return      none
 */
void p24g_user_init_normal(void);
void app_clock_init(app_clock_config_e select);
#ifdef __cplusplus
extern "C" {
#endif


#ifdef __cplusplus
}
#endif

#endif // __APP_2P4G_H__