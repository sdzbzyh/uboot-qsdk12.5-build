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
static int captured_len, send_result;

static u16 finish_sum(u32 sum)
{
	while (sum >> 16)
		sum = (sum & 0xffff) + (sum >> 16);
	return (u16)~sum;
}
static u16 tcp_sum(const u8 src[4], const u8 dst[4],
		   const u8 *seg, u16 len)
{
	u32 sum = 0;
	u16 i;
	for (i = 0; i < 4; i += 2) {
		sum += ((u16)src[i] << 8) | src[i + 1];
		sum += ((u16)dst[i] << 8) | dst[i + 1];
	}
	sum += 6 + len;
	for (i = 0; i + 1 < len; i += 2)
		sum += ((u16)seg[i] << 8) | seg[i + 1];
	if (i < len)
		sum += (u16)seg[i] << 8;
	return finish_sum(sum);
}
int tcp_checksum_ipv4_valid(const u8 src[4], const u8 dst[4],
			    const u8 *segment, u16 segment_len)
{
	return tcp_sum(src, dst, segment, segment_len) == 0;
}
int NetSetEther(uchar *pkt, uchar *addr, uint prot)
{
	memcpy(pkt, addr, 6);
	memcpy(pkt + 6, NetOurEther, 6);
	pkt[12] = prot >> 8;
	pkt[13] = prot;
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
	if (send_result)
		return send_result;
	memcpy(captured, packet, length);
	captured_len = length;
	return 0;
}
static void put16(u8 *p, u16 v) { p[0] = v >> 8; p[1] = v; }
static void setup(void)
{
	static const u8 local[4] = { 192, 0, 2, 10 };
	static const u8 mask[4] = { 255, 255, 255, 0 };
	memcpy(&NetOurIP, local, 4);
	memcpy(&NetOurSubnetMask, mask, 4);
	captured_len = 0;
	send_result = 0;
}
static void make_segment(u8 seg[20], const u8 src[4], const u8 dst[4])
{
	memset(seg, 0, 20);
	put16(seg, 12345);
	put16(seg + 2, 80);
	seg[7] = 1;
	seg[12] = 0x50;
	seg[13] = 0x12;
	put16(seg + 14, 4096);
	put16(seg + 16, tcp_sum(src, dst, seg, 20));
}
static int ip_checksum_valid(const u8 *ip)
{
	u32 sum = 0;
	int i;
	for (i = 0; i < 20; i += 2)
		sum += ((u16)ip[i] << 8) | ip[i + 1];
	while (sum >> 16)
		sum = (sum & 0xffff) + (sum >> 16);
	return sum == 0xffff;
}
int main(void)
{
	struct tcp_ipv4_tx_peer peer;
	static const u8 remote[4] = { 192, 0, 2, 20 };
	static const u8 offlink[4] = { 192, 0, 3, 20 };
	static const u8 mac[6] = { 0x02, 0xaa, 0xbb, 0xcc, 0xdd, 0xee };
	static const u8 bad_mac[6] = { 0x01, 0xaa, 0xbb, 0xcc, 0xdd, 0xee };
	u8 seg[20], rx[20], *ip, *out_seg;
	IPaddr_t saved_mask;

	setup();
	tcp_ipv4_tx_clear(&peer);
	assert(tcp_ipv4_tx_learn_peer(&peer, remote, mac) == 0);
	assert(peer.valid && tcp_ipv4_tx_peer_matches(&peer, remote, mac));
	assert(!tcp_ipv4_tx_peer_matches(&peer, remote, bad_mac));
	assert(tcp_ipv4_tx_learn_peer(&peer, offlink, mac) != 0);
	assert(tcp_ipv4_tx_learn_peer(&peer, remote, bad_mac) != 0);

	make_segment(seg, (const u8 *)&NetOurIP, remote);
	memcpy(rx, seg, sizeof(rx));
	assert(tcp_ipv4_tx_emit(&peer, (const u8 *)&NetOurIP, remote,
				rx, sizeof(rx)) == 0);
	assert(captured_len == 60);
	assert(!memcmp(captured, mac, 6));
	assert(!memcmp(captured + 6, local_mac, 6));
	assert(captured[12] == 0x08 && captured[13] == 0x00);
	ip = captured + ETHER_HDR_SIZE;
	assert(ip[0] == 0x45 && ip[9] == IPPROTO_TCP);
	assert(ntohs(*(u16 *)(ip + 2)) == 40);
	assert(ip_checksum_valid(ip));
	out_seg = ip + IP_HDR_SIZE;
	assert(!memcmp(out_seg, seg, 20));
	assert(tcp_checksum_ipv4_valid((const u8 *)&NetOurIP, remote,
				       out_seg, 20));
	memset(rx, 0xa5, sizeof(rx));
	assert(!memcmp(out_seg, seg, 20));

	send_result = -1;
	assert(tcp_ipv4_tx_emit(&peer, (const u8 *)&NetOurIP, remote,
				seg, 20) != 0);
	send_result = 0;
	assert(tcp_ipv4_tx_emit(&peer, (const u8 *)&NetOurIP, offlink,
				seg, 20) != 0);
	seg[16] ^= 1;
	assert(tcp_ipv4_tx_emit(&peer, (const u8 *)&NetOurIP, remote,
				seg, 20) != 0);

	saved_mask = NetOurSubnetMask;
	NetOurSubnetMask = 0;
	tcp_ipv4_tx_clear(&peer);
	assert(tcp_ipv4_tx_learn_peer(&peer, remote, mac) != 0);
	NetOurSubnetMask = saved_mask;
	tcp_ipv4_tx_clear(&peer);
	{
		static const u8 broadcast[4] = { 192, 0, 2, 255 };
		assert(tcp_ipv4_tx_learn_peer(&peer, broadcast, mac) != 0);
	}
	puts("tcp_ipv4_tx: PASS");
	return 0;
}
