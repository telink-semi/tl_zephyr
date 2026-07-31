/*
 * Copyright (c) 2024 Telink Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <stdarg.h>

int sniprintf(char *str, size_t size, const char *format, ...)
{
	va_list ap;
	int ret;

	va_start(ap, format);
	ret = vsnprintf(str, size, format, ap);
	va_end(ap);

	return ret;
}