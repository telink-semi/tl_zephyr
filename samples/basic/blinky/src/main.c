/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <string.h>

#if !CONFIG_MBEDTLS
#include <hash/hash.h>
#include <hash/hash_portable.h>
#else
#include "mbedtls/sha256.h"
#endif

int main(void)
{
	static uint8_t array[64 * 1024];

	printk("array: %p %u\n", array, sizeof(array));

	const uint8_t inp[] = {
		0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
		0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f
	};

	uint8_t ref[] = {
		0xbe, 0x45, 0xcb, 0x26, 0x05, 0xbf, 0x36, 0xbe,
		0xbd, 0xe6, 0x84, 0x84, 0x1a, 0x28, 0xf0, 0xfd,
		0x43, 0xc6, 0x98, 0x50, 0xa3, 0xdc, 0xe5, 0xfe,
		0xdb, 0xa6, 0x99, 0x28, 0xee, 0x3a, 0x89, 0x91
	};

	uint8_t out[32];


#if !CONFIG_MBEDTLS
	HASH_CTX ctx;

	printk("hash_dig_en\n");
	hash_dig_en();


	if (hash_init(&ctx, HASH_SHA256)) {
		printk("hash_init failed\n");
	}

	if (hash_update(&ctx, inp, sizeof(inp) / 2)) {
		printk("hash_update failed\n");
	}

	printk("sleep+\n");
	k_msleep(1000);
	printk("sleep-\n");

	hash_dig_en();

	if (hash_update(&ctx, &inp[sizeof(inp) / 2], sizeof(inp) - sizeof(inp) / 2)) {
		printk("hash_update failed\n");
	}

	if (hash_final(&ctx, out)) {
		printk("hash_final failed\n");
	}

#else
	mbedtls_sha256_context ctx;

	mbedtls_sha256_init(&ctx);

	if (mbedtls_sha256_starts(&ctx, false)) {
		printk("mbedtls_sha256_starts failed\n");
	}

	if (mbedtls_sha256_update(&ctx, inp, sizeof(inp) / 2)) {
		printk("mbedtls_sha256_update failed\n");
	}

	printk("sleep+\n");
	k_msleep(1000);
	printk("sleep-\n");

	if (mbedtls_sha256_update(&ctx, &inp[sizeof(inp) / 2], sizeof(inp) - sizeof(inp) / 2)) {
		printk("mbedtls_sha256_update failed\n");
	}

	if (mbedtls_sha256_finish(&ctx, out)) {
		printk("mbedtls_sha256_finish failed\n");
	}

	mbedtls_sha256_free(&ctx);
#endif

	if (!memcmp(ref, out, 32)) {
		printk("hash ok\n");
	} else {
		printk("hash digest failed\n");
	}

	for(;;) {}
	return 0;
}
