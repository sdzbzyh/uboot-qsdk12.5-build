/*
 * PA-220 bounded read-only HTTP URI handler.
 * SPDX-License-Identifier: GPL-2.0+
 */
#ifndef __PA220_HTTP_HANDLER_H__
#define __PA220_HTTP_HANDLER_H__
#include <linux/types.h>
#include <net/http_request.h>
#define HTTP_HANDLER_TX_CAPACITY 1460
#define HTTP_HANDLER_BODY_OFFSET 192
#define HTTP_HANDLER_TEXT_MAX 64
#define HTTP_INFO_MEMORY_VALID 0x01
#define HTTP_INFO_ETH_PRESENT 0x02
#define HTTP_INFO_MAC_VALID 0x04
#define HTTP_INFO_IP_VALID 0x08
#define HTTP_INFO_ETH_INIT 0
#define HTTP_INFO_ETH_PASSIVE 1
#define HTTP_INFO_ETH_ACTIVE 2
struct http_handler_info {
 char board_name[HTTP_HANDLER_TEXT_MAX];
 char cpu_name[HTTP_HANDLER_TEXT_MAX];
 char interface_name[16];
 u64 memory_start;
 u64 memory_total_bytes;
 u32 board_type;
 u32 cpu_clock_hz;
 u32 uptime_ms;
 u8 mac[6];
 u8 ip[4];
 u8 flags;
 u8 eth_state;
};
int http_handler_get_device_info(struct http_handler_info *info);
int http_handler_build(const struct http_request *request,
 const struct http_handler_info *info, u8 *output,
 u16 output_capacity, u16 *output_length);
#endif
