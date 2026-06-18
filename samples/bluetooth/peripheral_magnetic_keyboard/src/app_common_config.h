/********************************************************************************************************
 * @file    app_common_config.h
 *
 * @brief   This is the header file for BLE SDK
 *
 * @author  BLE GROUP
 * @date    2026
 *
 * @par     Copyright (c) 2026, Telink Semiconductor (Shanghai) Co., Ltd. ("TELINK")
 *
 *          Licensed under the Apache License, Version 2.0 (the "License");
 *          you may not use this file except in compliance with the License.
 *          You may obtain a copy of the License at
 *
 *              http://www.apache.org/licenses/LICENSE-2.0
 *
 *          Unless required by applicable law or agreed to in writing, software
 *          distributed under the License is distributed on an "AS IS" BASIS,
 *          WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *          See the License for the specific language governing permissions and
 *          limitations under the License.
 *
 *******************************************************************************************************/


///////////////////////// flash address Configuration////////////////////////////////////////////////
#define P24G_OTHER_INF_FLASH_ADDR_1M  0xFE000    //4K
#define P24G_PAIR_INF_FLASH_ADDR_1M   0xFD000    //4K
#define BLE_APP_PIPE_FLASH_ADDR_1M    0xFC000    //4K
#define BLE_APP_SMP_FLASH_ADDR_1M     0xFB000    //4K
#define BLE_STACK_SMP_FLASH_ADDR_1M   0xF7000    //16K 0xF7000~0xFAFFF

#define P24G_OTHER_INF_FLASH_ADDR_2M  0x1FE000    //4K
#define P24G_PAIR_INF_FLASH_ADDR_2M   0x1FD000    //4K
#define BLE_APP_PIPE_FLASH_ADDR_2M    0x1FC000    //4K
#define BLE_APP_SMP_FLASH_ADDR_2M     0x1FB000    //4K
#define BLE_STACK_SMP_FLASH_ADDR_2M   0x1F7000    //16K 0x3F7000~0x3FAFFF


#define P24G_OTHER_INF_FLASH_ADDR_4M  0x3FE000    //4K
#define P24G_PAIR_INF_FLASH_ADDR_4M   0x3FD000    //4K
#define BLE_APP_PIPE_FLASH_ADDR_4M    0x3FC000    //4K
#define BLE_APP_SMP_FLASH_ADDR_4M     0x3FB000    //4K
#define BLE_STACK_SMP_FLASH_ADDR_4M   0x3F7000    //16K 0x3F7000~0x3FAFFF


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

#define SPP_TX_FIFO_SIZE            32
#define SPP_TX_FIFO_SIZE_KB         24 //In 8K mode, the spp buffer size cannot exceed 24 on kb side.


typedef enum {
    APP_D24G_MODE                        =   0,
    APP_BLE_MODE                         =   1,
    APP_WIRED_USB_MODE                   =   2,
} kb_mode_t;


typedef enum
{
    STATE_POWERON                       =   0,
    STATE_PAIRING,
    STATE_RECONNECT,
    STATE_NORMAL,
    STATE_CONNECTED,
    STATE_DISCONNECTED,
    STATE_RF_IDLE,
    STATE_PAIRING_TIMEOUT,
    STATE_NONE,
    STATE_BUSY,
    STATE_TO_IDLE,
    STATE_IDLE,
    STATE_TO_SLEEP,
    STATE_SLEEP,

    STATE_PAIR_NONE                     =   0,
    STATE_PAIR_INIT,
    STATE_PAIR_RSP,
    STATE_PAIR_CFM,
    STATE_REJOIN,
} p24g_device_status_e;

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


typedef enum
{
    P24G_SPP_NONE                       =   0x60,
    P24G_SPP_LED_STATUS                 =   0x61,
    P24G_SPP_REPORT_RATE                =   0x62,
    P24G_SPP_BATT_CAP                   =   0x63,
    P24G_SPP_CHG_STATUS                 =   0x64,
    P24G_SPP_TEST_DATA                  =   0x65,
    P24G_SPP_ALL_KEY_DATA               =   0x66,
    P24G_SPP_CONSUME_KEY_DATA           =   0x67,
} p24g_spp_cmd_e;


typedef enum
{
    P24G_LL_CMD_NONE                    =   0x70,
    P24G_LL_CMD_TERMINATE_CONN          =   0x71,
    P24G_LL_CMD_TERMINATE_CONN_RSP      =   0x72,
    P24G_LL_CMD_8K_RR_ADAPT             =   0x73,
    P24G_LL_CMD_8K_RR_ADAPT_RSP         =   0x74,
} p24g_ll_cmd_e;


typedef enum {
    P24G_KB_MODE_USB                    =   0x00,
    P24G_KB_MODE_2P4G                   =   0x01,

    P24G_FLOW_SN                        =   0x01,
    P24G_FLOW_NESN                      =   0x02,

    P24G_LL_CONN_TIMEOUT                =   0x01,
    P24G_LL_CONN_TERMINATION_BY_PEER    =   0x02,
    P24G_LL_CONN_TERMINATION_BY_LOCAL   =   0x03,

    P24G_8K_KM_SLOT                     =   0x00,
    P24G_8K_SPP_SLOT                    =   0x01,
    P24G_8K_SLOT_POS                    =   0x03,

    DEVICE_TYPE_KB                      =   BIT(0),
    DEVICE_TYPE_MS                      =   BIT(1),

};

typedef enum
{
    REPORT_RATE_8K                      = BIT(7),
    REPORT_RATE_4K                      = BIT(0),
    REPORT_RATE_2K                      = BIT(1),
    REPORT_RATE_1K                      = BIT(2),
    REPORT_RATE_500                     = BIT(3),
    REPORT_RATE_250                     = BIT(4),
    REPORT_RATE_125                     = BIT(5),

    RR_8K_HIGH                          = 0,
    RR_8K_LOW                           = 1,
    REPORT_RATE_8K_MODE_LOW             = REPORT_RATE_500,
    REPORT_RATE_8K_MODE_HIGH            = REPORT_RATE_8K,
} report_rate_t;


