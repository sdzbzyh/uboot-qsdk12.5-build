/*
 * PA-220 single-peer IPv4/TCP Ethernet transmitter.
 * SPDX-License-Identifier: GPL-2.0+
 */
#ifndef __PA220_NET_TCP_IPV4_TX_H__
#define __PA220_NET_TCP_IPV4_TX_H__
#include <common.h>
struct tcp_ipv4_tx_peer {
	u8 ip[4];
	u8 mac[6];
	u8 valid;
};
void tcp_ipv4_tx_clear(struct tcp_ipv4_tx_peer *peer);
int tcp_ipv4_tx_learn_peer(struct tcp_ipv4_tx_peer *peer,
			   const u8 src_ip[4], const u8 src_mac[6]);
int tcp_ipv4_tx_peer_matches(const struct tcp_ipv4_tx_peer *peer,
			     const u8 src_ip[4], const u8 src_mac[6]);
int tcp_ipv4_tx_emit(void *arg, const u8 src[4], const u8 dst[4],
		     const u8 *segment, u16 segment_len);
#endif
