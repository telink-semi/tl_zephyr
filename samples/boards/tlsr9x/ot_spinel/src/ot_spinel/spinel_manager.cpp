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
	} else {
		LOG_INF("Spinel interface created");
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

} // namespace Spinel
} // namespace ot
