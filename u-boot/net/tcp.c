/*
 * PA-220 TCP core - bounded IPv4/TCP segment primitives.
 *
 * This file intentionally contains no NetReceive() hook, packet transmitter,
 * listener, or HTTP dependency.  Those belong to later integration steps.
 *
 * Copyright (C) 2026
 * SPDX-License-Identifier: GPL-2.0+
 */

#include <common.h>
#include <net/tcp.h>

static u16 tcp_get_be16(const u8 *p)
{
	return ((u16)p[0] << 8) | p[1];
}

static u32 tcp_get_be32(const u8 *p)
{
	return ((u32)p[0] << 24) | ((u32)p[1] << 16) |
	       ((u32)p[2] << 8) | (u32)p[3];
}

static u32 tcp_checksum_add(u32 sum, u16 value)
{
	sum += value;
	/* Keep the accumulator bounded even on 32-bit targets. */
	return (sum & 0xffffU) + (sum >> 16);
}

static u16 tcp_checksum_finish(u32 sum)
{
	while (sum >> 16)
		sum = (sum & 0xffffU) + (sum >> 16);

	return (u16)(~sum & 0xffffU);
}

int tcp_segment_parse(const u8 *segment, u16 segment_len,
		      struct tcp_segment_view *view)
{
	u16 header_len;
	u16 offset;
	u8 data_offset;
	u8 kind;
	u8 option_len;

	if (!segment || !view || segment_len < TCP_CORE_HEADER_MIN)
		return -1;

	data_offset = segment[12] >> 4;
	if (data_offset < 5 || data_offset > 15)
		return -1;

	header_len = (u16)data_offset * 4;
	if (header_len > segment_len || header_len > TCP_CORE_HEADER_MAX)
		return -1;

	memset(view, 0, sizeof(*view));
	view->source_port = tcp_get_be16(segment);
	view->dest_port = tcp_get_be16(segment + 2);
	view->sequence = tcp_get_be32(segment + 4);
	view->acknowledgment = tcp_get_be32(segment + 8);
	view->flags = segment[13];
	view->window = tcp_get_be16(segment + 14);
	view->checksum = tcp_get_be16(segment + 16);
	view->urgent = tcp_get_be16(segment + 18);
	view->header_len = header_len;
	view->payload = segment + header_len;
	view->payload_len = segment_len - header_len;

	/*
	 * Parse option TLVs only within the declared TCP header.  Unknown
	 * options are skipped; malformed lengths and duplicate MSS are rejected.
	 */
	offset = TCP_CORE_HEADER_MIN;
	while (offset < header_len) {
		kind = segment[offset];

		if (kind == 0)		/* End of option list */
			break;
		if (kind == 1) {	/* No-operation */
			offset++;
			continue;
		}

		if ((u16)(header_len - offset) < 2)
			return -1;
		option_len = segment[offset + 1];
		if (option_len < 2 || option_len > header_len - offset)
			return -1;

		if (kind == 2) {	/* Maximum Segment Size */
			if (option_len != 4 || view->mss_present)
				return -1;
			view->mss = tcp_get_be16(segment + offset + 2);
			if (view->mss == 0)
				return -1;
			view->mss_present = 1;
		}

		offset += option_len;
	}

	return 0;
}

u16 tcp_checksum_ipv4(const u8 src[4], const u8 dst[4],
		      const u8 *segment, u16 segment_len)
{
	u32 sum = 0;
	u16 i;

	if (!src || !dst || !segment || segment_len < TCP_CORE_HEADER_MIN)
		return 0;

	/* IPv4 pseudo-header: source, destination, zero/protocol, TCP length. */
	for (i = 0; i < 4; i += 2)
		sum = tcp_checksum_add(sum,
				       ((u16)src[i] << 8) | src[i + 1]);
	for (i = 0; i < 4; i += 2)
		sum = tcp_checksum_add(sum,
				       ((u16)dst[i] << 8) | dst[i + 1]);
	sum = tcp_checksum_add(sum, 6); /* zero byte + IPPROTO_TCP */
	sum = tcp_checksum_add(sum, segment_len);

	/* Sum network-order 16-bit words; pad an odd final byte with zero. */
	for (i = 0; i + 1 < segment_len; i += 2)
		sum = tcp_checksum_add(sum,
				       ((u16)segment[i] << 8) | segment[i + 1]);
	if (i < segment_len)
		sum = tcp_checksum_add(sum, (u16)segment[i] << 8);

	return tcp_checksum_finish(sum);
}

int tcp_checksum_ipv4_valid(const u8 src[4], const u8 dst[4],
			    const u8 *segment, u16 segment_len)
{
	if (!src || !dst || !segment || segment_len < TCP_CORE_HEADER_MIN)
		return 0;

	return tcp_checksum_ipv4(src, dst, segment, segment_len) == 0;
}

int tcp_seq_before(u32 a, u32 b)
{
	return (s32)(a - b) < 0;
}

int tcp_seq_after(u32 a, u32 b)
{
	return (s32)(a - b) > 0;
}
