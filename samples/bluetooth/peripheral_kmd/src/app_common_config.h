/** @file app_common_config.h
 *  @brief
 */

/*
 * Copyright (c) 2025 Telink Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/////////////////////////////////////////////////////////////////////////

#define TLK_ERR_BASE_NUM                                    (0x0)    /**< Global error base */

#define TLK_SUCCESS                                         (TLK_ERR_BASE_NUM + 0)    /**< Successful command */
#define TLK_ERR_NULL                                        (TLK_ERR_BASE_NUM + 1)    /**< Null pointer */
#define TLK_ERR_INVALID_PARAM                               (TLK_ERR_BASE_NUM + 2)    /**< Invalid parameter */
#define TLK_ERR_BUSY                                        (TLK_ERR_BASE_NUM + 3)    /**< Device or resourse Busy */
#define TLK_ERR_INVALID_STATE                               (TLK_ERR_BASE_NUM + 4)    /**< Invalid state, operation disallowed in this state */
#define TLK_ERR_BUFFER_EMPTY                                (TLK_ERR_BASE_NUM + 5)    /**< Buffer/FIFO is empty */
#define TLK_ERR_NO_MEM                                      (TLK_ERR_BASE_NUM + 6)    /**< No memory available for operation */
#define TLK_ERR_INVALID_LENGTH                              (TLK_ERR_BASE_NUM + 7)    /**< Invalid length */
#define TLK_ERR_TIMEOUT                                     (TLK_ERR_BASE_NUM + 8)    /**< Operation timed out */
#define TLK_ERR_INTERNAL                                    (TLK_ERR_BASE_NUM + 9)    /**< Internal error */
#define TLK_ERR_DEV_NOT_FOUND                               (TLK_ERR_BASE_NUM + 10)   /**< Destination device not found */
#define TLK_ERR_CMD_NOT_SUPPORT                             (TLK_ERR_BASE_NUM + 11)   /**< cmd not supported */
#define TLK_ERR_BUFFER_FULL                                 (TLK_ERR_BASE_NUM + 12)   /**< Buffer/FIFO is full */


/////////////////////////////////////////////////////////////////////////
#define TLKAPI_DEBUG_ENABLE         0


typedef enum {
    KB_MODE_2P4G                        =   0,
    KB_MODE_BLE                         =   1,
    KB_MODE_USB                         =   2,
} kb_mode_t;


typedef enum {

    P24G_SM_CMD_PAIRING                 =   0x00,
    P24G_SM_CMD_LL_CONTROL,
    P24G_SM_CMD_MOUSE_DRAW,
    P24G_SM_CMD_SET_STATE,
    P24G_SM_CMD_SAVE_PAIR_INFO,
    P24G_SM_CMD_SET_KB_MODE,
    P24G_SM_CMD_DATA_TYPE_SPP,
    P24G_SM_CMD_MISC,
    P24G_SM_CMD_REPORT_RATE_CHANGE,
    P24G_SM_CMD_MAX,
    P24G_SM_CMD_NONE                    =   0xFF,

} p24g_sm_cmd_e;
 
typedef enum {
    P24G_SM_OP_NONE                     =   0x00,
    P24G_SM_OP_TERMINATE_CONN           =   0x51,
    P24G_SM_OP_ENTER_RF_IDLE            =   0x52,
    P24G_SM_OP_MISC_TRANS_MAC           =   0x53,
    P24G_SM_OP_MISC_PEER_INFO           =   0x54,
    P24G_SM_OP_ENABLE_RECONN            =   0x55,
    P24G_SM_OP_MISC_STOP_STIMER         =   0x56,
    P24G_SM_OP_MISC_REPORT_RATE         =   0x57,
    P24G_SM_OP_MISC_SAVE_REPORT_RATE    =   0x58,
    P24G_SM_OP_MISC_RF_MODE             =   0x59,
    P24G_SM_OP_MISC_SUSP_RET            =   0x5A,
    P24G_SM_OP_MISC_LONG_SUSP_RET       =   0x5B,
} p24g_sm_op_e;


typedef enum {
    P24G_KB_MODE_USB                    =   0x00,
    P24G_KB_MODE_2P4G                   =   0x01,

    DEVICE_TYPE_KB                      =   BIT(0),
    DEVICE_TYPE_MS                      =   BIT(1),

} p24G_enum_Type;

