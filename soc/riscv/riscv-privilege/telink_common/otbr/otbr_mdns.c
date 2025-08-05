/*
 * Copyright (c) 2025 Telink Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "otbr_mdns.h"
#include "otbr_mdns_fmt.h"
#include "otbr_srp.h"
#include <zephyr/net/socket.h>

#define LOG_LEVEL CONFIG_TELINK_OTBR_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(otbr_mdns);

/************************************************************************
 * MDNS init Check configuration
 ************************************************************************/

#if CONFIG_NET_HOSTNAME_ENABLE
#error zephyr dns subsystem should be switched off
#endif /* CONFIG_NET_HOSTNAME_ENABLE */

/************************************************************************
 * MDNS internal data
 ************************************************************************/

#define OTBR_MDNS_PORT    5353
#define OTBR_MDNS_BUF_LEN 512

struct otbr_mdns_context {
	struct otbr_context *otbr_ctx;
	struct sockaddr cli_addr;
	socklen_t cli_addr_len;
	uint8_t rx_buf[OTBR_MDNS_BUF_LEN];
	uint8_t tx_buf[OTBR_MDNS_BUF_LEN];
	size_t tx_buf_pos;
	int socket;
	struct k_thread thread;
};

static bool otbr_mdns_update_top_domain(char *str, size_t str_len, const char *old,
					const char *new);

static const char otbr_mdns_host_name[] = CONFIG_TELINK_OTBR_HOST_NAME ".local.";

static struct otbr_mdns_context otbr_mdns_ctx = { .socket = -1 };
K_THREAD_STACK_DEFINE(otbr_mdns_thread_stack, CONFIG_TELINK_OTBR_MDNS_THREAD_STACK_SIZE);

/************************************************************************
 * MDNS internal functions
 ************************************************************************/

static void on_mdns_query(uint16_t qtype, uint16_t qclass, const uint8_t *qname, void *ctx)
{
	struct otbr_mdns_context *otbr_mdns_ctx = (struct otbr_mdns_context *)ctx;
	char qname_str[255];

	if (!otbr_mdns_fmt_query_get_str(qname_str, sizeof(qname_str), qname)) {
		qname_str[0] = '\0';
	}
	LOG_DBG("mdns query: %s %s %s", otbr_mdns_fmt_qtype_str(qtype),
		otbr_mdns_fmt_qclass_str(qclass), qname_str);
	if (qclass != OTBR_MDNS_FMT_QCLASS_IN) {
		return;
	}
	if (qtype == OTBR_MDNS_FMT_QTYPE_AAAA) {
		if (!otbr_mdns_fmt_strcasecmp(qname_str, otbr_mdns_host_name)) {
			if (otbr_mdns_ctx->cli_addr.sa_family == AF_INET6) {
				const struct in6_addr *addr = net_if_ipv6_select_src_addr(
					otbr_mdns_ctx->otbr_ctx->infra_if,
					(struct in6_addr *)&otbr_mdns_ctx->cli_addr);

				otbr_mdns_fmt_response_aaaa(
					otbr_mdns_ctx->tx_buf, sizeof(otbr_mdns_ctx->tx_buf),
					&otbr_mdns_ctx->tx_buf_pos, qname, addr);
			}
		} else {
			if (otbr_mdns_update_top_domain(
				    qname_str, sizeof(qname_str), "local.",
				    otSrpServerGetDomain(
					    otbr_mdns_ctx->otbr_ctx->ot_ctx->instance))) {
				const otSrpServerHost *host;

				openthread_api_mutex_lock(otbr_mdns_ctx->otbr_ctx->ot_ctx);
				if (otbr_srp_search_host(otbr_mdns_ctx->otbr_ctx->ot_ctx, qname_str,
							 &host)) {
					const otIp6Address *srp_addr = otbr_srp_get_host_addrss(
						otbr_mdns_ctx->otbr_ctx->ot_ctx, host);

					if (srp_addr) {
						struct in6_addr addr;

						memcpy(&addr, srp_addr, sizeof(addr));
						otbr_mdns_fmt_response_aaaa(
							otbr_mdns_ctx->tx_buf,
							sizeof(otbr_mdns_ctx->tx_buf),
							&otbr_mdns_ctx->tx_buf_pos, qname, &addr);
					}
				}
				openthread_api_mutex_unlock(otbr_mdns_ctx->otbr_ctx->ot_ctx);
			}
		}
	} else if (qtype == OTBR_MDNS_FMT_QTYPE_ANY) {
		if (otbr_mdns_update_top_domain(
			    qname_str, sizeof(qname_str), "local.",
			    otSrpServerGetDomain(otbr_mdns_ctx->otbr_ctx->ot_ctx->instance))) {
			const otSrpServerService *service;

			openthread_api_mutex_lock(otbr_mdns_ctx->otbr_ctx->ot_ctx);
			if (otbr_srp_search_service(otbr_mdns_ctx->otbr_ctx->ot_ctx, qname_str,
						    &service)) {
				otbr_mdns_fmt_response_srv(
					otbr_mdns_ctx->tx_buf, sizeof(otbr_mdns_ctx->tx_buf),
					&otbr_mdns_ctx->tx_buf_pos, qname,
					otSrpServerHostGetFullName(
						otSrpServerServiceGetHost(service)),
					otSrpServerServiceGetPort(service),
					otSrpServerGetDomain(
						otbr_mdns_ctx->otbr_ctx->ot_ctx->instance),
					"local.");
			}
			openthread_api_mutex_unlock(otbr_mdns_ctx->otbr_ctx->ot_ctx);
		}
	}
}

static void otbr_mdns_handler(void *arg1, void *arg2, void *arg3)
{
	struct otbr_mdns_context *otbr_mdns_ctx = (struct otbr_mdns_context *)arg1;

	otbr_mdns_ctx->cli_addr_len = sizeof(otbr_mdns_ctx->cli_addr);
	ssize_t result = zsock_recvfrom(otbr_mdns_ctx->socket, otbr_mdns_ctx->rx_buf,
					sizeof(otbr_mdns_ctx->rx_buf), ZSOCK_MSG_DONTWAIT,
					&otbr_mdns_ctx->cli_addr, &otbr_mdns_ctx->cli_addr_len);

	if (result > 0) {
		otbr_mdns_ctx->tx_buf_pos = 0;
		otbr_mdns_fmt_query_get(otbr_mdns_ctx->rx_buf, result, on_mdns_query,
					otbr_mdns_ctx);
		if (otbr_mdns_ctx->tx_buf_pos) {
			uint16_t id = otbr_mdns_fmt_query_get_id(otbr_mdns_ctx->rx_buf, result);
			struct sockaddr_in6 addr = {
				.sin6_family = AF_INET6,
				.sin6_addr = IN6ADDR_DNS_MULTICAST_INIT,
				.sin6_port = htons(OTBR_MDNS_PORT),
			};

			otbr_mdns_fmt_response_set_id(otbr_mdns_ctx->tx_buf,
						      otbr_mdns_ctx->tx_buf_pos, id);
			if (zsock_sendto(otbr_mdns_ctx->socket, otbr_mdns_ctx->tx_buf,
					 otbr_mdns_ctx->tx_buf_pos, 0, (struct sockaddr *)&addr,
					 sizeof(addr)) < 0) {
				LOG_ERR("mdns response fail");
			}
		}
	}
}

static bool otbr_mdns_update_top_domain(char *str, size_t str_len, const char *old, const char *new)
{
	bool updated = false;
	size_t str_l = strlen(str);
	size_t old_l = strlen(old);
	size_t new_l = strlen(new);

	if ((str_l == old_l || (str_l >= old_l && str[str_l - old_l - 1] == '.')) &&
	    str_l - old_l + new_l < str_len) {
		if (!otbr_mdns_fmt_strcasecmp(str + str_l - old_l, old)) {
			strcpy(str + str_l - old_l, new);
			updated = true;
		}
	}
	return updated;
}

/************************************************************************
 * MDNS external functions
 ************************************************************************/

void otbr_mdns_start(struct otbr_context *ctx)
{
	otbr_mdns_stop();
	bool started = false;

	do {
		otbr_mdns_ctx.socket = zsock_socket(AF_INET6, SOCK_DGRAM, IPPROTO_UDP);
		if (otbr_mdns_ctx.socket < 0) {
			break;
		}
		struct sockaddr_in6 addr = {.sin6_family = AF_INET6,
					    .sin6_addr = IN6ADDR_ANY_INIT,
					    .sin6_port = htons(OTBR_MDNS_PORT)};

		if (zsock_bind(otbr_mdns_ctx.socket, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
			break;
		}
		otbr_mdns_ctx.otbr_ctx = ctx;
		(void)k_thread_create(&otbr_mdns_ctx.thread,
			otbr_mdns_thread_stack, K_THREAD_STACK_SIZEOF(otbr_mdns_thread_stack),
			otbr_mdns_handler, &otbr_mdns_ctx, NULL, NULL,
			CONFIG_TELINK_OTBR_MDNS_THREAD_PRIORITY, 0, K_NO_WAIT);
		(void)k_thread_name_set(&otbr_mdns_ctx.thread, "OTBR-MDNS");
		k_thread_start(&otbr_mdns_ctx.thread);
		started = true;
		LOG_INF("mdns started");
	} while (0);
	if (!started) {
		LOG_ERR("mdns failed");
		otbr_mdns_stop();
	}
}

void otbr_mdns_stop(void)
{
	k_thread_abort(&otbr_mdns_ctx.thread);
	if (otbr_mdns_ctx.socket >= 0) {
		(void)zsock_close(otbr_mdns_ctx.socket);
		otbr_mdns_ctx.socket = -1;
	}
}
