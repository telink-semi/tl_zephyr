/*
 * Copyright (c) 2025 Telink Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef OTBR_MDNS_FMT_H
#define OTBR_MDNS_FMT_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <zephyr/net/net_ip.h>

#define OTBR_MDNS_FMT_QTYPE_A     1
#define OTBR_MDNS_FMT_QTYPE_CNAME 5
#define OTBR_MDNS_FMT_QTYPE_PTR   12
#define OTBR_MDNS_FMT_QTYPE_TXT   16
#define OTBR_MDNS_FMT_QTYPE_AAAA  28
#define OTBR_MDNS_FMT_QTYPE_SRV   33
#define OTBR_MDNS_FMT_QTYPE_ANY   255

#define OTBR_MDNS_FMT_QCLASS_IN   1
#define OTBR_MDNS_FMT_QCLASS_CH   3
#define OTBR_MDNS_FMT_QCLASS_HS   4
#define OTBR_MDNS_FMT_QCLASS_NONE 254
#define OTBR_MDNS_FMT_QCLASS_ANY  255

typedef void (*otbr_mdns_fmt_query)(uint16_t qtype, uint16_t qclass, const uint8_t *qname,
				    void *ctx);

void otbr_mdns_fmt_query_get(const uint8_t *data, size_t data_len, otbr_mdns_fmt_query cb,
			     void *ctx);
uint16_t otbr_mdns_fmt_query_get_id(const uint8_t *data, size_t data_len);
char *otbr_mdns_fmt_query_get_str(char *str, size_t str_len, const uint8_t *qmane);
const char *otbr_mdns_fmt_qtype_str(uint16_t qtype);
const char *otbr_mdns_fmt_qclass_str(uint16_t qclass);
void otbr_mdns_fmt_response_aaaa(uint8_t *data, size_t data_len, size_t *data_pos,
				 const uint8_t *qmame, const struct in6_addr *ip);
void otbr_mdns_fmt_response_srv(uint8_t *data, size_t data_len, size_t *data_pos,
				const uint8_t *qmame, const char *host_name, uint16_t port,
				const char *domain_old, const char *domain_new);
void otbr_mdns_fmt_response_set_id(uint8_t *data, size_t data_len, uint16_t id);
int otbr_mdns_fmt_strcasecmp(const char *str1, const char *str2);

#endif /* OTBR_MDNS_FMT_H */
