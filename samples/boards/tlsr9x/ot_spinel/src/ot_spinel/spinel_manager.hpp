/*
 * Copyright (c) 2025 Telink Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef OT_SPINEL_MANAGER_HPP_
#define OT_SPINEL_MANAGER_HPP_

#include <zephyr/kernel.h>

#include <lib/spinel/openthread-spinel-config.h>
#include <lib/spinel/spinel_interface.hpp>
#include <lib/spinel/spinel_driver.hpp>

namespace ot {
namespace Spinel {

class SpinelManager
{
public:
	static SpinelManager *GetInstance(void);
	static void DestroyInstance(void);

	SpinelInterface &GetSpinelInterface(void) {
		return *m_spinel_interface;
	}

	SpinelDriver &GetSpinelDriver(void) {
		return m_spinel_driver;
	}

	void BindInterfaceToDriver(void);

private:
	static const struct device *const k_uart_dev;
	static SpinelManager *m_instance;

	SpinelInterface      *m_spinel_interface;
	SpinelDriver          m_spinel_driver;

	SpinelManager();
	~SpinelManager();
	SpinelManager(const SpinelManager &) = delete;
	SpinelManager &operator=(const SpinelManager &) = delete;
};

} // namespace Spinel
} // namespace ot

#endif // OT_HDLC_INTERFACE_HPP_
