#ifndef TEST_TCP_H
#define TEST_TCP_H
#include <common.h>
#define TCP_CORE_HEADER_MIN 20
#define TCP_CORE_HEADER_MAX 60
#define TCP_CORE_TX_CAPACITY 1460
int tcp_checksum_ipv4_valid(const u8 src[4], const u8 dst[4],
			    const u8 *segment, u16 segment_len);
#endif
