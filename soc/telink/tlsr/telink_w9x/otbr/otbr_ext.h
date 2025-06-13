/*
 * Copyright (c) 2025 Telink Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef OTBR_EXT_H
#define OTBR_EXT_H

#include <zephyr/net/openthread.h>

void otbr_ext_infra_up(struct openthread_context *aContext, uint32_t aInfraIfIndex);
void otbr_ext_infra_down(struct openthread_context *aContext, uint32_t aInfraIfIndex);
void otbr_ext_thread_start(struct openthread_context *aContext);
bool otbr_ext_omr_ipaddr_show(struct openthread_context *aContext);
void otbr_ext_thread_dataset_show(struct openthread_context *aContext);
void otbr_ext_apply_omr_route(struct openthread_context *aContext, uint32_t aInfraIfIndex);

#endif /* OTBR_EXT_H */
