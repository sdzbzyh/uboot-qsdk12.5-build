/*
 * PA-220 TCP core - bounded passive-open, single-connection TCP engine.
 *
 * Deliberately independent of NetReceive(), Ethernet drivers and NetLoop().
 * All timers are driven by the caller through tcp_core_poll(now_ms).
 *
 * Copyright (C) 2026
 * SPDX-License-Identifier: GPL-2.0+
 */
#include <common.h>
#include <net/tcp.h>

/*
 * Keep the complete public TCP core API reachable when the PA-220 final
 * link uses --gc-sections. The native network layer references this table
 * only when a TCP input handler is explicitly registered.
 */
struct tcp_core_link_refs {
 int (*segment_parse)(const u8 *, u16, struct tcp_segment_view *);
 u16 (*checksum_ipv4)(const u8 [4], const u8 [4], const u8 *, u16);
 int (*checksum_ipv4_valid)(const u8 [4], const u8 [4], const u8 *, u16);
 int (*seq_before)(u32, u32);
 int (*seq_after)(u32, u32);
 void (*init)(struct tcp_core_connection *);
 int (*listen)(struct tcp_core_connection *, const u8 [4], u16, u32);
 int (*input)(struct tcp_core_connection *, const u8 [4], const u8 [4],
              const u8 *, u16, u32, tcp_core_emit_fn, void *);
 int (*poll)(struct tcp_core_connection *, u32, tcp_core_emit_fn, void *);
 int (*read)(struct tcp_core_connection *, u8 *, u16);
 int (*write)(struct tcp_core_connection *, const u8 *, u16, u32,
              tcp_core_emit_fn, void *);
 int (*close)(struct tcp_core_connection *, u32, tcp_core_emit_fn, void *);
};

static const struct tcp_core_link_refs tcp_core_api_refs = {
 .segment_parse = tcp_segment_parse,
 .checksum_ipv4 = tcp_checksum_ipv4,
 .checksum_ipv4_valid = tcp_checksum_ipv4_valid,
 .seq_before = tcp_seq_before,
 .seq_after = tcp_seq_after,
 .init = tcp_core_init,
 .listen = tcp_core_listen,
 .input = tcp_core_input,
 .poll = tcp_core_poll,
 .read = tcp_core_read,
 .write = tcp_core_write,
 .close = tcp_core_close
};

const void *tcp_core_link_anchor(void)
{
 return &tcp_core_api_refs;
}


static u16 tcp_get_be16(const u8 *p)
{
	return ((u16)p[0] << 8) | p[1];
}

static u32 tcp_get_be32(const u8 *p)
{
	return ((u32)p[0] << 24) | ((u32)p[1] << 16) |
	       ((u32)p[2] << 8) | (u32)p[3];
}

static void tcp_put_be16(u8 *p, u16 v)
{
	p[0] = (u8)(v >> 8);
	p[1] = (u8)v;
}

static void tcp_put_be32(u8 *p, u32 v)
{
	p[0] = (u8)(v >> 24);
	p[1] = (u8)(v >> 16);
	p[2] = (u8)(v >> 8);
	p[3] = (u8)v;
}

static u32 tcp_checksum_add(u32 sum, u16 value)
{
	sum += value;
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
	if (data_offset < 5 || data_offset > 15 ||
	    (segment[12] & 0x0f))
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

	offset = TCP_CORE_HEADER_MIN;
	while (offset < header_len) {
		kind = segment[offset];
		if (kind == 0)
			break;
		if (kind == 1) {
			offset++;
			continue;
		}
		if ((u16)(header_len - offset) < 2)
			return -1;
		option_len = segment[offset + 1];
		if (option_len < 2 || option_len > header_len - offset)
			return -1;
		if (kind == 2) {
			if (option_len != 4 || view->mss_present)
				return -1;
			view->mss = tcp_get_be16(segment + offset + 2);
			if (!view->mss)
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
	for (i = 0; i < 4; i += 2)
		sum = tcp_checksum_add(sum,
				       ((u16)src[i] << 8) | src[i + 1]);
	for (i = 0; i < 4; i += 2)
		sum = tcp_checksum_add(sum,
				       ((u16)dst[i] << 8) | dst[i + 1]);
	sum = tcp_checksum_add(sum, 6);
	sum = tcp_checksum_add(sum, segment_len);
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

static int tcp_time_reached(u32 now, u32 deadline)
{
	return (s32)(now - deadline) >= 0;
}

static int tcp_ip_equal(const u8 a[4], const u8 b[4])
{
	return a[0] == b[0] && a[1] == b[1] &&
	       a[2] == b[2] && a[3] == b[3];
}

static u16 tcp_rx_window(const struct tcp_core_connection *c)
{
	return (u16)(TCP_CORE_RX_CAPACITY - c->rx_len);
}

static int tcp_emit(struct tcp_core_connection *c, u8 flags, u32 seq,
		    u32 ack, const u8 *payload, u16 payload_len,
		    int syn_options, tcp_core_emit_fn emit, void *arg)
{
	u8 *p = c->tx_wire;
	u16 hlen = syn_options ? 24 : TCP_CORE_HEADER_MIN;
	u16 total = hlen + payload_len;
	u16 sum;

	if (!emit || payload_len > TCP_CORE_TX_CAPACITY ||
	    total > sizeof(c->tx_wire))
		return -1;
	memset(p, 0, total);
	tcp_put_be16(p, c->local_port);
	tcp_put_be16(p + 2, c->remote_port);
	tcp_put_be32(p + 4, seq);
	tcp_put_be32(p + 8, ack);
	p[12] = (u8)((hlen / 4) << 4);
	p[13] = flags;
	tcp_put_be16(p + 14, tcp_rx_window(c));
	if (syn_options) {
		p[20] = 2;
		p[21] = 4;
		tcp_put_be16(p + 22, TCP_CORE_MAX_MSS);
	}
	if (payload_len)
		memcpy(p + hlen, payload, payload_len);
	sum = tcp_checksum_ipv4(c->local_ip, c->remote_ip, p, total);
	tcp_put_be16(p + 16, sum);
	return emit(arg, c->local_ip, c->remote_ip, p, total);
}

static void tcp_clear_pending(struct tcp_core_connection *c)
{
	c->pending = 0;
	c->pending_flags = 0;
	c->retry_len = 0;
	c->retries = 0;
}

static void tcp_abort_connection(struct tcp_core_connection *c)
{
	u8 local[4];
	u16 port;
	u32 next_iss;

	memcpy(local, c->local_ip, 4);
	port = c->local_port;
	next_iss = c->iss + 0x10001U;
	memset(c, 0, sizeof(*c));
	memcpy(c->local_ip, local, 4);
	c->local_port = port;
	c->iss = next_iss;
	c->snd_una = next_iss;
	c->snd_nxt = next_iss;
	c->state = port ? TCP_CORE_LISTEN : TCP_CORE_CLOSED;
}

/* Save exactly one outstanding SYN, FIN or data range for bounded retry. */
static int tcp_send_reliable(struct tcp_core_connection *c, u8 flags,
			     u32 seq, const u8 *data, u16 len, u32 now,
			     int syn_options, tcp_core_emit_fn emit, void *arg)
{
	if (c->pending || len > TCP_CORE_TX_CAPACITY)
		return -1;
	if (tcp_emit(c, flags, seq, c->rcv_nxt, data, len,
		     syn_options, emit, arg))
		return -1;
	if (len)
		memcpy(c->retry_data, data, len);
	c->retry_len = len;
	c->retry_flags = flags;
	c->retry_seq = seq;
	c->pending_flags = flags;
	c->pending = 1;
	c->retries = 0;
	c->retry_deadline = now + TCP_CORE_RETRANSMIT_MS;
	return 0;
}

static int tcp_send_ack(struct tcp_core_connection *c,
			tcp_core_emit_fn emit, void *arg)
{
	return tcp_emit(c, TCP_FLAG_ACK, c->snd_nxt, c->rcv_nxt,
			NULL, 0, 0, emit, arg);
}

static void tcp_ack_pending(struct tcp_core_connection *c, u32 ack)
{
	u32 advance;

	if (!c->pending || !tcp_seq_after(ack, c->snd_una))
		return;
	if (tcp_seq_after(ack, c->snd_nxt))
		return;
	advance = ack - c->snd_una;
	if (c->retry_flags & TCP_FLAG_SYN) {
		if (advance >= 1)
			tcp_clear_pending(c);
	} else if (c->retry_flags & TCP_FLAG_FIN) {
		if (advance >= 1)
			tcp_clear_pending(c);
	} else {
		if (advance >= c->retry_len) {
			tcp_clear_pending(c);
		} else if (advance) {
			memmove(c->retry_data, c->retry_data + advance,
				c->retry_len - advance);
			c->retry_len -= (u16)advance;
			c->retry_seq = ack;
			c->retries = 0;
		}
	}
	c->snd_una = ack;
}

void tcp_core_init(struct tcp_core_connection *conn)
{
	if (conn)
		memset(conn, 0, sizeof(*conn));
}

int tcp_core_listen(struct tcp_core_connection *conn, const u8 local_ip[4],
		    u16 local_port, u32 initial_sequence)
{
	if (!conn || !local_ip || !local_port)
		return -1;
	tcp_core_init(conn);
	memcpy(conn->local_ip, local_ip, 4);
	conn->local_port = local_port;
	conn->iss = initial_sequence;
	conn->snd_una = initial_sequence;
	conn->snd_nxt = initial_sequence;
	conn->state = TCP_CORE_LISTEN;
	return 0;
}


int tcp_core_input(struct tcp_core_connection *c,
		   const u8 src_ip[4], const u8 dst_ip[4],
		   const u8 *segment, u16 segment_len, u32 now,
		   tcp_core_emit_fn emit, void *arg)
{
	struct tcp_segment_view v;
	u32 seg_end;
	u16 free_space;
	u16 accepted;
	int matched;

	if (!c || !src_ip || !dst_ip || !segment || !emit ||
	    tcp_segment_parse(segment, segment_len, &v))
		return -1;
	if (!tcp_checksum_ipv4_valid(src_ip, dst_ip, segment, segment_len))
		return -1;

	if (c->state == TCP_CORE_CLOSED)
		return 0;
	if (c->state == TCP_CORE_LISTEN) {
		if (v.dest_port != c->local_port || !v.source_port ||
		    (v.flags & (TCP_FLAG_URG | TCP_FLAG_ECE | TCP_FLAG_CWR)) ||
		    !(v.flags & TCP_FLAG_SYN) ||
		    (v.flags & (TCP_FLAG_ACK | TCP_FLAG_RST | TCP_FLAG_FIN)) ||
		    v.payload_len)
			return 0;
		memcpy(c->remote_ip, src_ip, 4);
		c->remote_port = v.source_port;
		if (!tcp_ip_equal(dst_ip, c->local_ip)) {
			c->remote_port = 0;
			return 0;
		}
		c->irs = v.sequence;
		c->rcv_nxt = v.sequence + 1;
		c->peer_window = v.window;
		c->peer_mss = v.mss_present ? v.mss : 536;
		if (!c->peer_mss || c->peer_mss > TCP_CORE_MAX_MSS)
			c->peer_mss = TCP_CORE_MAX_MSS;
		c->snd_una = c->iss;
		c->snd_nxt = c->iss + 1;
		c->state = TCP_CORE_SYN_RECEIVED;
		c->last_activity = now;
		if (tcp_send_reliable(c, TCP_FLAG_SYN | TCP_FLAG_ACK,
				      c->iss, NULL, 0, now, 1, emit, arg)) {
			c->state = TCP_CORE_LISTEN;
			c->remote_port = 0;
			return -1;
		}
		return 1;
	}

	matched = v.dest_port == c->local_port &&
		  v.source_port == c->remote_port &&
		  tcp_ip_equal(src_ip, c->remote_ip) &&
		  tcp_ip_equal(dst_ip, c->local_ip);
	if (!matched)
		return 0;

	if (v.flags & TCP_FLAG_RST) {
		if (v.sequence == c->rcv_nxt)
			tcp_abort_connection(c);
		return 1;
	}
	if (c->state == TCP_CORE_SYN_RECEIVED) {
		if ((v.flags & TCP_FLAG_SYN) && v.sequence == c->irs) {
			if (c->pending &&
			    tcp_emit(c, c->retry_flags, c->retry_seq,
				     c->rcv_nxt, NULL, 0, 1, emit, arg))
				return -1;
			return 1;
		}
		if (!(v.flags & TCP_FLAG_ACK) ||
		    v.acknowledgment != c->snd_nxt ||
		    v.sequence != c->rcv_nxt || v.payload_len ||
		    (v.flags & (TCP_FLAG_SYN | TCP_FLAG_FIN)))
			return 0;
		tcp_ack_pending(c, v.acknowledgment);
		c->peer_window = v.window;
		c->state = TCP_CORE_ESTABLISHED;
		c->last_activity = now;
		return 1;
	}

	if (v.flags & TCP_FLAG_SYN)
		return 0;
	if (v.flags & (TCP_FLAG_URG | TCP_FLAG_ECE | TCP_FLAG_CWR))
		return 0;
	/* ACK is mandatory after the three-way handshake. */
	if (!(v.flags & TCP_FLAG_ACK))
		return 0;
	/* This minimal core accepts only the exact next receive sequence. */
	if (v.sequence != c->rcv_nxt) {
		if (v.payload_len || (v.flags & TCP_FLAG_FIN))
			return tcp_send_ack(c, emit, arg);
		return 0;
	}
	if (v.flags & TCP_FLAG_ACK) {
		if (tcp_seq_after(v.acknowledgment, c->snd_nxt) ||
		    tcp_seq_before(v.acknowledgment, c->snd_una))
			return 0;
		tcp_ack_pending(c, v.acknowledgment);
		c->peer_window = v.window;
		if (c->state == TCP_CORE_FIN_WAIT_1 && !c->pending)
			c->state = c->fin_received ?
				TCP_CORE_TIME_WAIT : TCP_CORE_FIN_WAIT_2;
		else if (c->state == TCP_CORE_CLOSING && !c->pending)
			c->state = TCP_CORE_TIME_WAIT;
		else if (c->state == TCP_CORE_LAST_ACK && !c->pending) {
			tcp_abort_connection(c);
			return 1;
		}
	}
	c->last_activity = now;

	seg_end = v.sequence + v.payload_len +
		  ((v.flags & TCP_FLAG_FIN) ? 1 : 0);
	/*
	 * In CLOSE-WAIT the peer has closed its send direction.  Continue
	 * processing ACKs above, but ignore any subsequent segment payload.
	 */
	if (v.payload_len && c->state != TCP_CORE_CLOSE_WAIT) {
		if (v.sequence != c->rcv_nxt ||
		    v.payload_len > TCP_CORE_MAX_MSS)
			return tcp_send_ack(c, emit, arg);
		free_space = tcp_rx_window(c);
		accepted = v.payload_len;
		if (accepted > free_space)
			return tcp_send_ack(c, emit, arg);
		if (accepted) {
			memcpy(c->rx_data + c->rx_len, v.payload, accepted);
			c->rx_len += accepted;
			c->rcv_nxt += accepted;
		}
	}
	if (v.flags & TCP_FLAG_FIN) {
		if (seg_end - 1 != c->rcv_nxt)
			return tcp_send_ack(c, emit, arg);
		c->rcv_nxt++;
		c->fin_received = 1;
		if (c->state == TCP_CORE_ESTABLISHED)
			c->state = TCP_CORE_CLOSE_WAIT;
		else if (c->state == TCP_CORE_FIN_WAIT_1)
			c->state = c->pending ? TCP_CORE_CLOSING :
				TCP_CORE_TIME_WAIT;
		else if (c->state == TCP_CORE_FIN_WAIT_2)
			c->state = TCP_CORE_TIME_WAIT;
		if (c->state == TCP_CORE_TIME_WAIT)
			c->time_wait_deadline = now + TCP_CORE_TIME_WAIT_MS;
	}
	if (v.payload_len || (v.flags & TCP_FLAG_FIN))
		return tcp_send_ack(c, emit, arg);
	return 0;
}

int tcp_core_poll(struct tcp_core_connection *c, u32 now,
		  tcp_core_emit_fn emit, void *arg)
{
	if (!c)
		return -1;
	if (c->state == TCP_CORE_CLOSED || c->state == TCP_CORE_LISTEN)
		return 0;
	if (c->state == TCP_CORE_TIME_WAIT) {
		if (tcp_time_reached(now, c->time_wait_deadline))
			tcp_abort_connection(c);
		return 0;
	}
	if (tcp_time_reached(now, c->last_activity + TCP_CORE_IDLE_TIMEOUT_MS)) {
		tcp_abort_connection(c);
		return 1;
	}
	if (!c->pending || !tcp_time_reached(now, c->retry_deadline))
		return 0;
	if (c->retries >= TCP_CORE_MAX_RETRIES) {
		tcp_abort_connection(c);
		return 1;
	}
	if (tcp_emit(c, c->retry_flags, c->retry_seq, c->rcv_nxt,
		     c->retry_len ? c->retry_data : NULL, c->retry_len,
		     !!(c->retry_flags & TCP_FLAG_SYN), emit, arg)) {
		c->retries++;
		if (c->retries >= TCP_CORE_MAX_RETRIES) {
			tcp_abort_connection(c);
			return 1;
		}
		c->retry_deadline = now + TCP_CORE_RETRANSMIT_MS;
		return -1;
	}
	c->retries++;
	c->retry_deadline = now + TCP_CORE_RETRANSMIT_MS;
	return 1;
}

int tcp_core_read(struct tcp_core_connection *c, u8 *dst, u16 capacity)
{
	u16 count;

	if (!c || (!dst && capacity))
		return -1;
	count = c->rx_len < capacity ? c->rx_len : capacity;
	if (count) {
		memcpy(dst, c->rx_data, count);
		memmove(c->rx_data, c->rx_data + count, c->rx_len - count);
		c->rx_len -= count;
	}
	return count;
}

int tcp_core_write(struct tcp_core_connection *c, const u8 *data,
		   u16 length, u32 now, tcp_core_emit_fn emit, void *arg)
{
	u16 limit;

	if (!c || !data || !length ||
	    (c->state != TCP_CORE_ESTABLISHED &&
	     c->state != TCP_CORE_CLOSE_WAIT) ||
	    c->pending)
		return -1;
	limit = c->peer_mss;
	if (limit > TCP_CORE_TX_CAPACITY)
		limit = TCP_CORE_TX_CAPACITY;
	if (length > limit)
		return -1;
	if (!c->peer_window || length > c->peer_window)
		return 0;
	memcpy(c->tx_data, data, length);
	if (tcp_send_reliable(c, TCP_FLAG_ACK | TCP_FLAG_PSH,
			      c->snd_nxt, c->tx_data, length, now, 0,
			      emit, arg))
		return -1;
	c->snd_nxt += length;
	c->tx_len = length;
	return length;
}

int tcp_core_close(struct tcp_core_connection *c, u32 now,
		   tcp_core_emit_fn emit, void *arg)
{
	enum tcp_core_state next;

	if (!c || c->pending ||
	    (c->state != TCP_CORE_ESTABLISHED &&
	     c->state != TCP_CORE_CLOSE_WAIT))
		return -1;
	next = c->state == TCP_CORE_ESTABLISHED ?
		TCP_CORE_FIN_WAIT_1 : TCP_CORE_LAST_ACK;
	if (tcp_send_reliable(c, TCP_FLAG_FIN | TCP_FLAG_ACK,
			      c->snd_nxt, NULL, 0, now, 0, emit, arg))
		return -1;
	c->snd_nxt++;
	c->state = next;
	return 0;
}
