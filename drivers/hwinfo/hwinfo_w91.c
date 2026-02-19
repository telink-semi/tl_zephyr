/*
 * Copyright (c) 2021-2023 Telink Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/drivers/hwinfo.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/storage/flash_map.h>
#include <zephyr/kernel.h>
#include <ipc/ipc_based_driver.h>
#include <string.h>
#include <stdint.h>

#define SPI_FLASH_HWINFO_ID_LEN ((size_t)6)

enum {
	IPC_DISPATCHER_SYS_RESET_CAUSE_GET_VALUE = IPC_DISPATCHER_SYS,
};

enum {
	POR = 1,
	HW_RESET,
	BOR,
	WDT_DIGITAL_TOP_RESET,
	WDT_MCU_SUBSYS_RESET,
	SW_RESET,
	HIBERNATION_RESET,
	DEEPSLEEP_RESET,
};

struct reset_cause_w91_get_value_resp {
	int err;
	uint32_t value;
};

extern uint32_t flash_w91_get_id(const struct device *dev, uint8_t *flash_id);

static struct ipc_based_driver ipc_data; /* ipc driver data part */

/* API implementation: get reset cause */
IPC_DISPATCHER_PACK_FUNC_WITHOUT_PARAM(reset_cause_w91_get_value,
				       IPC_DISPATCHER_SYS_RESET_CAUSE_GET_VALUE);

static void unpack_reset_cause_w91_get_value(void *unpack_data, const uint8_t *pack_data,
					     size_t pack_data_len)
{
	struct reset_cause_w91_get_value_resp *p_reset_cause_get_value_resp = unpack_data;
	size_t expected_len = sizeof(uint32_t) + sizeof(p_reset_cause_get_value_resp->err) +
			      sizeof(p_reset_cause_get_value_resp->value);

	if (expected_len != pack_data_len) {
		p_reset_cause_get_value_resp->err = -EINVAL;
		return;
	}

	pack_data += sizeof(uint32_t);
	IPC_DISPATCHER_UNPACK_FIELD(pack_data, p_reset_cause_get_value_resp->err);
	IPC_DISPATCHER_UNPACK_FIELD(pack_data, p_reset_cause_get_value_resp->value);
}

static int reset_cause_w91_get_value(uint32_t *value)
{
	struct reset_cause_w91_get_value_resp reset_cause_get_value_resp;

	IPC_DISPATCHER_HOST_SEND_DATA(&ipc_data, 0, reset_cause_w91_get_value, NULL,
				      &reset_cause_get_value_resp,
				      CONFIG_TELINK_IPC_DISPATCHER_TIMEOUT_MS);

	if (!reset_cause_get_value_resp.err) {
		*value = reset_cause_get_value_resp.value;
	}

	return reset_cause_get_value_resp.err;
}

ssize_t z_impl_hwinfo_get_device_id(uint8_t *buffer, size_t length)
{
	ssize_t result = length;
	uint8_t chip_id_val[SPI_FLASH_HWINFO_ID_LEN] = {0};
	const struct device *flash_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_flash_controller));

	if (length < SPI_FLASH_HWINFO_ID_LEN) {
		printk("Not enougth buffer size to get the hwinfo (ID).\n\r");
		result = 0;
	} else {
		result = (size_t)flash_w91_get_id(flash_dev, chip_id_val);
	}

	/* Check get device ID operation and store it into the buffer */
	if (result == (size_t)0) {
		memcpy(buffer, chip_id_val, SPI_FLASH_HWINFO_ID_LEN);
		result = SPI_FLASH_HWINFO_ID_LEN;
	} else {
		printk("Flash hw INFO get ID read failed!\n");
		result = 0;
	}

	return result;
}

int z_impl_hwinfo_get_reset_cause(uint32_t *cause)
{
	int ret;
	uint32_t flags = 0;
	uint32_t reset_cause = 0;

	ret = reset_cause_w91_get_value(&reset_cause);
	if (ret) {
		return ret;
	}

	switch (reset_cause) {
	case POR:
		flags |= RESET_POR;
		break;
	case HW_RESET:
		flags |= RESET_PIN;
		break;
	case BOR:
		flags |= RESET_BROWNOUT;
		break;
	case WDT_DIGITAL_TOP_RESET:
		flags |= RESET_WATCHDOG;
		break;
	case WDT_MCU_SUBSYS_RESET:
		flags |= RESET_WATCHDOG;
		break;
	case SW_RESET:
		flags |= RESET_SOFTWARE;
		break;
	case HIBERNATION_RESET:
		flags |= RESET_LOW_POWER_WAKE;
		break;
	case DEEPSLEEP_RESET:
		flags |= RESET_LOW_POWER_WAKE;
		break;
	default:
		break;
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
	*supported = (RESET_PIN | RESET_SOFTWARE | RESET_BROWNOUT | RESET_POR | RESET_WATCHDOG |
		      RESET_LOW_POWER_WAKE);

	return 0;
}

static int reset_cause_w91_init(void)
{
	ipc_based_driver_init(&ipc_data);

	return 0;
}

SYS_INIT(reset_cause_w91_init, POST_KERNEL, CONFIG_TELINK_IPC_PRE_DRIVERS_INIT_PRIORITY);
