/*
 * Copyright (c) 2024 Telink Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/net/wifi_mgmt.h>

#include <zephyr/posix/unistd.h>
#include <zephyr/posix/netdb.h>
#include <zephyr/posix/sys/time.h>
#include <zephyr/posix/sys/socket.h>
#include <zephyr/posix/arpa/inet.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(sock_app, LOG_LEVEL_INF);

#if CONFIG_NET_IPV4
#define ECHO_SERVER_IP           CONFIG_NET_CONFIG_PEER_IPV4_ADDR
#define ECHO_SERVER_UDP_BUF_SIZE 1472
#define ECHO_SERVER_TCP_BUF_SIZE 2048
#endif /* CONFIG_NET_IPV4 */
#if CONFIG_NET_IPV6
#define ECHO_SERVER_IPV6              CONFIG_NET_CONFIG_PEER_IPV6_ADDR
#define ECHO_SERVER_UDP_IPV6_BUF_SIZE 1452
#define ECHO_SERVER_TCP_IPV6_BUF_SIZE 2048
#endif /* CONFIG_NET_IPV6 */
#define ECHO_SERVER_PORT          2024
#define ECHO_SERVER_TIMEOUT_MS    1000
#define ECHO_SERVER_DEADTIME_MS   1000
#define WIFI_RECONNECT_TIMEOUT_MS 100
#define APP_RECONNECT_TIMEOUT_MS  1000

#if CONFIG_APP_SOCKET_UDP
static void udp_data_exchange(volatile bool *ip_v4, volatile bool *ip_v6)
{
	LOG_INF("app started");

	int serv_sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

	do {
		if (serv_sock < 0) {
			LOG_ERR("socket failed: (%d) %s", errno, strerror(errno));
			break;
		}

		struct sockaddr_in me_addr = {.sin_family = AF_INET,
					      .sin_port = htons(ECHO_SERVER_PORT),
					      .sin_addr.s_addr = htonl(INADDR_ANY)};

		if (bind(serv_sock, (const struct sockaddr *)&me_addr, sizeof(me_addr)) < 0) {
			LOG_ERR("bind: (%d) %s", errno, strerror(errno));
		}

		struct timeval tv = {.tv_sec = ECHO_SERVER_TIMEOUT_MS / 1000,
				     .tv_usec = (ECHO_SERVER_TIMEOUT_MS % 1000) * 1000};

		if (setsockopt(serv_sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
			LOG_ERR("setsockopt failed: (%d) %s", errno, strerror(errno));
			break;
		}

		uint8_t cnt = 0;

		for (bool error = false; !error && *ip_v4 && *ip_v6; cnt++) {

			struct sockaddr_in server_addr = {
				.sin_family = AF_INET,
				.sin_port = htons(ECHO_SERVER_PORT),
			};

			if (inet_pton(AF_INET, ECHO_SERVER_IP, &server_addr.sin_addr) < 0) {
				LOG_ERR("inet_pton failed");
				error = true;
				continue;
			}

			/* flush socket */
			uint8_t trash;

			while (recv(serv_sock, &trash, sizeof(trash), MSG_DONTWAIT) > 0) {
			}

			static uint8_t tx_buf[ECHO_SERVER_UDP_BUF_SIZE];
			size_t tx_len = 0;

			for (size_t i = 0; i < sizeof(tx_buf); i++) {
				tx_buf[i] = cnt + i;
			}

			while (tx_len < sizeof(tx_buf)) {
				ssize_t l = sendto(
					serv_sock, &tx_buf[tx_len], sizeof(tx_buf) - tx_len, 0,
					(struct sockaddr *)&server_addr, sizeof(server_addr));
				if (l < 0) {
					LOG_ERR("sendto failed: (%d) %s", errno, strerror(errno));
					error = true;
					break;
				}
				tx_len += l;
			}

			if (error) {
				continue;
			}

			LOG_INF("sent message to: %s:%u", ECHO_SERVER_IP,
				ntohs(server_addr.sin_port));

			static uint8_t rx_buf[sizeof(tx_buf)];
			ssize_t rx_len = 0;

			while (rx_len < sizeof(rx_buf)) {
				struct sockaddr_in client_addr;
				socklen_t client_addr_len = sizeof(client_addr);

				ssize_t l = recvfrom(
					serv_sock, &rx_buf[rx_len], sizeof(rx_buf) - rx_len, 0,
					(struct sockaddr *)&client_addr, &client_addr_len);
				if (l < 0) {
					LOG_ERR("recvfrom failed: (%d) %s", errno, strerror(errno));
					break;
				}

				if (server_addr.sin_addr.s_addr == client_addr.sin_addr.s_addr) {
					rx_len += l;
				}
			}

			if (error) {
				continue;
			}

			if (rx_len == tx_len) {
				if (!memcmp(rx_buf, tx_buf, tx_len)) {
					LOG_INF("all OK");
				} else {
					LOG_ERR("transmit and receive mismatch");
				}
			} else {
				LOG_ERR("transmit and receive lengths mismatch (%u - %u)",
					(unsigned int)tx_len, (unsigned int)rx_len);
			}
			k_msleep(ECHO_SERVER_DEADTIME_MS);
		}

	} while (0);

	if (serv_sock >= -0) {
		if (close(serv_sock) < 0) {
			LOG_ERR("close failed: (%d) %s", errno, strerror(errno));
		}
	}

	LOG_INF("app finished");
}

static void (*data_exchange)(volatile bool *ip_v4, volatile bool *ip_v6) = udp_data_exchange;

#endif /* CONFIG_APP_SOCKET_UDP */

#if CONFIG_APP_SOCKET_UDP_IPV6
static void udp_ipv6_data_exchange(volatile bool *ip_v4, volatile bool *ip_v6)
{
	LOG_INF("app started");

	int serv_sock = socket(AF_INET6, SOCK_DGRAM, IPPROTO_UDP);

	do {
		if (serv_sock < 0) {
			LOG_ERR("socket failed: (%d) %s", errno, strerror(errno));
			break;
		}

		struct sockaddr_in6 me_addr = {.sin6_family = AF_INET6,
					       .sin6_port = htons(ECHO_SERVER_PORT),
					       .sin6_addr = in6addr_any};

		if (bind(serv_sock, (const struct sockaddr *)&me_addr, sizeof(me_addr)) < 0) {
			LOG_ERR("bind: (%d) %s", errno, strerror(errno));
		}

		struct timeval tv = {.tv_sec = ECHO_SERVER_TIMEOUT_MS / 1000,
				     .tv_usec = (ECHO_SERVER_TIMEOUT_MS % 1000) * 1000};

		if (setsockopt(serv_sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
			LOG_ERR("setsockopt failed: (%d) %s", errno, strerror(errno));
			break;
		}

		uint8_t cnt = 0;

		for (bool error = false; !error && *ip_v4 && *ip_v6; cnt++) {

			struct sockaddr_in6 server_addr = {
				.sin6_family = AF_INET6,
				.sin6_port = htons(ECHO_SERVER_PORT),
			};

			if (inet_pton(AF_INET6, ECHO_SERVER_IPV6, &server_addr.sin6_addr) < 0) {
				LOG_ERR("inet_pton failed");
				error = true;
				continue;
			}

			if (net_ipv6_is_ll_addr(&server_addr.sin6_addr)) {
				int scope_id = net_if_get_by_iface(net_if_get_default());

				if (scope_id > 0) {
					server_addr.sin6_scope_id = scope_id;
				} else {
					LOG_WRN("no default interface index");
				}
			}

			/* flush socket */
			uint8_t trash;

			while (recv(serv_sock, &trash, sizeof(trash), MSG_DONTWAIT) > 0) {
			}

			static uint8_t tx_buf[ECHO_SERVER_UDP_IPV6_BUF_SIZE];
			size_t tx_len = 0;

			for (size_t i = 0; i < sizeof(tx_buf); i++) {
				tx_buf[i] = cnt + i;
			}

			while (tx_len < sizeof(tx_buf)) {
				ssize_t l = sendto(
					serv_sock, &tx_buf[tx_len], sizeof(tx_buf) - tx_len, 0,
					(struct sockaddr *)&server_addr, sizeof(server_addr));
				if (l < 0) {
					LOG_ERR("sendto failed: (%d) %s", errno, strerror(errno));
					error = true;
					break;
				}
				tx_len += l;
			}

			if (error) {
				continue;
			}

			LOG_INF("sent message to: %s:%u", ECHO_SERVER_IPV6,
				ntohs(server_addr.sin6_port));

			static uint8_t rx_buf[sizeof(tx_buf)];
			ssize_t rx_len = 0;

			while (rx_len < sizeof(rx_buf)) {
				struct sockaddr_in6 client_addr;
				socklen_t client_addr_len = sizeof(client_addr);

				ssize_t l = recvfrom(
					serv_sock, &rx_buf[rx_len], sizeof(rx_buf) - rx_len, 0,
					(struct sockaddr *)&client_addr, &client_addr_len);
				if (l < 0) {
					LOG_ERR("recvfrom failed: (%d) %s", errno, strerror(errno));
					break;
				}

				if (!memcmp(&server_addr.sin6_addr, &client_addr.sin6_addr,
					    sizeof(struct in6_addr))) {
					rx_len += l;
				}
			}

			if (error) {
				continue;
			}

			if (rx_len == tx_len) {
				if (!memcmp(rx_buf, tx_buf, tx_len)) {
					LOG_INF("all OK");
				} else {
					LOG_ERR("transmit and receive mismatch");
				}
			} else {
				LOG_ERR("transmit and receive lengths mismatch (%u - %u)",
					(unsigned int)tx_len, (unsigned int)rx_len);
			}
			k_msleep(ECHO_SERVER_DEADTIME_MS);
		}

	} while (0);

	if (serv_sock >= -0) {
		if (close(serv_sock) < 0) {
			LOG_ERR("close failed: (%d) %s", errno, strerror(errno));
		}
	}

	LOG_INF("app finished");
}

static void (*data_exchange)(volatile bool *ip_v4, volatile bool *ip_v6) = udp_ipv6_data_exchange;

#endif /* CONFIG_APP_SOCKET_UDP_IPV6 */

#if CONFIG_APP_SOCKET_TCP
static void tcp_data_exchange(volatile bool *ip_v4, volatile bool *ip_v6)
{
	LOG_INF("app started");

	struct sockaddr_in server_addr = {.sin_family = AF_INET,
					  .sin_port = htons(ECHO_SERVER_PORT)};

	if (inet_pton(AF_INET, ECHO_SERVER_IP, &server_addr.sin_addr) < 0) {
		LOG_ERR("inet_pton failed");
		return;
	}

	uint8_t cnt = 0;

	for (bool error = false; !error && *ip_v4 && *ip_v6; cnt++) {
		int serv_sock = -1;

		do {
			serv_sock = socket(AF_INET, SOCK_STREAM, 0);
			if (serv_sock < 0) {
				LOG_ERR("socket failed: (%d) %s", errno, strerror(errno));
				error = true;
				break;
			}

			if (connect(serv_sock, (struct sockaddr *)&server_addr,
				    sizeof(server_addr)) < 0) {
				LOG_ERR("connect failed: (%d) %s", errno, strerror(errno));
				error = true;
				break;
			}

			static uint8_t tx_buf[ECHO_SERVER_TCP_BUF_SIZE];
			size_t tx_len = 0;

			for (size_t i = 0; i < sizeof(tx_buf); i++) {
				tx_buf[i] = cnt + i;
			}

			while (tx_len < sizeof(tx_buf)) {
				ssize_t l =
					write(serv_sock, &tx_buf[tx_len], sizeof(tx_buf) - tx_len);

				if (l < 0) {
					LOG_ERR("write: (%d) %s", errno, strerror(errno));
					break;
				}
				tx_len += l;
			}

			if (tx_len != sizeof(tx_buf)) {
				error = true;
				break;
			}

			static uint8_t rx_buf[sizeof(tx_buf)];
			size_t rx_len = 0;

			while (rx_len < sizeof(rx_buf)) {
				ssize_t l =
					read(serv_sock, &rx_buf[rx_len], sizeof(rx_buf) - rx_len);

				if (l < 0) {
					LOG_ERR("read: (%d) %s", errno, strerror(errno));
					break;
				}
				rx_len += l;
			}

			if (rx_len == tx_len) {
				if (!memcmp(rx_buf, tx_buf, tx_len)) {
					LOG_INF("all OK");
				} else {
					LOG_ERR("transmit and receive mismatch");
				}
			} else {
				LOG_ERR("transmit and receive lengths mismatch (%u - %u)",
					(unsigned int)tx_len, (unsigned int)rx_len);
			}

			k_msleep(ECHO_SERVER_DEADTIME_MS);
		} while (0);

		if (serv_sock >= -0) {
			if (close(serv_sock) < 0) {
				error = true;
				LOG_ERR("close failed: (%d) %s", errno, strerror(errno));
			}
		}
	}

	LOG_INF("app finished");
}

static void (*data_exchange)(volatile bool *ip_v4, volatile bool *ip_v6) = tcp_data_exchange;

#endif /* CONFIG_APP_SOCKET_TCP */

#if CONFIG_APP_SOCKET_TCP_IPV6
static void tcp_ipv6_data_exchange(volatile bool *ip_v4, volatile bool *ip_v6)
{
	LOG_INF("app started");

	struct sockaddr_in6 server_addr = {.sin6_family = AF_INET6,
					   .sin6_port = htons(ECHO_SERVER_PORT)};

	if (inet_pton(AF_INET6, ECHO_SERVER_IPV6, &server_addr.sin6_addr) < 0) {
		LOG_ERR("inet_pton failed");
		return;
	}

	if (net_ipv6_is_ll_addr(&server_addr.sin6_addr)) {
		int scope_id = net_if_get_by_iface(net_if_get_default());

		if (scope_id > 0) {
			server_addr.sin6_scope_id = scope_id;
		} else {
			LOG_WRN("no default interface index");
		}
	}

	uint8_t cnt = 0;

	for (bool error = false; !error && *ip_v4 && *ip_v6; cnt++) {
		int serv_sock = -1;

		do {
			serv_sock = socket(AF_INET6, SOCK_STREAM, 0);
			if (serv_sock < 0) {
				LOG_ERR("socket failed: (%d) %s", errno, strerror(errno));
				error = true;
				break;
			}

			if (connect(serv_sock, (struct sockaddr *)&server_addr,
				    sizeof(server_addr)) < 0) {
				LOG_ERR("connect failed: (%d) %s", errno, strerror(errno));
				error = true;
				break;
			}

			static uint8_t tx_buf[ECHO_SERVER_TCP_IPV6_BUF_SIZE];
			size_t tx_len = 0;

			for (size_t i = 0; i < sizeof(tx_buf); i++) {
				tx_buf[i] = cnt + i;
			}

			while (tx_len < sizeof(tx_buf)) {
				ssize_t l =
					write(serv_sock, &tx_buf[tx_len], sizeof(tx_buf) - tx_len);

				if (l < 0) {
					LOG_ERR("write: (%d) %s", errno, strerror(errno));
					break;
				}
				tx_len += l;
			}

			if (tx_len != sizeof(tx_buf)) {
				error = true;
				break;
			}

			static uint8_t rx_buf[sizeof(tx_buf)];
			size_t rx_len = 0;

			while (rx_len < sizeof(rx_buf)) {
				ssize_t l =
					read(serv_sock, &rx_buf[rx_len], sizeof(rx_buf) - rx_len);

				if (l < 0) {
					LOG_ERR("read: (%d) %s", errno, strerror(errno));
					break;
				}
				rx_len += l;
			}

			if (rx_len == tx_len) {
				if (!memcmp(rx_buf, tx_buf, tx_len)) {
					LOG_INF("all OK");
				} else {
					LOG_ERR("transmit and receive mismatch");
				}
			} else {
				LOG_ERR("transmit and receive lengths mismatch (%u - %u)",
					(unsigned int)tx_len, (unsigned int)rx_len);
			}

			k_msleep(ECHO_SERVER_DEADTIME_MS);
		} while (0);

		if (serv_sock >= -0) {
			if (close(serv_sock) < 0) {
				error = true;
				LOG_ERR("close failed: (%d) %s", errno, strerror(errno));
			}
		}
	}

	LOG_INF("app finished");
}

static void (*data_exchange)(volatile bool *ip_v4, volatile bool *ip_v6) = tcp_ipv6_data_exchange;

#endif /* CONFIG_APP_SOCKET_TCP_IPV6 */

static struct {
	volatile bool ip_v4;
	volatile bool ip_v6;
	struct k_sem sem;
} app_data;

static void on_network_event(struct net_mgmt_event_callback *cb, uint32_t mgmt_event,
			     struct net_if *iface)
{
	switch (mgmt_event) {
	case NET_EVENT_WIFI_CONNECT_RESULT: {
		const struct wifi_status *status = (const struct wifi_status *)cb->info;

		LOG_INF("connect event: %d", status->status);

		if (!status->status) {
#if !CONFIG_NET_DHCPV4
			app_data.ip_v4 = true;
#endif /* !CONFIG_NET_DHCPV4 */
#if !CONFIG_NET_DHCPV6
			app_data.ip_v6 = true;
#endif /* !CONFIG_NET_DHCPV6  */
		}
	} break;
	case NET_EVENT_WIFI_DISCONNECT_RESULT:
		LOG_INF("disconnect event");
		app_data.ip_v4 = false;
		app_data.ip_v6 = false;
#if CONFIG_NET_DHCPV4
		for (size_t i = 0; i < NET_IF_MAX_IPV4_ADDR; i++) {
			if (iface->config.ip.ipv4->unicast[i].ipv4.addr_type != NET_ADDR_DHCP) {
				continue;
			}
			(void)net_if_ipv4_addr_rm(
				iface, &iface->config.ip.ipv4->unicast[i].ipv4.address.in_addr);
		}
#endif /* CONFIG_NET_DHCPV4 */
#if CONFIG_NET_DHCPV6
		for (size_t i = 0; i < NET_IF_MAX_IPV6_ADDR; i++) {
			if (iface->config.ip.ipv6->unicast[i].addr_type != NET_ADDR_DHCP) {
				continue;
			}
			(void)net_if_ipv6_addr_rm(
				iface, &iface->config.ip.ipv6->unicast[i].address.in6_addr);
		}
#endif /* CONFIG_NET_DHCPV6 */
		break;
#if CONFIG_NET_DHCPV4
	case NET_EVENT_IPV4_ADDR_ADD:
		LOG_INF("new ip v4");
		for (size_t i = 0; i < NET_IF_MAX_IPV4_ADDR; i++) {
			LOG_INF("%s",
				inet_ntop(AF_INET,
					  &iface->config.ip.ipv4->unicast[i].ipv4.address.in_addr,
					  (char[INET_ADDRSTRLEN]) {}, INET_ADDRSTRLEN));
			if (iface->config.ip.ipv4->unicast[i].ipv4.addr_type != NET_ADDR_DHCP) {
				continue;
			}
			LOG_INF("obtained dhcp v4");
			app_data.ip_v4 = true;
		}
		break;
#endif /* CONFIG_NET_DHCPV4 */
#if CONFIG_NET_DHCPV6
	case NET_EVENT_IPV6_ADDR_ADD:
		LOG_INF("new ip v6");
		for (size_t i = 0; i < NET_IF_MAX_IPV6_ADDR; i++) {
			LOG_INF("%s", inet_ntop(AF_INET6,
						&iface->config.ip.ipv6->unicast[i].address.in6_addr,
						(char[INET6_ADDRSTRLEN]) {}, INET6_ADDRSTRLEN));
			if (iface->config.ip.ipv6->unicast[i].addr_type != NET_ADDR_DHCP) {
				continue;
			}
			LOG_INF("obtained dhcp v6");
			app_data.ip_v6 = true;
		}
		break;
#endif /* CONFIG_NET_DHCPV6 */
	default:
		break;
	}
	if (app_data.ip_v4 && app_data.ip_v6) {
		k_sem_give(&app_data.sem);
	}
}

int main(void)
{
	app_data.ip_v4 = false;
	app_data.ip_v6 = false;
	(void)k_sem_init(&app_data.sem, 0, 1);

	struct net_mgmt_event_callback wifi_cb;

	net_mgmt_init_event_callback(&wifi_cb, on_network_event,
				     NET_EVENT_WIFI_CONNECT_RESULT |
					     NET_EVENT_WIFI_DISCONNECT_RESULT);
	net_mgmt_add_event_callback(&wifi_cb);

#if CONFIG_NET_DHCPV4 && CONFIG_NET_DHCPV6
	struct net_mgmt_event_callback dhcp_v4_cb;
	struct net_mgmt_event_callback dhcp_v6_cb;

	net_mgmt_init_event_callback(&dhcp_v4_cb, on_network_event, NET_EVENT_IPV4_ADDR_ADD);
	net_mgmt_init_event_callback(&dhcp_v6_cb, on_network_event, NET_EVENT_IPV6_ADDR_ADD);
	net_mgmt_add_event_callback(&dhcp_v4_cb);
	net_mgmt_add_event_callback(&dhcp_v6_cb);
#elif CONFIG_NET_DHCPV4
	struct net_mgmt_event_callback dhcp_v4_cb;

	net_mgmt_init_event_callback(&dhcp_v4_cb, on_network_event, NET_EVENT_IPV4_ADDR_ADD);
	net_mgmt_add_event_callback(&dhcp_v4_cb);
#elif CONFIG_NET_DHCPV6
	struct net_mgmt_event_callback dhcp_v6_cb;

	net_mgmt_init_event_callback(&dhcp_v6_cb, on_network_event, NET_EVENT_IPV6_ADDR_ADD);
	net_mgmt_add_event_callback(&dhcp_v6_cb);
#endif

	for (;;) {
		if (!app_data.ip_v4 || !app_data.ip_v6) {
			struct wifi_connect_req_params connect_req_params = {
				.ssid = CONFIG_SAMPLE_WIFI_SSID,
				.ssid_length = strlen(CONFIG_SAMPLE_WIFI_SSID),
				.psk = CONFIG_SAMPLE_WIFI_PASSWORD,
				.psk_length = strlen(CONFIG_SAMPLE_WIFI_PASSWORD),
				.security = WIFI_SECURITY_TYPE_PSK};

			k_sem_reset(&app_data.sem);
			if (net_mgmt(NET_REQUEST_WIFI_CONNECT, net_if_get_default(),
				     &connect_req_params, sizeof(connect_req_params))) {
				LOG_ERR("connection request failed\n");
				k_msleep(WIFI_RECONNECT_TIMEOUT_MS);
			} else {
				LOG_INF("connecting...");
				(void)k_sem_take(&app_data.sem, K_FOREVER);
			}
		}
		if (app_data.ip_v4 && app_data.ip_v6) {
			LOG_INF("connected");
			data_exchange(&app_data.ip_v4, &app_data.ip_v6);
			LOG_INF("disconnected");
		}
		k_msleep(APP_RECONNECT_TIMEOUT_MS);
	}

	return 0;
}
