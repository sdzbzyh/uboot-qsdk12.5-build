#include <assert.h>
#include <stdio.h>
#include <common.h>
#include <net/tcp.h>

static const u8 local_ip[4] = { 192, 0, 2, 1 };
static const u8 peer_ip[4] = { 192, 0, 2, 2 };

struct capture {
	int count;
	u8 flags;
	u32 sequence;
	u32 acknowledgment;
	int fail;
};

static void put16(u8 *p, u16 v)
{
	p[0] = (u8)(v >> 8);
	p[1] = (u8)v;
}

static void put32(u8 *p, u32 v)
{
	p[0] = (u8)(v >> 24);
	p[1] = (u8)(v >> 16);
	p[2] = (u8)(v >> 8);
	p[3] = (u8)v;
}

static void make_segment(u8 seg[20], u32 seq, u32 ack, u8 flags)
{
	memset(seg, 0, 20);
	put16(seg, 12345);
	put16(seg + 2, 80);
	put32(seg + 4, seq);
	put32(seg + 8, ack);
	seg[12] = 0x50;
	seg[13] = flags;
	put16(seg + 14, 4096);
	put16(seg + 16, tcp_checksum_ipv4(peer_ip, local_ip, seg, 20));
	assert(tcp_checksum_ipv4_valid(peer_ip, local_ip, seg, 20));
}

static int capture_emit(void *arg, const u8 src[4], const u8 dst[4],
			const u8 *segment, u16 len)
{
	struct capture *cap = arg;

	assert(src && dst && segment && len >= TCP_CORE_HEADER_MIN);
	cap->count++;
	cap->flags = segment[13];
	cap->sequence = ((u32)segment[4] << 24) |
			((u32)segment[5] << 16) |
			((u32)segment[6] << 8) | segment[7];
	cap->acknowledgment = ((u32)segment[8] << 24) |
			      ((u32)segment[9] << 16) |
			      ((u32)segment[10] << 8) | segment[11];
	return cap->fail ? -1 : 0;
}

int main(void)
{
	struct tcp_core_connection c;
	struct capture cap = { 0, 0, 0, 0, 0 };
	u8 seg[20];
	int emitted;

	tcp_core_init(&c);
	assert(tcp_core_listen(&c, local_ip, 80, 1000) == 0);

	/* Passive open and complete the three-way handshake. */
	make_segment(seg, 2000, 0, TCP_FLAG_SYN);
	assert(tcp_core_input(&c, peer_ip, local_ip, seg, sizeof(seg), 0,
			      capture_emit, &cap) == 1);
	assert(c.state == TCP_CORE_SYN_RECEIVED);
	make_segment(seg, 2001, 1001, TCP_FLAG_ACK);
	assert(tcp_core_input(&c, peer_ip, local_ip, seg, sizeof(seg), 1,
			      capture_emit, &cap) == 1);
	assert(c.state == TCP_CORE_ESTABLISHED);

	/* The peer FIN is acknowledged and leaves the core in CLOSE-WAIT. */
	make_segment(seg, 2001, 1001, TCP_FLAG_FIN | TCP_FLAG_ACK);
	assert(tcp_core_input(&c, peer_ip, local_ip, seg, sizeof(seg), 2,
			      capture_emit, &cap) == 1);
	assert(c.state == TCP_CORE_CLOSE_WAIT);
	assert(c.fin_received);
	assert(cap.flags == TCP_FLAG_ACK);
	assert(cap.acknowledgment == 2002);

	/* The application close policy emits exactly one local FIN. */
	assert(tcp_core_close(&c, 3, capture_emit, &cap) == 0);
	assert(c.state == TCP_CORE_LAST_ACK);
	assert(c.pending);
	assert(cap.flags == (TCP_FLAG_FIN | TCP_FLAG_ACK));
	assert(cap.sequence == 1001);
	assert(cap.acknowledgment == 2002);
	emitted = cap.count;
	assert(tcp_core_close(&c, 4, capture_emit, &cap) == -1);
	assert(cap.count == emitted);

	/* The peer's final ACK clears the connection and restores LISTEN. */
	make_segment(seg, 2002, 1002, TCP_FLAG_ACK);
	assert(tcp_core_input(&c, peer_ip, local_ip, seg, sizeof(seg), 4,
			      capture_emit, &cap) == 1);
	assert(c.state == TCP_CORE_LISTEN);
	assert(c.local_port == 80);
	assert(c.remote_port == 0);
	assert(!c.pending);

	/* A subsequent SYN proves that the listener can accept another peer. */
	make_segment(seg, 5000, 0, TCP_FLAG_SYN);
	assert(tcp_core_input(&c, peer_ip, local_ip, seg, sizeof(seg), 5,
			      capture_emit, &cap) == 1);
	assert(c.state == TCP_CORE_SYN_RECEIVED);
	assert(c.irs == 5000);

	puts("tcp_close_wait: PASS");
	return 0;
}
