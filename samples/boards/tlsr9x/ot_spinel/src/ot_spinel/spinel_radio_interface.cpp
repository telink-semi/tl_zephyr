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

static void spinel_radio_interface_rx_done(otInstance *aInstance,
	otRadioFrame *aFrame, ot::Error aError)
{
	LOG_INF("%s", __func__);
}

static void spinel_radio_interface_tx_done(otInstance *aInstance,
	otRadioFrame *aFrame, otRadioFrame *aAckFrame, ot::Error aError)
{
	LOG_INF("%s", __func__);
}

static void spinel_radio_interface_escan_done(otInstance *aInstance,
	int8_t aMaxRssi)
{
	LOG_INF("%s", __func__);
}

static void spinel_radio_interface_tx_started(otInstance *aInstance,
	otRadioFrame *aFrame)
{
	LOG_INF("%s", __func__);
}

static void spinel_radio_interface_sw_done(otInstance *aInstance,
	bool aSuccess)
{
	LOG_INF("%s", __func__);
}

#if OPENTHREAD_CONFIG_DIAG_ENABLE
static void spinel_radio_interface_diag_rx_done(otInstance *aInstance,
	otRadioFrame *aFrame, ot::Error aError)
{
	LOG_INF("%s", __func__);
}

static void spinel_radio_interface_diag_tx_done(otInstance *aInstance
	otRadioFrame *aFrame, ot::Error aError)
{
	LOG_INF("%s", __func__);
}
#endif // OPENTHREAD_CONFIG_DIAG_ENABLE

static RadioSpinelCallbacks radio_callbacks = {
	.mReceiveDone = spinel_radio_interface_rx_done,
	.mTransmitDone = spinel_radio_interface_tx_done,
	.mEnergyScanDone = spinel_radio_interface_escan_done,
	.mTxStarted = spinel_radio_interface_tx_started,
	.mSwitchoverDone = spinel_radio_interface_sw_done,
#if OPENTHREAD_CONFIG_DIAG_ENABLE
	.mDiagReceiveDone = spinel_radio_interface_diag_rx_done,
	.mDiagTransmitDone = spinel_radio_interface_diag_tx_done
#endif
};

extern "C"
void spinel_radio_interface_init(void)
{
	if (!spinel_radio_interface) {
		spinel_radio_interface = new RadioSpinel;
		if (spinel_radio_interface) {
			spinel_radio_interface->SetCallbacks(radio_callbacks);
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
otRadioState spinel_radio_interface_get_state(void)
{
	otRadioState result = OT_RADIO_STATE_INVALID;

	if (spinel_radio_interface) {
		result = spinel_radio_interface->GetState();
	}
	return result;
}

extern "C"
uint8_t spinel_radio_interface_get_channel(void)
{
	uint8_t result = 0;

	if (spinel_radio_interface) {
		result = spinel_radio_interface->GetChannel();
	}
	return result;
}

extern "C"
otError spinel_radio_interface_get_ieee_eui64(uint8_t *ieee_eui64)
{
	otError result = OT_ERROR_FAILED;

	if (spinel_radio_interface) {
		result = spinel_radio_interface->GetIeeeEui64(ieee_eui64);
	}
	return result;
}

extern "C"
otError spinel_radio_interface_set_short_address(uint16_t short_address)
{
	otError result = OT_ERROR_FAILED;

	if (spinel_radio_interface) {
		result = spinel_radio_interface->SetShortAddress(short_address);
	}
	return result;
}

extern "C"
otError spinel_radio_interface_set_extended_address(const otExtAddress *extended_address)
{
	otError result = OT_ERROR_FAILED;

	if (spinel_radio_interface) {
		result = spinel_radio_interface->SetExtendedAddress(*extended_address);
	}
	return result;
}

extern "C"
otError spinel_radio_interface_set_pan_id(uint16_t pan_id)
{
	otError result = OT_ERROR_FAILED;

	if (spinel_radio_interface) {
		result = spinel_radio_interface->SetPanId(pan_id);
	}
	return result;
}

extern "C"
otError spinel_radio_interface_energy_scan(uint8_t channel, uint16_t duration_us)
{
	otError result = OT_ERROR_FAILED;

	if (spinel_radio_interface) {
		result = spinel_radio_interface->EnergyScan(channel, duration_us);
	}
	return result;
}

extern "C"
otError spinel_radio_interface_set_cca_energy_detect_threshold(int8_t threshold)
{
	otError result = OT_ERROR_FAILED;

	if (spinel_radio_interface) {
		result = spinel_radio_interface->SetCcaEnergyDetectThreshold(threshold);
	}
	return result;
}

extern "C"
int8_t spinel_radio_interface_get_rssi(void)
{
	int8_t  result  = OT_RADIO_RSSI_INVALID;

	if (spinel_radio_interface) {
		result = spinel_radio_interface->GetRssi();
	}
	return result;
}

extern "C"
bool spinel_radio_interface_radio_is_enabled(void)
{
	bool result = false;

	if (spinel_radio_interface) {
		result = spinel_radio_interface->IsEnabled();
	}
	return result;
}

extern "C"
otError spinel_radio_interface_enable(otInstance *instance)
{
	otError result = OT_ERROR_FAILED;

	if (spinel_radio_interface) {
		result = spinel_radio_interface->Enable(instance);
	}
	return result;
}

extern "C"
otError spinel_radio_interface_disable(void)
{
	otError result = OT_ERROR_FAILED;

	if (spinel_radio_interface) {
		result = spinel_radio_interface->Disable();
	}
	return result;
}

extern "C"
otError spinel_radio_interface_sleep(void)
{
	otError result = OT_ERROR_FAILED;

	if (spinel_radio_interface) {
		result = spinel_radio_interface->Sleep();
	}
	return result;
}

extern "C"
otError spinel_radio_interface_receive(uint8_t channel)
{
	otError result = OT_ERROR_FAILED;

	if (spinel_radio_interface) {
		result = spinel_radio_interface->Receive(channel);
	}
	return result;
}

extern "C"
otError spinel_radio_interface_set_ch_max_transmit_power(uint8_t channel, int8_t power)
{
	otError result = OT_ERROR_FAILED;

	if (spinel_radio_interface) {
		result = spinel_radio_interface->SetChannelMaxTransmitPower(channel, power);
	}
	return result;
}

extern "C"
otError spinel_radio_interface_get_transmit_power(int8_t *power)
{
	otError result = OT_ERROR_FAILED;

	if (spinel_radio_interface) {
		result = spinel_radio_interface->GetTransmitPower(*power);
	}
	return result;
}

extern "C"
otError spinel_radio_interface_set_transmit_power(int8_t power)
{
	otError result = OT_ERROR_FAILED;

	if (spinel_radio_interface) {
		result = spinel_radio_interface->SetTransmitPower(power);
	}
	return result;
}

extern "C"
otError spinel_radio_interface_transmit(otRadioFrame *frame)
{
	otError result = OT_ERROR_FAILED;

	if (spinel_radio_interface) {
		result = spinel_radio_interface->Transmit(*frame);
	}
	return result;
}
