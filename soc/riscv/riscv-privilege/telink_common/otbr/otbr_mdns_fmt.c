/*
 * Copyright (c) 2025 Telink Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "otbr_mdns_fmt.h"
#include <string.h>
#include <ctype.h>
#include <zephyr/sys/byteorder.h>

#define OTBR_MDNS_FMT_FLAGS_MASK_QR     0x8000
#define OTBR_MDNS_FMT_FLAGS_MASK_OPCODE 0x7800
#define OTBR_MDNS_FMT_FLAGS_MASK_AA     0x0400
#define OTBR_MDNS_FMT_FLAGS_MASK_TC     0x0200
#define OTBR_MDNS_FMT_FLAGS_MASK_RD     0x0100
#define OTBR_MDNS_FMT_FLAGS_MASK_RA     0x0080
#define OTBR_MDNS_FMT_FLAGS_MASK_ZERO   0x0070
#define OTBR_MDNS_FMT_FLAGS_MASK_RCODE  0x0004

#define OTBR_MDNS_FMT_QCLASS_FLUSH_BIT 0x8000

#define OTBR_MDNS_FMT_TTL 120

struct otbr_mdns_fmt_header {
	uint16_t id;
	uint16_t flags;
	uint16_t qdcount;
	uint16_t ancount;
	uint16_t nscount;
	uint16_t arcount;
} __packed;

void otbr_mdns_fmt_query_get(const uint8_t *data, size_t data_len, otbr_mdns_fmt_query cb,
			     void *ctx)
{
	if (!cb) {
		return;
	}
	if (data_len < sizeof(struct otbr_mdns_fmt_header)) {
		return;
	}
	struct otbr_mdns_fmt_header *hdr = (struct otbr_mdns_fmt_header *)data;

	if ((sys_be16_to_cpu(hdr->flags) & OTBR_MDNS_FMT_FLAGS_MASK_QR)) {
		return;
	}
	const uint8_t *begin = data, *end = data;

	data += sizeof(struct otbr_mdns_fmt_header);
	data_len -= sizeof(struct otbr_mdns_fmt_header);

	for (const uint8_t *qname = NULL; data_len;) {
		if (!qname) {
			qname = data;
		}
		if (!*data) {
			if (data_len > 4) {
				uint16_t qtype = sys_be16_to_cpu(*((uint16_t *)(data + 1)));
				uint16_t qclass = sys_be16_to_cpu(*((uint16_t *)(data + 3)));

				cb(qtype, qclass, qname, ctx);
				end = qname;
				qname = NULL;
				data_len -= 5;
				data += 5;
			} else {
				break;
			}
		} else if (*data < 64) {
			if (data_len > *data) {
				data_len -= (size_t)1 + *data;
				data += (size_t)1 + *data;
			} else {
				break;
			}
		} else {
			if (data_len > 5) {
				uint16_t qtype = sys_be16_to_cpu(*((uint16_t *)(data + 2)));
				uint16_t qclass = sys_be16_to_cpu(*((uint16_t *)(data + 4)));

				qname = begin + (sys_be16_to_cpu(*((uint16_t *)data)) & 0x3fff);
				if (qname >= begin + sizeof(struct otbr_mdns_fmt_header) &&
				    qname <= end) {
					cb(qtype, qclass, qname, ctx);
				}
				qname = NULL;
				data_len -= 6;
				data += 6;
			} else {
				break;
			}
		}
	}
}

uint16_t otbr_mdns_fmt_query_get_id(const uint8_t *data, size_t data_len)
{
	uint16_t id = 0;

	if (data_len >= sizeof(struct otbr_mdns_fmt_header)) {
		struct otbr_mdns_fmt_header *hdr = (struct otbr_mdns_fmt_header *)data;

		id = sys_be16_to_cpu(hdr->id);
	}
	return id;
}

char *otbr_mdns_fmt_query_get_str(char *str, size_t str_len, const uint8_t *qmane)
{
	char *result = str;

	while (result && *qmane) {
		if (*qmane < str_len) {
			memcpy(str, qmane + 1, *qmane);
			str += *qmane;
			*(str++) = '.';
			str_len -= (size_t)1 + *qmane;
			qmane += (size_t)1 + *qmane;
		} else {
			result = NULL;
			break;
		}
	}
	if (result) {
		if (str_len) {
			*str = '\0';
		} else {
			result = NULL;
		}
	}
	return result;
}

const char *otbr_mdns_fmt_qtype_str(uint16_t qtype)
{
	switch (qtype) {
	case OTBR_MDNS_FMT_QTYPE_A:
		return "A";
	case OTBR_MDNS_FMT_QTYPE_CNAME:
		return "CNAME";
	case OTBR_MDNS_FMT_QTYPE_PTR:
		return "PTR";
	case OTBR_MDNS_FMT_QTYPE_TXT:
		return "TXT";
	case OTBR_MDNS_FMT_QTYPE_AAAA:
		return "AAAA";
	case OTBR_MDNS_FMT_QTYPE_SRV:
		return "SRV";
	case OTBR_MDNS_FMT_QTYPE_ANY:
		return "ANY";
	default:
		break;
	}
	return "???";
}

const char *otbr_mdns_fmt_qclass_str(uint16_t qclass)
{
	switch (qclass) {
	case OTBR_MDNS_FMT_QCLASS_IN:
		return "IN";
	case OTBR_MDNS_FMT_QCLASS_CH:
		return "CH";
	case OTBR_MDNS_FMT_QCLASS_HS:
		return "HS";
	case OTBR_MDNS_FMT_QCLASS_NONE:
		return "NONE";
	case OTBR_MDNS_FMT_QCLASS_ANY:
		return "ANY";
	default:
		break;
	}
	return "???";
}

static void otbr_mdns_fmt_response_header(uint8_t *data, size_t data_len, size_t *data_pos)
{
	if (!data || !data_pos) {
		return;
	}
	if (*data_pos < sizeof(struct otbr_mdns_fmt_header)) {
		struct otbr_mdns_fmt_header *hdr = (struct otbr_mdns_fmt_header *)data;

		if (data_len >= sizeof(struct otbr_mdns_fmt_header)) {
			memset(hdr, 0, sizeof(struct otbr_mdns_fmt_header));
			hdr->flags = sys_cpu_to_be16(OTBR_MDNS_FMT_FLAGS_MASK_QR);
			*data_pos = sizeof(struct otbr_mdns_fmt_header);
		}
	}
}

static bool otbr_mdns_fmt_response_set_qname(uint8_t *data, size_t data_len, const char *str,
					     size_t str_len)
{
	bool result = false;

	if (!data || !str) {
		return result;
	}
	for (uint8_t *len = NULL;; str++, str_len--) {
		if (!len) {
			if (data_len) {
				len = data;
				*data = 0;
				data++;
				data_len--;
			} else {
				break;
			}
		}
		if (!*str || !str_len) {
			if (*len) {
				if (data_len) {
					*data = 0;
					result = true;
				}
			} else {
				result = true;
			}
			break;
		}
		if (*str == '.') {
			if (*len) {
				len = NULL;
			}
		} else {
			if (data_len) {
				*data = *str;
				*len += 1;
				data++;
				data_len--;
			} else {
				break;
			}
		}
	}
	return result;
}

void otbr_mdns_fmt_response_aaaa(uint8_t *data, size_t data_len, size_t *data_pos,
				 const uint8_t *qmame, const struct in6_addr *ip)
{
	if (!data || !data_pos || !qmame || !ip) {
		return;
	}
	otbr_mdns_fmt_response_header(data, data_len, data_pos);
	if (*data_pos < sizeof(struct otbr_mdns_fmt_header)) {
		return;
	}
	struct otbr_mdns_fmt_header *hdr = (struct otbr_mdns_fmt_header *)data;
	size_t qmame_len = strlen(qmame);

	if (data_len >= *data_pos + qmame_len + 11 + sizeof(struct in6_addr)) {
		uint16_t ancount = sys_be16_to_cpu(hdr->ancount);

		memcpy(data + *data_pos, qmame, qmame_len + 1);
		*data_pos += qmame_len + 1;
		*((uint16_t *)(data + *data_pos)) = sys_cpu_to_be16(OTBR_MDNS_FMT_QTYPE_AAAA);
		*((uint16_t *)(data + *data_pos + 2)) =
			sys_cpu_to_be16(OTBR_MDNS_FMT_QCLASS_IN | OTBR_MDNS_FMT_QCLASS_FLUSH_BIT);
		*((uint32_t *)(data + *data_pos + 4)) = sys_cpu_to_be32(OTBR_MDNS_FMT_TTL);
		*((uint16_t *)(data + *data_pos + 8)) = sys_cpu_to_be16(sizeof(struct in6_addr));
		*data_pos += 10;
		memcpy(data + *data_pos, ip, sizeof(struct in6_addr));
		*data_pos += sizeof(struct in6_addr);
		hdr->ancount = sys_cpu_to_be16(ancount + 1);
	}
}

void otbr_mdns_fmt_response_srv(uint8_t *data, size_t data_len, size_t *data_pos,
				const uint8_t *qmame, const char *host_name, uint16_t port,
				const char *domain_old, const char *domain_new)
{
	if (!data || !data_pos || !qmame || !host_name) {
		return;
	}
	otbr_mdns_fmt_response_header(data, data_len, data_pos);
	if (*data_pos < sizeof(struct otbr_mdns_fmt_header)) {
		return;
	}
	size_t host_name_l = strlen(host_name);
	size_t domain_new_l = 0;

	if (domain_old && domain_new) {
		size_t domain_old_l = strlen(domain_old);

		if ((host_name_l == domain_old_l ||
		     (host_name_l >= domain_old_l &&
		      host_name[host_name_l - domain_old_l - 1] == '.'))) {
			if (!otbr_mdns_fmt_strcasecmp(host_name + host_name_l - domain_old_l,
						      domain_old)) {
				host_name_l -= domain_old_l;
				domain_new_l = strlen(domain_new);
			}
		}
	}
	struct otbr_mdns_fmt_header *hdr = (struct otbr_mdns_fmt_header *)data;
	size_t qmame_len = strlen(qmame);

	if (data_len >= *data_pos + qmame_len + 17 + host_name_l + domain_new_l) {
		uint16_t ancount = sys_be16_to_cpu(hdr->ancount);

		memcpy(data + *data_pos, qmame, qmame_len + 1);
		*data_pos += qmame_len + 1;
		*((uint16_t *)(data + *data_pos)) = sys_cpu_to_be16(OTBR_MDNS_FMT_QTYPE_SRV);
		*((uint16_t *)(data + *data_pos + 2)) =
			sys_cpu_to_be16(OTBR_MDNS_FMT_QCLASS_IN | OTBR_MDNS_FMT_QCLASS_FLUSH_BIT);
		*((uint32_t *)(data + *data_pos + 4)) = sys_cpu_to_be32(OTBR_MDNS_FMT_TTL);
		*((uint16_t *)(data + *data_pos + 8)) =
			sys_cpu_to_be16((size_t)7 + host_name_l + domain_new_l);
		*data_pos += 10;
		*((uint16_t *)(data + *data_pos)) = 0;
		*((uint16_t *)(data + *data_pos + 2)) = 0;
		*((uint16_t *)(data + *data_pos + 4)) = sys_cpu_to_be16(port);
		*data_pos += 6;
		(void)otbr_mdns_fmt_response_set_qname(data + *data_pos, (size_t)1 + host_name_l,
						       host_name, host_name_l);
		*data_pos += (size_t)1 + host_name_l;
		if (domain_new_l) {
			(void)otbr_mdns_fmt_response_set_qname(data + *data_pos - 1,
							       (size_t)1 + domain_new_l, domain_new,
							       domain_new_l);
			*data_pos += domain_new_l;
		}
		hdr->ancount = sys_cpu_to_be16(ancount + 1);
	}
}

void otbr_mdns_fmt_response_set_id(uint8_t *data, size_t data_len, uint16_t id)
{
	if (data_len >= sizeof(struct otbr_mdns_fmt_header)) {
		struct otbr_mdns_fmt_header *hdr = (struct otbr_mdns_fmt_header *)data;

		hdr->id = sys_cpu_to_be16(id);
	}
}

int otbr_mdns_fmt_strcasecmp(const char *str1, const char *str2)
{
	while (*str1 && (tolower(*str1) == tolower(*str2))) {
		str1++;
		str2++;
	}
	return (unsigned char)tolower(*str1) - (unsigned char)tolower(*str2);
}
