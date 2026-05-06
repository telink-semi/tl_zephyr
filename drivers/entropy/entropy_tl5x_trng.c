/*
 * Copyright (c) 2025 Telink Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT telink_tl5x_trng

#include <zephyr/drivers/entropy.h>

static int entropy_tl5x_trng_init(const struct device *dev)
{
	ARG_UNUSED(dev);

	return 0;
}

static int entropy_tl5x_trng_get_entropy(const struct device *dev, uint8_t *buffer, uint16_t length)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(buffer);
	ARG_UNUSED(length);

	return -ENOTSUP;
}

static int entropy_tl5x_trng_get_entropy_isr(const struct device *dev, uint8_t *buffer,
					     uint16_t length, uint32_t flags)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(buffer);
	ARG_UNUSED(length);
	ARG_UNUSED(flags);

	return -ENOTSUP;
}

static const struct entropy_driver_api entropy_tl5x_trng_api = {
	.get_entropy = entropy_tl5x_trng_get_entropy,
	.get_entropy_isr = entropy_tl5x_trng_get_entropy_isr};

DEVICE_DT_INST_DEFINE(0, entropy_tl5x_trng_init, NULL, NULL, NULL, PRE_KERNEL_1,
		      CONFIG_ENTROPY_INIT_PRIORITY, &entropy_tl5x_trng_api);
