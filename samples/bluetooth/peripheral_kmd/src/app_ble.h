/** @file
 *  @brief HoG Service sample
 */

/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
    unsigned char slave_mac_addr[4];//4
    int ble_id[4];//20
    uint8_t temp2[3]; //23
    uint8_t mast_id;//24
    int idx;//28
} ST_BLE_APP_PIPE_INFO;
extern ST_BLE_APP_PIPE_INFO ble_app_pip_info;

#define    PAIR_TIMEOUT_US              60*1000//6s
#define    RECONN_TIMEOUT_US            60*1000//60s
#define    CON_NO_ACTIVE_TIMEOUT_S      600 * 1000//10min

typedef enum
{
    BLE_STATUS_INIT=0,
    IDLE_BLE_STATUS=1,
    ADV_PAIR_BLE_STATUS=2,
    ADV_RECONNECT_BLE_STATUS=3,
    CON_BEGIN_BLE_STATUS=4,
    CON_FIRST_SMP_BLE_STATUS=5,
    CON_RECONNECT_SMP_BLE_STATUS=6,
    CON_WRITE_CCC_BLE_STATUS=7,
    CON_OK_BLE_STATUS=8,
    SLEEP_BLE_STATUS=9,
        
}BLE_STATUS_APP_ENUM;
extern volatile uint8_t ble_status;
extern uint8_t connect_complete;
extern uint32_t tick_connected;
extern uint8_t pair_flag;


void ble_init(void);
void ble_start_pairing_delayed_work_handler(struct k_work *work);
void start_pairing_by_delay_work(void);
void app_ble_report_to_client(void);
void disconnect_current_connection(void);
void save_ble_app_info(void);
#ifdef __cplusplus
}
#endif
