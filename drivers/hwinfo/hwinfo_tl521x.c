/*
 * Copyright (c) 2026 Telink Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/hwinfo.h>
#include <ext_driver/ext_pm.h>
#include <string.h>
#include <flash.h>
#include <flash/flash_common.h>

extern void pm_update_status_info(unsigned char clr_en);

static bool is_mcu_status_updated;

ssize_t z_impl_hwinfo_get_device_id(uint8_t *buffer, size_t length)
{
	uint32_t flash_mid = 0;
	uint8_t uid[16];

	flash_mid = flash_read_mid();
	flash_read_mid_uid_with_check(&flash_mid, uid);

	if (length > sizeof(uid)) {
		length = sizeof(uid);
	}
	memcpy(buffer, uid, length);

	return length;
}

int z_impl_hwinfo_get_reset_cause(uint32_t *cause)
{
	if (!is_mcu_status_updated) {
		pm_update_status_info(1);
		is_mcu_status_updated = true;
	}

	uint32_t flags = 0;
	uint32_t reason = pm_get_mcu_status();

	if (reason & MCU_STATUS_POWER_ON) {
		flags |= RESET_PIN;
	} else {
		flags |= RESET_SOFTWARE;
	}

	*cause = flags;

	return 0;
}

int z_impl_hwinfo_clear_reset_cause(void)
{
	return 0;
}

int z_impl_hwinfo_get_supported_reset_cause(uint32_t *supported)
{
	*supported = (RESET_PIN | RESET_SOFTWARE);

	return 0;
}
