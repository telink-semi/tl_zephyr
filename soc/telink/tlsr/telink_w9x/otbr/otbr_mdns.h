/*
 * Copyright (c) 2025 Telink Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef OTBR_MDNS_H
#define OTBR_MDNS_H

#include "otbr_context.h"

void otbr_mdns_start(struct otbr_context *ctx);
void otbr_mdns_stop(void);

#endif /* OTBR_MDNS_H */
