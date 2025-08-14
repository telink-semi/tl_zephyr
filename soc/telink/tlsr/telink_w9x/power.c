/*
 * Copyright (c) 2025 Telink Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/init.h>
#include <zephyr/pm/pm.h>
#include <zephyr/logging/log.h>
#include <ipc/ipc_based_driver.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(pm_w91, CONFIG_SOC_LOG_LEVEL);

#define DT_DRV_COMPAT riscv_machine_timer

#define MTIME_REG    DT_INST_REG_ADDR(0)
#define MTIMECMP_REG (DT_INST_REG_ADDR(0) + 8)

enum {
	IPC_DISPATCHER_PM_ENABLE = IPC_DISPATCHER_PM,
	IPC_DISPATCHER_PM_SET_WAKEUP_TIME,
};

static struct ipc_based_driver ipc_data; /* ipc driver data part */

/* APIs implementation: pm enable */
static size_t pack_pm_w91_enable(uint8_t inst, void *unpack_data, uint8_t *pack_data)
{
	uint8_t *p_pm_enable = unpack_data;

	size_t pack_data_len = sizeof(uint32_t) + sizeof(*p_pm_enable);

	if (pack_data != NULL) {
		uint32_t id = IPC_DISPATCHER_MK_ID(IPC_DISPATCHER_PM_ENABLE, inst);

		IPC_DISPATCHER_PACK_FIELD(pack_data, id);
		IPC_DISPATCHER_PACK_FIELD(pack_data, *p_pm_enable);
	}

	return pack_data_len;
}

IPC_DISPATCHER_UNPACK_FUNC_ONLY_WITH_ERROR_PARAM(pm_w91_enable);

static int pm_w91_enable(bool enable)
{
	int err;

	IPC_DISPATCHER_HOST_SEND_DATA(&ipc_data, 0, pm_w91_enable, &enable, &err,
				      CONFIG_TELINK_W91_IPC_DISPATCHER_TIMEOUT_MS);

	return err;
}

/* APIs implementation: pm set wakeup time */
static int pm_w91_set_wakeup_time(uint64_t *p_time)
{
	int err;
	uint32_t id = IPC_DISPATCHER_MK_ID(IPC_DISPATCHER_PM_SET_WAKEUP_TIME, 0);
	size_t pack_data_len = sizeof(id) + sizeof(*p_time);
	uint8_t pack_data[pack_data_len];

	memcpy(pack_data, &id, sizeof(id));
	memcpy(pack_data + sizeof(id), p_time, sizeof(*p_time));

	err = ipc_dispatcher_send(pack_data, pack_data_len);
	if (err < 0) {
		LOG_ERR("Failed to set wake up time (err = %d)", err);
	}

	return err;
}

static int pm_w91_init(void)
{
	ipc_based_driver_init(&ipc_data);
	pm_w91_enable(true);

	return 0;
}

/* Get Machine Timer Compare value */
static uint64_t get_mtime_compare(void)
{
	return *(const volatile uint64_t *const)((uint32_t)(MTIMECMP_REG +
							    (_current_cpu->id * sizeof(uint64_t))));
}

/* Get Machine Timer value */
static uint64_t get_mtime(void)
{
	const volatile uint32_t *const rl = (const volatile uint32_t *const)MTIME_REG;
	const volatile uint32_t *const rh =
		(const volatile uint32_t *const)(MTIME_REG + sizeof(uint32_t));
	uint32_t mtime_l, mtime_h;

	do {
		mtime_h = *rh;
		mtime_l = *rl;
	} while (mtime_h != *rh);
	return (((uint64_t)mtime_h) << 32) | mtime_l;
}

/* PM state set API implementation */
void pm_state_set(enum pm_state state, uint8_t substate_id)
{
	ARG_UNUSED(substate_id);

	switch (state) {
	case PM_STATE_SUSPEND_TO_IDLE:
	case PM_STATE_STANDBY: {
		uint64_t current_time = get_mtime();
		uint64_t wakeup_time = get_mtime_compare();

		if (current_time >= wakeup_time) {
			LOG_WRN("Incorrect wake up time: ctime (%llu) >= (%llu) wtime\n",
				current_time, wakeup_time);
			return;
		}

		(void)pm_w91_set_wakeup_time(&wakeup_time);
		break;
	}
	default:
		LOG_DBG("Unsupported power state %u", state);
		break;
	}

	k_cpu_idle();
}

/* Handle SOC specific activity after Low Power Mode Exit */
void pm_state_exit_post_ops(enum pm_state state, uint8_t substate_id)
{
	ARG_UNUSED(state);
	ARG_UNUSED(substate_id);
}

SYS_INIT(pm_w91_init, POST_KERNEL, CONFIG_TELINK_W91_IPC_PRE_DRIVERS_INIT_PRIORITY);
