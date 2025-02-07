/*
 * Copyright (c) 2025 Telink Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "spinel_manager.hpp"
#include "hdlc_interface.hpp"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(SpinMan, LOG_LEVEL_INF); // TODO: use level from config?

namespace ot {
namespace Spinel {

const struct device *const SpinelManager::k_uart_dev =
	DEVICE_DT_GET_OR_NULL(DT_CHOSEN(zephyr_ot_uart));

SpinelManager* SpinelManager::m_instance = nullptr;

SpinelManager::SpinelManager() : m_spinel_interface(nullptr), m_spinel_driver()
{
	m_spinel_interface = new HdlcInterface(k_uart_dev);
	if (!m_spinel_interface) {
		LOG_ERR("Can't create interface");
	}
}

SpinelManager::~SpinelManager()
{
	if (m_spinel_interface) {
		m_spinel_interface->Deinit();
		delete m_spinel_interface;
		LOG_INF("Spinel interface delated");
	}
	m_spinel_driver.Deinit();
}

SpinelManager *SpinelManager::GetInstance(void)
{
	if(!m_instance) {
		m_instance = new SpinelManager;
	}
	return m_instance;
}

void SpinelManager::DestroyInstance(void)
{
	if(m_instance) {
		delete m_instance;
		m_instance = nullptr;
	}
}

void SpinelManager::BindInterfaceToDriver(void)
{
	if (m_spinel_interface) {
		LOG_INF("Spinel interface created");
		spinel_iid_t spinel_iid = 0;

		CoprocessorType type = m_spinel_driver.Init(*m_spinel_interface,
			true, &spinel_iid, sizeof(spinel_iid));
		static const char *coprocessor_type_str[] = {"unknown", "rcp", "ncp"};

		LOG_INF("Coprocessor type: (%u) %s", type, coprocessor_type_str[
			type < ARRAY_SIZE(coprocessor_type_str) ? type : 0]);
	} else {
		LOG_ERR("No spinel interface");
	}
}

} // namespace Spinel
} // namespace ot
