/*
 * Copyright (c) 2025 Telink Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef OTBR_EXT_H
#define OTBR_EXT_H

#include "otbr_context.h"

void otbr_ext_infra_up(struct otbr_context *ctx);
void otbr_ext_infra_down(struct otbr_context *ctx);
void otbr_ext_thread_start(struct openthread_context *aContext);
bool otbr_ext_omr_ipaddr_show(struct openthread_context *aContext);
void otbr_ext_thread_dataset_show(struct openthread_context *aContext);
void otbr_ext_apply_omr_prefix(struct openthread_context *aContext);

#endif /* OTBR_EXT_H */
