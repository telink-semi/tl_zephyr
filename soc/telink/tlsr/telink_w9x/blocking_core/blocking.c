/*
 * Copyright (c) 2024 - 2025 Telink Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/init.h>
#include <ipc/ipc_based_driver.h>
#include <soc.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(blocking_core_w91);

enum {
	BLOCKING_CORE_STATE_INVALID = 0,
	BLOCKING_CORE_STATE_INITED,
	BLOCKING_CORE_STATE_BLOCK_READY
};

static void __GENERIC_SECTION(.ram_code) __attribute__((noinline))
blocking_w91_wait(volatile uint32_t *addr)
{
	uint32_t cnt = 0;

	/* Bug: waits until core reaches a safe execution point in RAM before flash operations.
	 * Add 100 nop iterations
	 */
	while (cnt++ < 100) {
		__asm("nop");
	}

	*addr = BLOCKING_CORE_STATE_BLOCK_READY;
	while (*addr != BLOCKING_CORE_STATE_INITED) {
		__asm("nop");
	}
}

static void blocking_w91_request(const void *data, size_t len, void *param)
{
	volatile uint32_t *blocking_state = (volatile uint32_t *)param;
	uint32_t key = irq_lock();

	blocking_w91_wait(blocking_state);
	irq_unlock(key);
}

static int blocking_w91_init(void)
{
	static volatile uint32_t __GENERIC_SECTION(.ram_code_data) blocking_state =
		BLOCKING_CORE_STATE_INVALID;

	uint32_t out[2] = {IPC_DISPATCHER_MK_ID(IPC_DISPATCHER_BLOCKING, 0),
			   (uint32_t)&blocking_state};

	ipc_dispatcher_add(IPC_DISPATCHER_MK_ID(IPC_DISPATCHER_BLOCKING, 0), blocking_w91_request,
			   (void *)&blocking_state);
	if (ipc_dispatcher_send(out, sizeof(out)) != sizeof(out)) {
		LOG_ERR("blocking core can't share address");
		return -EIO;
	}
	while (blocking_state != BLOCKING_CORE_STATE_INITED) {
		__asm("nop");
	}
	LOG_DBG("blocking core shared %p", (void *)&blocking_state);
	return 0;
}

SYS_INIT(blocking_w91_init, POST_KERNEL, CONFIG_TELINK_W91_IPC_PRE_DRIVERS_INIT_PRIORITY);
