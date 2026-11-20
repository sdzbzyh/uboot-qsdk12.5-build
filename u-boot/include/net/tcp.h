/*
 * PA-220 TCP core - bounded IPv4/TCP segment primitives.
 *
 * This module is deliberately independent of NetReceive() until the
 * network-dispatch integration is reviewed in a separate change.
 *
 * Copyright (C) 2026
 * SPDX-License-Identifier: GPL-2.0+
 */

#ifndef __PA220_NET_TCP_H__
#define __PA220_NET_TCP_H__

#include <linux/types.h>

/*
 * First implementation limits.  These are protocol limits for the planned
 * single-connection service, not an assertion that a listener is integrated.
 */
#define TCP_CORE_MAX_MSS          536
#define TCP_CORE_RX_CAPACITY      4096
#define TCP_CORE_TX_CAPACITY      1460
#define TCP_CORE_MAX_RETRIES      5
#define TCP_CORE_RETRANSMIT_MS    1000
#define TCP_CORE_HEADER_MIN       20
#define TCP_CORE_HEADER_MAX       60

#define TCP_FLAG_FIN              0x01
#define TCP_FLAG_SYN              0x02
#define TCP_FLAG_RST              0x04
#define TCP_FLAG_PSH              0x08
#define TCP_FLAG_ACK              0x10
#define TCP_FLAG_URG              0x20
#define TCP_FLAG_ECE              0x40
#define TCP_FLAG_CWR              0x80

/*
 * A view into the current RX packet.  payload is transient and MUST NOT be
 * retained after the receive callback returns.  A connection implementation
 * must copy accepted bytes into its own bounded storage.
 */
struct tcp_segment_view {
	u16 source_port;
	u16 dest_port;
	u32 sequence;
	u32 acknowledgment;
	u16 window;
	u16 checksum;
	u16 urgent;
	u16 header_len;
	u16 payload_len;
	u16 mss;
	u8 flags;
	u8 mss_present;
	const u8 *payload;
};

/*
 * Parse a TCP segment without assuming alignment or host byte order.
 * Returns 0 on success, -1 for a truncated or malformed segment/options.
 */
int tcp_segment_parse(const u8 *segment, u16 segment_len,
		      struct tcp_segment_view *view);

/*
 * Calculate the IPv4 TCP checksum over the pseudo-header and complete TCP
 * segment.  src/dst point to four network-order address bytes.  The returned
 * checksum is a host-order 16-bit value suitable for writing big-endian.
 * For validation, call tcp_checksum_ipv4_valid() with the received checksum
 * still present in the segment.
 */
u16 tcp_checksum_ipv4(const u8 src[4], const u8 dst[4],
		      const u8 *segment, u16 segment_len);
int tcp_checksum_ipv4_valid(const u8 src[4], const u8 dst[4],
			    const u8 *segment, u16 segment_len);

/* TCP serial-number comparisons (RFC 9293 modulo-2^32 arithmetic). */
int tcp_seq_before(u32 a, u32 b);
int tcp_seq_after(u32 a, u32 b);

#endif /* __PA220_NET_TCP_H__ */
