/*
 * Copyright (c) 2025 Telink Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef OTBR_SRP_H
#define OTBR_SRP_H

#include "otbr_context.h"
#include <openthread/srp_server.h>

const otIp6Address *otbr_srp_get_host_addrss(struct openthread_context *aContext,
					     const otSrpServerHost *aHost);
bool otbr_srp_search_service(struct openthread_context *aContext, const char *service_name,
			     const otSrpServerService **aService);
bool otbr_srp_search_host(struct openthread_context *aContext, const char *host_name,
			  const otSrpServerHost **aHost);
void otbr_srp_init(struct openthread_context *aContext);

#endif /* OTBR_SRP_H */
