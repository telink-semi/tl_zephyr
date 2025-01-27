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
otError spinel_radio_interface_get_ieee_eui64(uint8_t *ieee_eui64);
otError spinel_radio_interface_set_short_address(uint16_t short_address);
otError spinel_radio_interface_set_extended_address(const otExtAddress *extended_address);
otError spinel_radio_interface_set_pan_id(uint16_t pan_id);

} /* extern "C" */

#endif /* OT_SPINEL_RADIO_INTERFACE_ */
