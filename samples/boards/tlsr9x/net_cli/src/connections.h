/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CONNECTIONS_H
#define CONNECTIONS_H

#include <zephyr/net/net_if.h>
#include <openthread/thread.h>

typedef void (*connections_wifi_changed)(struct net_if *iface, bool is_connected);
typedef void (*connections_thread_changed)(otInstance *instance, otDeviceRole role);

void connections_init(connections_wifi_changed on_wifi, connections_thread_changed on_thread);

#endif /* CONNECTIONS_H */
