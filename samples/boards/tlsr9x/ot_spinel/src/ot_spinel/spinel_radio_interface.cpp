/*
 * Copyright (c) 2025 Telink Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "spinel_radio_interface.h"
#include "spinel_manager.hpp"
#include <lib/spinel/radio_spinel.hpp>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(SpinelRadio, LOG_LEVEL_INF); // TODO: use level from config?

using namespace ot::Spinel;

static RadioSpinel *spinel_radio_interface = nullptr;

extern "C"
void spinel_radio_interface_init(void)
{
	if (!spinel_radio_interface) {
		spinel_radio_interface = new RadioSpinel;
		if (spinel_radio_interface) {
			spinel_radio_interface->Init(false, true,
				&SpinelManager::GetInstance()->GetSpinelDriver());
		}
	}
}

extern "C"
void spinel_radio_interface_deinit(void)
{
	if (spinel_radio_interface) {
		spinel_radio_interface->Deinit();
		SpinelManager::DestroyInstance();
		delete spinel_radio_interface;
		spinel_radio_interface = nullptr;
	}
}

extern "C"
uint32_t spinel_radio_interface_get_bus_speed(void)
{
	uint32_t result = 0;

	if (spinel_radio_interface) {
		result = spinel_radio_interface->GetBusSpeed();
	}
	return result;
}

extern "C"
const char *spinel_radio_interface_get_version(void)
{
	const char * result = nullptr;

	if (spinel_radio_interface) {
		result = spinel_radio_interface->GetVersion();
	}
	return result;
}

extern "C"
otRadioCaps spinel_radio_interface_get_radio_caps(void)
{
	otRadioCaps result = OT_RADIO_CAPS_NONE;

	if (spinel_radio_interface) {
		result = spinel_radio_interface->GetRadioCaps();
	}
	return result;
}

extern "C"
otError spinel_radio_interface_set_short_address(uint16_t aAddress)
{
	otError result = OT_ERROR_FAILED;

	if (spinel_radio_interface) {
		result = spinel_radio_interface->SetShortAddress(aAddress);
	}
	return result;
}

extern "C"
otError spinel_radio_interface_set_extended_address(const otExtAddress *aExtAddress)
{
	otError result = OT_ERROR_FAILED;

	if (spinel_radio_interface) {
		result = spinel_radio_interface->SetExtendedAddress(*aExtAddress);
	}
	return result;
}

extern "C"
otError spinel_radio_interface_set_pan_id(uint16_t aPanId)
{
	otError result = OT_ERROR_FAILED;

	if (spinel_radio_interface) {
		result = spinel_radio_interface->SetPanId(aPanId);
	}
	return result;
}
