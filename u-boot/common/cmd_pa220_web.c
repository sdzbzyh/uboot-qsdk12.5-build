/*
 * PA-220 foreground Web/TCP lifecycle entry.
 *
 * This stage integrates the foreground service lifecycle with the bounded
 * single-peer TCP core and its native IPv4/Ethernet output adapter.
 * No HTTP parser, URI handlers, upload path or remote write operation exists.
 *
 * SPDX-License-Identifier: GPL-2.0+
 */
#include <common.h>
#include <command.h>
#include <net.h>
#include <net/tcp.h>
#include <net/tcp_ipv4_tx.h>
#include <watchdog.h>
#include <asm/global_data.h>

DECLARE_GLOBAL_DATA_PTR;

#define PA220_WEB_TCP_PORT 80

static struct tcp_core_connection pa220_web_connection;
static struct tcp_ipv4_tx_peer pa220_web_peer;

static void pa220_web_reset_listener(void)
{
	u8 local_ip[4];
	u32 next_iss = pa220_web_connection.iss + 0x10001U;

	memcpy(local_ip, pa220_web_connection.local_ip, sizeof(local_ip));
	tcp_ipv4_tx_clear(&pa220_web_peer);
	tcp_core_init(&pa220_web_connection);
	(void)tcp_core_listen(&pa220_web_connection, local_ip,
			      PA220_WEB_TCP_PORT, next_iss);
}

static void pa220_web_tcp_rx(const uchar *segment, unsigned len,
			     IPaddr_t src_ip, IPaddr_t dst_ip,
			     const uchar src_mac[6])
{
	struct tcp_segment_view view;
	u8 src[4], dst[4];

	if (!segment || len < TCP_CORE_HEADER_MIN || len > 0xffffU ||
	    !src_mac)
		return;

	memcpy(src, &src_ip, sizeof(src));
	memcpy(dst, &dst_ip, sizeof(dst));

	if (pa220_web_connection.state == TCP_CORE_LISTEN) {
		/* Learn L2 only from a valid, payload-free SYN addressed to us. */
		if (tcp_segment_parse(segment, (u16)len, &view) ||
		    !tcp_checksum_ipv4_valid(src, dst, segment, (u16)len) ||
		    view.dest_port != PA220_WEB_TCP_PORT ||
		    !view.source_port || view.flags != TCP_FLAG_SYN ||
		    view.payload_len ||
		    memcmp(dst, pa220_web_connection.local_ip, sizeof(dst)) ||
		    tcp_ipv4_tx_learn_peer(&pa220_web_peer, src, src_mac))
			return;
	} else {
		/* Ignore unrelated IPs; an L2 identity change aborts this session. */
		if (!pa220_web_peer.valid ||
		    memcmp(src, pa220_web_peer.ip, sizeof(src)))
			return;
		if (!tcp_ipv4_tx_peer_matches(&pa220_web_peer, src, src_mac)) {
			pa220_web_reset_listener();
			return;
		}
	}

	(void)tcp_core_input(&pa220_web_connection, src, dst, segment,
			     (u16)len, (u32)get_timer(0),
			     tcp_ipv4_tx_emit, &pa220_web_peer);
	if (pa220_web_connection.state == TCP_CORE_LISTEN)
		tcp_ipv4_tx_clear(&pa220_web_peer);
}

int do_pa220_web(cmd_tbl_t *cmdtp, int flag, int argc,
		 char * const argv[])
{
	struct eth_device *dev;
	static const uchar zero_mac[6] = { 0, 0, 0, 0, 0, 0 };
	static const uchar broadcast_mac[6] =
		{ 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };
	u8 local_ip[4];
	u32 initial_sequence;
	int eth_attempted = 0;
	int ret = 1;

	(void)cmdtp;
	(void)flag;

	if (argc != 2 || strcmp(argv[1], "web"))
		return CMD_RET_USAGE;

	/*
	 * Follow NetLoop's established initialization order, but keep this
	 * service in its own foreground eth_rx() loop rather than nesting a
	 * blocking NetLoop().
	 */
	net_init();
	eth_halt();
	eth_set_current();
	dev = eth_get_dev();
	if (!dev) {
		puts("pa220 web: no current Ethernet device\n");
		goto cleanup;
	}
	if (!memcmp(dev->enetaddr, zero_mac, sizeof(zero_mac)) ||
	    !memcmp(dev->enetaddr, broadcast_mac, sizeof(broadcast_mac))) {
		puts("pa220 web: current Ethernet device has no valid MAC\n");
		goto cleanup;
	}
	if (!NetOurIP) {
		puts("pa220 web: ipaddr is not configured\n");
		goto cleanup;
	}

	eth_attempted = 1;
	if (eth_init(gd->bd) < 0) {
		puts("pa220 web: Ethernet initialization failed\n");
		goto cleanup;
	}

	memcpy(local_ip, &NetOurIP, sizeof(local_ip));
	initial_sequence = (u32)get_timer(0) ^ (u32)NetOurIP;
	tcp_core_init(&pa220_web_connection);
	if (tcp_core_listen(&pa220_web_connection, local_ip,
			    PA220_WEB_TCP_PORT, initial_sequence)) {
		puts("pa220 web: TCP listener initialization failed\n");
		goto cleanup;
	}

	tcp_ipv4_tx_clear(&pa220_web_peer);
	/* Registration occurs only after all startup checks have succeeded. */
	net_set_tcp_handler(pa220_web_tcp_rx);
	puts("pa220 web: foreground lifecycle started; TCP RX hook on port 80\n");
	puts("TCP IPv4/Ethernet output enabled for one on-link peer.\n");
	puts("No HTTP parser or remote management operation is enabled.\n");
	puts("Press Ctrl-C to stop.\n");

	while (!ctrlc()) {
		WATCHDOG_RESET();
		eth_rx();
		(void)tcp_core_poll(&pa220_web_connection, (u32)get_timer(0),
				    tcp_ipv4_tx_emit, &pa220_web_peer);
		if (pa220_web_connection.state == TCP_CORE_LISTEN)
			tcp_ipv4_tx_clear(&pa220_web_peer);
	}

	puts("\npa220 web: foreground service stopped by user\n");
	ret = 0;

cleanup:
	/*
	 * Unregister before clearing the connection object so NetReceive()
	 * can never call into a reset or expired connection context.
	 */
	net_set_tcp_handler(NULL);
	tcp_ipv4_tx_clear(&pa220_web_peer);
	tcp_core_init(&pa220_web_connection);
	if (eth_attempted)
		eth_halt();
	return ret;
}
