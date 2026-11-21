#include <assert.h>
#include <stdio.h>
#include <common.h>
#include <net.h>
#include <net/tcp.h>
#include <net/tcp_ipv4_tx.h>

IPaddr_t NetOurIP;
IPaddr_t NetOurSubnetMask;
uchar NetOurEther[6] = { 0x02, 0x10, 0x20, 0x30, 0x40, 0x50 };
static u8 captured[1600];
static int captured_len, send_result, send_count;

static void put16(u8 *p, u16 v) { p[0] = v >> 8; p[1] = (u8)v; }
static u16 get16(const u8 *p) { return ((u16)p[0] << 8) | p[1]; }
static void put32(u8 *p, u32 v)
{
	p[0] = v >> 24; p[1] = v >> 16; p[2] = v >> 8; p[3] = v;
}
int NetSetEther(uchar *pkt, uchar *addr, uint prot)
{
	memcpy(pkt, addr, 6);
	memcpy(pkt + 6, NetOurEther, 6);
	put16(pkt + 12, (u16)prot);
	return ETHER_HDR_SIZE;
}
void net_set_ip_header(uchar *pkt, IPaddr_t dest, IPaddr_t source)
{
	struct ip_udp_hdr *ip = (struct ip_udp_hdr *)pkt;
	memset(ip, 0, IP_HDR_SIZE);
	ip->ip_hl_v = 0x45;
	ip->ip_ttl = 255;
	memcpy(&ip->ip_src, &source, 4);
	memcpy(&ip->ip_dst, &dest, 4);
}
uint NetCksum(uchar *ptr, int len)
{
	u32 sum = 0;
	u16 *p = (u16 *)ptr;
	while (len-- > 0)
		sum += *p++;
	sum = (sum & 0xffff) + (sum >> 16);
	sum = (sum & 0xffff) + (sum >> 16);
	return sum & 0xffff;
}
int eth_send(void *packet, int length)
{
	send_count++;
	if (send_result)
		return send_result;
	memcpy(captured, packet, length);
	captured_len = length;
	return 0;
}
static int ip_checksum_valid(const u8 *ip)
{
	u32 sum = 0;
	int i;
	for (i = 0; i < IP_HDR_SIZE; i += 2)
		sum += ((u16)ip[i] << 8) | ip[i + 1];
	while (sum >> 16)
		sum = (sum & 0xffff) + (sum >> 16);
	return sum == 0xffff;
}
static void setup(void)
{
	static const u8 local[4] = { 192, 0, 2, 10 };
	static const u8 mask[4] = { 255, 255, 255, 0 };
	memcpy(&NetOurIP, local, 4);
	memcpy(&NetOurSubnetMask, mask, 4);
	captured_len = send_count = send_result = 0;
}
static void make_syn(u8 syn[24], const u8 remote[4], const u8 local[4])
{
	memset(syn, 0, 24);
	put16(syn, 40000);
	put16(syn + 2, 80);
	put32(syn + 4, 0x10203040);
	syn[12] = 0x60;
	syn[13] = TCP_FLAG_SYN;
	put16(syn + 14, 4096);
	syn[20] = 2;
	syn[21] = 4;
	put16(syn + 22, 1460);
	put16(syn + 16, tcp_checksum_ipv4(remote, local, syn, 24));
}
int main(void)
{
	struct tcp_core_connection conn, failed_conn;
	struct tcp_ipv4_tx_peer peer, failed_peer;
	struct tcp_segment_view view;
	static const u8 local[4] = { 192, 0, 2, 10 };
	static const u8 remote[4] = { 192, 0, 2, 20 };
	static const u8 remote_mac[6] = { 0x02, 0xaa, 0xbb, 0xcc, 0xdd, 0xee };
	const u32 iss = 0x55667788;
	u8 syn[24], *ip, *tcp;

	setup();
	tcp_ipv4_tx_clear(&peer);
	assert(tcp_ipv4_tx_learn_peer(&peer, remote, remote_mac) == 0);
	make_syn(syn, remote, local);
	tcp_core_init(&conn);
	assert(tcp_core_listen(&conn, local, 80, iss) == 0);
	assert(tcp_core_input(&conn, remote, local, syn, sizeof(syn), 10,
			      tcp_ipv4_tx_emit, &peer) == 1);
	assert(conn.state == TCP_CORE_SYN_RECEIVED && conn.pending);
	assert(send_count == 1 && captured_len == 60);

	assert(!memcmp(captured, remote_mac, 6));
	assert(!memcmp(captured + 6, NetOurEther, 6));
	assert(get16(captured + 12) == PROT_IP);
	ip = captured + ETHER_HDR_SIZE;
	assert(ip[0] == 0x45 && ip[9] == IPPROTO_TCP);
	assert(get16(ip + 2) == IP_HDR_SIZE + 24);
	assert(!memcmp(ip + 12, local, 4));
	assert(!memcmp(ip + 16, remote, 4));
	assert(ip_checksum_valid(ip));

	tcp = ip + IP_HDR_SIZE;
	assert(tcp_segment_parse(tcp, 24, &view) == 0);
	assert(view.source_port == 80 && view.dest_port == 40000);
	assert(view.sequence == iss);
	assert(view.acknowledgment == 0x10203041);
	assert(view.flags == (TCP_FLAG_SYN | TCP_FLAG_ACK));
	assert(view.header_len == 24 && view.mss_present);
	assert(view.mss == TCP_CORE_MAX_MSS);
	assert(view.window == TCP_CORE_RX_CAPACITY);
	assert(tcp_checksum_ipv4_valid(local, remote, tcp, 24));

	/* The output frame is independent of the caller's transient SYN buffer. */
	memset(syn, 0xa5, sizeof(syn));
	assert(tcp_checksum_ipv4_valid(local, remote, tcp, 24));

	/* A failed synchronous Ethernet send must not establish SYN_RECEIVED. */
	tcp_ipv4_tx_clear(&failed_peer);
	assert(tcp_ipv4_tx_learn_peer(&failed_peer, remote, remote_mac) == 0);
	make_syn(syn, remote, local);
	tcp_core_init(&failed_conn);
	assert(tcp_core_listen(&failed_conn, local, 80, iss + 1) == 0);
	send_result = -1;
	assert(tcp_core_input(&failed_conn, remote, local, syn, sizeof(syn), 20,
			      tcp_ipv4_tx_emit, &failed_peer) == -1);
	assert(failed_conn.state == TCP_CORE_LISTEN);
	assert(!failed_conn.pending);
	tcp_ipv4_tx_clear(&failed_peer);
	assert(!failed_peer.valid);

	puts("tcp_synack_tx: PASS");
	return 0;
}
