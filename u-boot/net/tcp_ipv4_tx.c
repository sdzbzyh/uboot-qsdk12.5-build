/*
 * PA-220 bounded single-peer IPv4/TCP Ethernet output.
 *
 * Peer MAC is learned only from a validated inbound SYN after confirming
 * that the source IPv4 address is on the configured local subnet. This
 * deliberately avoids the legacy global ARP pending slot and NetTxPacket.
 *
 * eth_send() is called directly because NetSendPacket() discards its return
 * value. The PA-220 OCTEON driver copies the frame into an FPA buffer before
 * returning, so this private aligned frame is not retained by the driver.
 *
 * SPDX-License-Identifier: GPL-2.0+
 */
#include <common.h>
#include <net.h>
#include <net/tcp.h>
#include <net/tcp_ipv4_tx.h>

#define TCP_IPV4_TX_FRAME_CAPACITY	1600
#define TCP_IPV4_TX_ETH_MIN_SIZE	60
#define TCP_IPV4_TX_MTU		1500

static u8 tcp_ipv4_tx_frame[TCP_IPV4_TX_FRAME_CAPACITY] __aligned(PKTALIGN);

static int tcp_ipv4_tx_mac_valid(const u8 mac[6])
{
	u8 any = 0;
	u8 all_ff = 0xff;
	int i;

	if (!mac || (mac[0] & 1))
		return 0;
	for (i = 0; i < 6; i++) {
		any |= mac[i];
		all_ff &= mac[i];
	}
	return any != 0 && all_ff != 0xff;
}

static int tcp_ipv4_tx_mask_prefix(const u8 mask[4])
{
	int prefix = 0;
	int zero_seen = 0;
	int i, bit;

	for (i = 0; i < 4; i++) {
		for (bit = 7; bit >= 0; bit--) {
			if (mask[i] & (1U << bit)) {
				if (zero_seen)
					return -1;
				prefix++;
			} else {
				zero_seen = 1;
			}
		}
	}
	if (prefix == 0 || prefix == 32)
		return -1;
	return prefix;
}

static int tcp_ipv4_tx_ip_valid(const u8 ip[4])
{
	if (!ip || ip[0] == 0 || ip[0] == 127 || ip[0] >= 224)
		return 0;
	return 1;
}

void tcp_ipv4_tx_clear(struct tcp_ipv4_tx_peer *peer)
{
	if (peer)
		memset(peer, 0, sizeof(*peer));
}

int tcp_ipv4_tx_learn_peer(struct tcp_ipv4_tx_peer *peer,
			   const u8 src_ip[4], const u8 src_mac[6])
{
	u8 local[4], mask[4], host[4];
	int prefix;
	int all_host_zero = 1, all_host_one = 1;
	int i;

	if (!peer || !src_ip || !src_mac || !NetOurIP ||
	    !NetOurSubnetMask || !tcp_ipv4_tx_mac_valid(src_mac) ||
	    !tcp_ipv4_tx_ip_valid(src_ip))
		return -1;

	memcpy(local, &NetOurIP, sizeof(local));
	memcpy(mask, &NetOurSubnetMask, sizeof(mask));
	prefix = tcp_ipv4_tx_mask_prefix(mask);
	if (prefix < 0 || !memcmp(local, src_ip, sizeof(local)))
		return -1;

	for (i = 0; i < 4; i++) {
		if ((local[i] & mask[i]) != (src_ip[i] & mask[i]))
			return -1;
		host[i] = (u8)(src_ip[i] & (u8)~mask[i]);
		if (host[i] != 0)
			all_host_zero = 0;
		if (host[i] != (u8)~mask[i])
			all_host_one = 0;
	}
	if (prefix <= 30 && (all_host_zero || all_host_one))
		return -1;

	if (peer->valid)
		return tcp_ipv4_tx_peer_matches(peer, src_ip, src_mac) ? 0 : -1;

	memcpy(peer->ip, src_ip, sizeof(peer->ip));
	memcpy(peer->mac, src_mac, sizeof(peer->mac));
	peer->valid = 1;
	return 0;
}

int tcp_ipv4_tx_peer_matches(const struct tcp_ipv4_tx_peer *peer,
			     const u8 src_ip[4], const u8 src_mac[6])
{
	return peer && peer->valid && src_ip && src_mac &&
	       !memcmp(peer->ip, src_ip, sizeof(peer->ip)) &&
	       !memcmp(peer->mac, src_mac, sizeof(peer->mac));
}

int tcp_ipv4_tx_emit(void *arg, const u8 src[4], const u8 dst[4],
		     const u8 *segment, u16 segment_len)
{
	struct tcp_ipv4_tx_peer *peer = arg;
	struct ip_udp_hdr *ip;
	IPaddr_t src_addr, dst_addr;
	int eth_len, ip_len, frame_len, send_len;

	if (!peer || !peer->valid || !src || !dst || !segment ||
	    segment_len < TCP_CORE_HEADER_MIN)
		return -1;

	memcpy(&src_addr, src, sizeof(src_addr));
	memcpy(&dst_addr, dst, sizeof(dst_addr));
	if (src_addr != NetOurIP || memcmp(dst, peer->ip, 4))
		return -1;

	ip_len = IP_HDR_SIZE + segment_len;
	if (ip_len > TCP_IPV4_TX_MTU ||
	    !tcp_checksum_ipv4_valid(src, dst, segment, segment_len))
		return -1;

	eth_len = NetSetEther(tcp_ipv4_tx_frame, peer->mac, PROT_IP);
	if (eth_len != ETHER_HDR_SIZE && eth_len != VLAN_ETHER_HDR_SIZE)
		return -1;
	if (eth_len + ip_len > (int)sizeof(tcp_ipv4_tx_frame))
		return -1;

	ip = (struct ip_udp_hdr *)(tcp_ipv4_tx_frame + eth_len);
	net_set_ip_header((uchar *)ip, dst_addr, src_addr);
	ip->ip_len = htons(ip_len);
	ip->ip_p = IPPROTO_TCP;
	ip->ip_sum = 0;
	ip->ip_sum = (ushort)~NetCksum((uchar *)ip, IP_HDR_SIZE / 2);
	memcpy((u8 *)ip + IP_HDR_SIZE, segment, segment_len);

	frame_len = eth_len + ip_len;
	send_len = frame_len < TCP_IPV4_TX_ETH_MIN_SIZE ?
		   TCP_IPV4_TX_ETH_MIN_SIZE : frame_len;
	if (send_len > frame_len)
		memset(tcp_ipv4_tx_frame + frame_len, 0, send_len - frame_len);

	/* The native OCTEON eth_send() success result is exactly zero. */
	return eth_send(tcp_ipv4_tx_frame, send_len) == 0 ? 0 : -1;
}
