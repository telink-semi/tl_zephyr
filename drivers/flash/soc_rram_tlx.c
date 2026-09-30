/*
 * Copyright (c) 2024 Telink Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * RRAM (NVM) driver for Telink TLx series.
 *
 * RRAM vs Flash key differences:
 *   - No erase needed (direct overwrite)
 *   - Minimum read/write granularity: 16 bytes (NVM controller burst size)
 *   - Empty state (after power-up): all zeros (0x00)
 *   - Access through NVM controller at 0x80101000 via AHB/REG mode
 *   - Memory mapped at AHB0: 0x00400000
 */

#define DT_DRV_COMPAT telink_tlx_rram
#define RRAM_NODE   DT_NODELABEL(rram)
#define RRAM_SIZE   DT_REG_SIZE(RRAM_NODE)
#define RRAM_ORIGIN DT_REG_ADDR(RRAM_NODE)

#include <zephyr/logging/log.h>
#include <string.h>
#include <errno.h>
#include <zephyr/device.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/kernel.h>
#include <stdlib.h>
#include <nvm.h>
#include "tl322x_rram_volt.h"

LOG_MODULE_REGISTER(rram_tlx, CONFIG_FLASH_LOG_LEVEL);

/*
 * Sector size: 4096 bytes, matching Flash sector size.
 * This allows NVS and other Zephyr subsystems to work unchanged.
 * RRAM doesn't physically have sectors, but this provides a logical
 * erasure granularity for API compatibility.
 */
#define SECTOR_SIZE             (0x1000u)

/*
 * RRAM NVM controller operates in bursts of 4 words (16 bytes).
 * Read via AHB0 can be byte-granular. Write via REG mode requires
 * 16-byte alignment.
 */
#define RRAM_WRITE_MIN_SIZE     (16u)

/* RRAM physical page size: 256 bytes per page, used as erase batch unit */
#define RRAM_ERASE_CHUNK_SIZE   (256u)

/* Maximum wait time for mutex acquisition */
#define RRAM_ACCESS_TIMEOUT_MS  (30u)

/* Driver instance data */
struct rram_tlx_data {
	struct k_mutex lock;
};

/*
 * Flash parameters exposed to upper layers (NVS, etc.):
 *   write_block_size = 16  (NVM controller minimum write burst)
 *   erase_value      = 0x00 (RRAM power-on state is all zeros)
 */
static const struct flash_parameters rram_tlx_parameters = {
	.write_block_size = DT_PROP(RRAM_NODE, write_block_size),
	.erase_value = 0x00,
};

/*
 * RRAM MTP lock helpers.
 *
 * Unlike Flash protection where unlock persists until next boot,
 * RRAM MTP lock must be re-locked after each write operation:
 *   1. nvm_read_reg(NVM_READ_MP_CMD) check current lock state
 *   2. nvm_mtp_lock_set(NVM_MTP_LOCK_NONE)   unlock
 *   3. nvm_reg_write(...)                     write
 *   4. nvm_mtp_lock_set(NVM_MTP_LOCK_ALL_512K) re-lock
 */

static inline bool rram_tlx_is_locked(void)
{
	return (nvm_read_reg(NVM_READ_MP_CMD) == NVM_MTP_LOCK_ALL_512K);
}

static inline void rram_tlx_unlock(void)
{
	if (rram_tlx_is_locked()) {
		nvm_mtp_lock_set(NVM_MTP_LOCK_NONE);
	}
}

static inline void rram_tlx_lock(void)
{
	nvm_mtp_lock_set(NVM_MTP_LOCK_ALL_512K);
	nvm_write_disable();
}

static inline void rram_tlx_lock_init(void)
{
	if (!rram_tlx_is_locked()) {
		nvm_mtp_lock_set(NVM_MTP_LOCK_ALL_512K);
		nvm_write_disable();
	}
}

/* Check for valid offset and length */
static bool rram_tlx_is_range_valid(off_t offset, size_t len)
{
	if ((offset < 0) || (len < 1) || ((offset + len) > RRAM_SIZE)) {
		return false;
	}
	return true;
}

/*
 * Core write implementation.
 *
 * RRAM writes via the NVM controller REG mode require 16-byte alignment.
 * For unaligned accesses, a read-modify-write is performed:
 *   1. Align offset down to 16-byte boundary
 *   2. Read existing 16-byte block via AHB0
 *   3. Merge new data into the buffer
 *   4. Write the 16-byte block via REG mode
 *
 * Unlike Flash, NO ERASE is performed - RRAM supports direct overwrite.
 *
 * nvm_reg_write() addr and len are in units of 16 bytes.
 */
static int rram_tlx_write_impl(const struct device *dev, off_t offset,
			       const void *data, size_t len, bool is_erase)
{
	int rc = 0;
	uint32_t write_buf[RRAM_WRITE_MIN_SIZE / sizeof(uint32_t)];
	uint32_t aligned_off;
	uint32_t block_off;
	size_t remaining;
	const uint8_t *src;

	if (len == 0) {
		return 0;
	}

	/*
	 * RRAM writes require the core voltage at 1.1V. Boost/restore
	 * once around the whole burst loop below; do not switch voltage
	 * per iteration.
	 */
	bool volt_boosted = tl322x_rram_volt_boost();

	aligned_off = offset & ~(RRAM_WRITE_MIN_SIZE - 1);
	block_off  = offset - aligned_off;
	remaining  = len;
	src        = (const uint8_t *)data;

	while (remaining > 0) {
		size_t copy_len;

		/*
		 * Bulk path: when 16-byte aligned, batch writes in 256-byte
		 * chunks matching the RRAM physical page size.  This applies
		 * to both erase (write zeros) and normal write (write data).
		 * For normal writes, the RMW read is skipped because NVS
		 * always provides 16-byte aligned offset and length, so the
		 * entire block is being overwritten anyway.
		 */
		if (block_off == 0 && remaining >= RRAM_WRITE_MIN_SIZE) {
			uint32_t chunk_buf[RRAM_ERASE_CHUNK_SIZE
					   / sizeof(uint32_t)];

			copy_len = MIN(remaining, RRAM_ERASE_CHUNK_SIZE);

			if (is_erase) {
				memset(chunk_buf, 0, copy_len);
			} else {
				memcpy(chunk_buf, src, copy_len);
				src += copy_len;
			}
			nvm_reg_write(aligned_off, copy_len, chunk_buf);

			aligned_off += copy_len;
			remaining -= copy_len;
			continue;
		}

		/*
		 * RMW path: read-modify-write a single 16-byte block.
		 * Only needed when the write is unaligned at head or tail
		 * (block_off != 0) or fewer than 16 bytes remain, which
		 * only occurs for non-NVS callers or erase tail fragments.
		 */
		nvm_ahb0_read_byte(aligned_off, RRAM_WRITE_MIN_SIZE,
				   (unsigned char *)write_buf);

		copy_len = RRAM_WRITE_MIN_SIZE - block_off;
		if (copy_len > remaining) {
			copy_len = remaining;
		}

		if (is_erase) {
			memset((uint8_t *)write_buf + block_off, 0, copy_len);
		} else {
			memcpy((uint8_t *)write_buf + block_off, src, copy_len);
			src += copy_len;
		}

		nvm_reg_write(aligned_off, RRAM_WRITE_MIN_SIZE,
			      (unsigned int *)write_buf);

		aligned_off += RRAM_WRITE_MIN_SIZE;
		block_off = 0;
		remaining -= copy_len;
	}

	tl322x_rram_volt_restore(volt_boosted);

	return rc;
}

/* API implementation: driver initialization */
static int rram_tlx_init(const struct device *dev)
{
	struct rram_tlx_data *dev_data = dev->data;

	k_mutex_init(&dev_data->lock);

	/*
	 * Initialize MTP lock on RRAM to protect the full 512K area.
	 */
	rram_tlx_lock_init();

	return 0;
}

/*
 * Erase: Write erase_value (0x00) to the target region.
 *
 * RRAM does not require physical erasure, but Zephyr's flash API
 * requires this callback. It writes zeros via the same REG mode
 * path as regular writes.
 *
 * NVS calls this with sector-aligned offset and sector_size length.
 */
static int rram_tlx_erase(const struct device *dev, off_t offset, size_t len)
{
	struct rram_tlx_data *dev_data = dev->data;
	int rc;

	if (!rram_tlx_is_range_valid(offset, len)) {
		return -EINVAL;
	}

	if (k_mutex_lock(&dev_data->lock,
			 K_MSEC(RRAM_ACCESS_TIMEOUT_MS))) {
		return -EACCES;
	}

	rram_tlx_unlock();
	/*
	 * is_erase = true: write_impl uses memset(0) for full blocks
	 * instead of calling nvm_ahb0_read_byte first.
	 */
	rc = rram_tlx_write_impl(dev, offset, NULL, len, true);
	rram_tlx_lock();

	k_mutex_unlock(&dev_data->lock);

	return rc;
}

/* API implementation: write */
static int rram_tlx_write(const struct device *dev, off_t offset,
			  const void *data, size_t len)
{
	struct rram_tlx_data *dev_data = dev->data;
	int rc;

	if (!rram_tlx_is_range_valid(offset, len)) {
		return -EINVAL;
	}

	if (k_mutex_lock(&dev_data->lock,
			 K_MSEC(RRAM_ACCESS_TIMEOUT_MS))) {
		return -EACCES;
	}

	/*
	 * RRAM write protection pattern (per write):
	 *   unlock → write → lock
	 */
	rram_tlx_unlock();
	rc = rram_tlx_write_impl(dev, offset, data, len, false);
	rram_tlx_lock();

	k_mutex_unlock(&dev_data->lock);

	return rc;
}

/*
 * Read: Read from RRAM via AHB0 byte interface.
 *
 * nvm_ahb0_read_byte() reads individual bytes from RRAM address space
 * at 0x00400000. No alignment restrictions.
 *
 * No lock/unlock needed for reads - MTP lock only restricts writes.
 */
static int rram_tlx_read(const struct device *dev, off_t offset,
			 void *data, size_t len)
{
	struct rram_tlx_data *dev_data = dev->data;

	/* Return SUCCESS if len == 0 (required by tests/drivers/flash) */
	if (!len) {
		return 0;
	}

	if (!rram_tlx_is_range_valid(offset, len)) {
		return -EINVAL;
	}

	if (k_mutex_lock(&dev_data->lock,
			 K_MSEC(RRAM_ACCESS_TIMEOUT_MS))) {
		return -EACCES;
	}

	/*
	 * AHBO byte read: data[i] = reg_nvm_ahb0_rd_data_byte(addr + i)
	 * The address is relative to NVM_AHB0_BASE_ADDR (0x00400000).
	 */
	nvm_ahb0_read_byte((unsigned int)offset, (unsigned int)len,
			   (unsigned char *)data);

	k_mutex_unlock(&dev_data->lock);

	return 0;
}

/* API implementation: get flash parameters */
static const struct flash_parameters *
rram_tlx_get_parameters(const struct device *dev)
{
	ARG_UNUSED(dev);

	return &rram_tlx_parameters;
}

/*
 * Page layout: RRAM is logically divided into sectors of SECTOR_SIZE (4096)
 * for compatibility with Zephyr's flash page layout API and NVS.
 */
#if defined(CONFIG_FLASH_PAGE_LAYOUT)
static const struct flash_pages_layout dev_layout = {
	.pages_count = RRAM_SIZE / SECTOR_SIZE,
	.pages_size = SECTOR_SIZE,
};

static void rram_tlx_pages_layout(const struct device *dev,
				  const struct flash_pages_layout **layout,
				  size_t *layout_size)
{
	*layout = &dev_layout;
	*layout_size = 1;
}
#endif /* CONFIG_FLASH_PAGE_LAYOUT */

/* Zephyr flash driver API vtable */
static const struct flash_driver_api rram_tlx_api = {
	.erase          = rram_tlx_erase,
	.write          = rram_tlx_write,
	.read           = rram_tlx_read,
	.get_parameters = rram_tlx_get_parameters,
#if defined(CONFIG_FLASH_PAGE_LAYOUT)
	.page_layout    = rram_tlx_pages_layout,
#endif
};

/*
 * Driver registration macro.
 * Binds to device tree nodes with compatible = "telink,tlx-rram".
 * The child node "soc-nv-flash" provides reg and write-block-size.
 */
#define RRAM_TLX_INIT(n)						\
	static struct rram_tlx_data rram_data_##n;			\
	DEVICE_DT_INST_DEFINE(n, rram_tlx_init,			\
			      NULL,					\
			      &rram_data_##n,				\
			      NULL,					\
			      POST_KERNEL,				\
			      CONFIG_FLASH_INIT_PRIORITY,		\
			      &rram_tlx_api);

DT_INST_FOREACH_STATUS_OKAY(RRAM_TLX_INIT)