/** @file
 *  @brief app_battery.h
 */

/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifdef __cplusplus
extern "C" {
#endif

extern uint32_t lowBattDet_tick;

void app_battery_check_init(void);
void app_battery_check(uint16_t alarm_vol_mv);
#ifdef __cplusplus
}
#endif
