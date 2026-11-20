/*
 * PA-220 TCP core - bounded IPv4/TCP segment and single-connection engine.
 *
 * RX payload pointers are transient. Output callbacks are synchronous and
 * must consume/copy the supplied segment before returning.
 *
 * Copyright (C) 2026
 * SPDX-License-Identifier: GPL-2.0+
 */
#ifndef __PA220_NET_TCP_H__
#define __PA220_NET_TCP_H__

#include <linux/types.h>

#define TCP_CORE_MAX_MSS          536
#define TCP_CORE_RX_CAPACITY      4096
#define TCP_CORE_TX_CAPACITY      1460
#define TCP_CORE_MAX_RETRIES      5
#define TCP_CORE_RETRANSMIT_MS    1000
#define TCP_CORE_IDLE_TIMEOUT_MS  30000
#define TCP_CORE_TIME_WAIT_MS     60000
#define TCP_CORE_HEADER_MIN       20
#define TCP_CORE_HEADER_MAX       60
#define TCP_CORE_WIRE_MAX         (TCP_CORE_HEADER_MAX + TCP_CORE_TX_CAPACITY)

#define TCP_FLAG_FIN              0x01
#define TCP_FLAG_SYN              0x02
#define TCP_FLAG_RST              0x04
#define TCP_FLAG_PSH              0x08
#define TCP_FLAG_ACK              0x10
#define TCP_FLAG_URG              0x20
#define TCP_FLAG_ECE              0x40
#define TCP_FLAG_CWR              0x80

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

enum tcp_core_state {
	TCP_CORE_CLOSED = 0,
	TCP_CORE_LISTEN,
	TCP_CORE_SYN_RECEIVED,
	TCP_CORE_ESTABLISHED,
	TCP_CORE_FIN_WAIT_1,
	TCP_CORE_FIN_WAIT_2,
	TCP_CORE_CLOSE_WAIT,
	TCP_CORE_CLOSING,
	TCP_CORE_LAST_ACK,
	TCP_CORE_TIME_WAIT
};

/*
 * emit() is called synchronously with a complete TCP segment (no IP header).
 * src/dst are four network-order IPv4 bytes. The callback must not retain
 * segment; return 0 on accepted transmission, non-zero on failure.
 */
typedef int (*tcp_core_emit_fn)(void *arg, const u8 src[4],
				const u8 dst[4], const u8 *segment,
				u16 segment_len);

struct tcp_core_connection {
	enum tcp_core_state state;
	u8 local_ip[4];
	u8 remote_ip[4];
	u16 local_port;
	u16 remote_port;
	u32 iss;
	u32 snd_una;
	u32 snd_nxt;
	u32 irs;
	u32 rcv_nxt;
	u16 peer_window;
	u16 peer_mss;
	u16 rx_len;
	u8 rx_data[TCP_CORE_RX_CAPACITY];
	u16 tx_len;
	u8 tx_data[TCP_CORE_TX_CAPACITY];
	u8 pending_flags;
	u8 pending;
	u8 retries;
	u8 fin_received;
	u32 retry_deadline;
	u32 last_activity;
	u32 time_wait_deadline;
	u8 tx_wire[TCP_CORE_WIRE_MAX];
	u8 retry_data[TCP_CORE_TX_CAPACITY];
	u16 retry_len;
	u8 retry_flags;
	u32 retry_seq;
};

int tcp_segment_parse(const u8 *segment, u16 segment_len,
		      struct tcp_segment_view *view);
u16 tcp_checksum_ipv4(const u8 src[4], const u8 dst[4],
		      const u8 *segment, u16 segment_len);
int tcp_checksum_ipv4_valid(const u8 src[4], const u8 dst[4],
			    const u8 *segment, u16 segment_len);
int tcp_seq_before(u32 a, u32 b);
int tcp_seq_after(u32 a, u32 b);

/* One static connection is owned by the caller; no allocation is performed. */
void tcp_core_init(struct tcp_core_connection *conn);
int tcp_core_listen(struct tcp_core_connection *conn, const u8 local_ip[4],
		    u16 local_port, u32 initial_sequence);
int tcp_core_input(struct tcp_core_connection *conn,
		   const u8 src_ip[4], const u8 dst_ip[4],
		   const u8 *segment, u16 segment_len, u32 now_ms,
		   tcp_core_emit_fn emit, void *emit_arg);
int tcp_core_poll(struct tcp_core_connection *conn, u32 now_ms,
		  tcp_core_emit_fn emit, void *emit_arg);
int tcp_core_read(struct tcp_core_connection *conn, u8 *dst, u16 capacity);
int tcp_core_write(struct tcp_core_connection *conn, const u8 *data,
		   u16 length, u32 now_ms, tcp_core_emit_fn emit,
		   void *emit_arg);
int tcp_core_close(struct tcp_core_connection *conn, u32 now_ms,
		   tcp_core_emit_fn emit, void *emit_arg);

#endif /* __PA220_NET_TCP_H__ */
