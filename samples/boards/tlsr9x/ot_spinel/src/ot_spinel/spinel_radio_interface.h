/*
 * Copyright (c) 2025 Telink Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef OT_SPINEL_RADIO_INTERFACE_
#define OT_SPINEL_RADIO_INTERFACE_

#include <openthread/platform/radio.h>

extern "C" {

void spinel_radio_interface_init(void);
void spinel_radio_interface_deinit(void);
uint32_t spinel_radio_interface_get_bus_speed(void);
const char *spinel_radio_interface_get_version(void);
otRadioCaps spinel_radio_interface_get_radio_caps(void);
otRadioState spinel_radio_interface_get_state(void);
uint8_t spinel_radio_interface_get_channel(void);
otError spinel_radio_interface_get_ieee_eui64(uint8_t *ieee_eui64);
otError spinel_radio_interface_set_short_address(uint16_t short_address);
otError spinel_radio_interface_set_extended_address(const otExtAddress *extended_address);
otError spinel_radio_interface_set_pan_id(uint16_t pan_id);
uint32_t spinel_radio_interface_get_radio_channel_mask(bool preffered);
otError spinel_radio_interface_energy_scan(uint8_t channel, uint16_t duration_us);
otError spinel_radio_interface_set_cca_energy_detect_threshold(int8_t threshold);
int8_t spinel_radio_interface_get_rssi(void);
bool spinel_radio_interface_radio_is_enabled(void);
otError spinel_radio_interface_enable(otInstance *instance);
otError spinel_radio_interface_disable(void);
otError spinel_radio_interface_sleep(void);
otError spinel_radio_interface_receive(uint8_t channel);
otError spinel_radio_interface_set_ch_max_transmit_power(uint8_t channel, int8_t power);
otError spinel_radio_interface_get_transmit_power(int8_t *power);
otError spinel_radio_interface_set_transmit_power(int8_t power);
otError spinel_radio_interface_transmit(otRadioFrame *frame);
bool spinel_radio_interface_is_transmitting(void);
bool spinel_radio_interface_is_transmit_done(void);

} /* extern "C" */

#endif /* OT_SPINEL_RADIO_INTERFACE_ */
