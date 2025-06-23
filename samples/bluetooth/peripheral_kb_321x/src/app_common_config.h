/********************************************************************************************************
 * @file    app_common_config.h
 *
 * @brief   This is the header file for BLE SDK
 *
 * @author  BLE GROUP
 * @date    06,2022
 *
 * @par     Copyright (c) 2022, Telink Semiconductor (Shanghai) Co., Ltd. ("TELINK")
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

// #include "config.h"
// //#include "types.h"
// #include "bit.h"



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

#define MAC_ADDR_LEN                6 //MAC address length



typedef enum {
    KB_MODE_2P4G                        =   0,
    KB_MODE_BLE                         =   1,
    KB_MODE_USB                         =   2,
} kb_mode_t;

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
