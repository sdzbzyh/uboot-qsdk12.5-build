#ifndef TEST_NET_H
#define TEST_NET_H
#include <common.h>
#define PROT_IP 0x0800
#define IP_HDR_SIZE 20
#define ETHER_HDR_SIZE 14
#define VLAN_ETHER_HDR_SIZE 18
struct ip_udp_hdr {
	u8 ip_hl_v, ip_tos;
	u16 ip_len, ip_id, ip_off;
	u8 ip_ttl, ip_p;
	u16 ip_sum;
	IPaddr_t ip_src, ip_dst;
	u16 udp_src, udp_dst, udp_len, udp_xsum;
};
extern IPaddr_t NetOurIP;
extern IPaddr_t NetOurSubnetMask;
int NetSetEther(uchar *pkt, uchar *addr, uint prot);
void net_set_ip_header(uchar *pkt, IPaddr_t dest, IPaddr_t source);
uint NetCksum(uchar *ptr, int len);
int eth_send(void *packet, int length);
#endif
