#include <assert.h>
#include <stdio.h>
#include <common.h>
#include <net/tcp.h>

static const u8 local_ip[4] = { 192, 0, 2, 1 };
static const u8 peer_ip[4] = { 192, 0, 2, 2 };
static int send_count;

static u16 finish_sum(u32 sum)
{
	while (sum >> 16)
		sum = (sum & 0xffff) + (sum >> 16);
	return (u16)~sum;
}
static void put16(u8 *p, u16 v) { p[0] = v >> 8; p[1] = v; }
static void put32(u8 *p, u32 v)
{
	p[0] = v >> 24; p[1] = v >> 16; p[2] = v >> 8; p[3] = v;
}
static void make_syn(u8 seg[20])
{
	u32 sum = 0;
	u16 i;
	memset(seg, 0, 20);
	put16(seg, 12345);
	put16(seg + 2, 80);
	put32(seg + 4, 2000);
	seg[12] = 0x50;
	seg[13] = TCP_FLAG_SYN;
	put16(seg + 14, 4096);
	for (i = 0; i < 4; i += 2) {
		sum += ((u16)local_ip[i] << 8) | local_ip[i + 1];
		sum += ((u16)peer_ip[i] << 8) | peer_ip[i + 1];
	}
	sum += 6 + 20;
	for (i = 0; i < 20; i += 2)
		sum += ((u16)seg[i] << 8) | seg[i + 1];
	put16(seg + 16, finish_sum(sum));
}
static int emit_switch(void *arg, const u8 src[4], const u8 dst[4],
		       const u8 *segment, u16 len)
{
	int *fail = arg;
	(void)src; (void)dst; (void)segment; (void)len;
	send_count++;
	return *fail ? -1 : 0;
}
int main(void)
{
	struct tcp_core_connection c;
	u8 syn[20];
	int fail = 0;
	int i;
	u32 interval = TCP_CORE_RETRANSMIT_MS;

	tcp_core_init(&c);
	assert(tcp_core_listen(&c, local_ip, 80, 1000) == 0);
	make_syn(syn);
	assert(tcp_core_input(&c, peer_ip, local_ip, syn, sizeof(syn), 0,
			      emit_switch, &fail) == 1);
	assert(c.state == TCP_CORE_SYN_RECEIVED && c.pending);
	assert(send_count == 1);

	fail = 1;
	assert(tcp_core_input(&c, peer_ip, local_ip, syn, sizeof(syn), 100,
			      emit_switch, &fail) == -1);
	assert(c.state == TCP_CORE_SYN_RECEIVED && c.pending);
	assert(c.snd_nxt == 1001);
	assert(tcp_core_poll(&c, interval, emit_switch, &fail) == -1);
	assert(c.retries == 1 && c.retry_deadline == interval * 2);
	i = send_count;
	assert(tcp_core_poll(&c, interval, emit_switch, &fail) == 0);
	assert(send_count == i);
	for (i = 2; i <= TCP_CORE_MAX_RETRIES; i++) {
		int rc = tcp_core_poll(&c, (u32)i * interval,
				       emit_switch, &fail);
		if (i == TCP_CORE_MAX_RETRIES)
			assert(rc == 1);
		else
			assert(rc == -1);
	}
	assert(c.state == TCP_CORE_LISTEN);
	assert(!c.pending);
	puts("tcp_core_retry: PASS");
	return 0;
}
