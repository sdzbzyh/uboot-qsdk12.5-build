/*
 * PA-220 foreground HTTP/Web/TCP lifecycle entry.
 * One peer, one bounded HTTP/1.x request, read-only URI handler.
 * SPDX-License-Identifier: GPL-2.0+
 */
#include <common.h>
#include <command.h>
#include <net.h>
#include <net/tcp.h>
#include <net/tcp_ipv4_tx.h>
#include <net/http_request.h>
#include <net/http_handler.h>
#include <watchdog.h>
#include <asm/global_data.h>

DECLARE_GLOBAL_DATA_PTR;

#define PA220_WEB_TCP_PORT             80
#define PA220_WEB_CLOSE_RETRY_MS       1000U
#define PA220_WEB_WRITE_RETRY_MS       100U
#define PA220_WEB_REQUEST_TIMEOUT_MS  10000U
#define PA220_WEB_RX_CHUNK              TCP_CORE_MAX_MSS

static struct tcp_core_connection pa220_web_connection;
static struct tcp_ipv4_tx_peer pa220_web_peer;

/*
 * These objects deliberately have static storage.  In particular the parser
 * embeds its full bounded RX and header storage and must never live on stack.
 */
static struct http_request_parser pa220_web_parser;
static struct http_handler_info pa220_web_info;
static u8 pa220_web_rx[PA220_WEB_RX_CHUNK];
static u8 pa220_web_response[HTTP_HANDLER_TX_CAPACITY];
static u16 pa220_web_response_len;
static u16 pa220_web_response_off;
static u8 pa220_web_request_started;
static u8 pa220_web_response_ready;
static u8 pa220_web_response_closing;
static u32 pa220_web_request_deadline;
static u32 pa220_web_write_retry_deadline;
static u32 pa220_web_close_retry_deadline;

static void pa220_web_http_reset(void)
{
	http_request_parser_reset(&pa220_web_parser);
	memset(&pa220_web_info, 0, sizeof(pa220_web_info));
	memset(pa220_web_rx, 0, sizeof(pa220_web_rx));
	memset(pa220_web_response, 0, sizeof(pa220_web_response));
	pa220_web_response_len = 0;
	pa220_web_response_off = 0;
	pa220_web_request_started = 0;
	pa220_web_response_ready = 0;
	pa220_web_response_closing = 0;
	pa220_web_request_deadline = 0;
	pa220_web_write_retry_deadline = 0;
	pa220_web_close_retry_deadline = 0;
}

static void pa220_web_reset_listener(void)
{
	u8 local_ip[4];
	u32 next_iss = pa220_web_connection.iss + 0x10001U;

	memcpy(local_ip, pa220_web_connection.local_ip, sizeof(local_ip));
	tcp_ipv4_tx_clear(&pa220_web_peer);
	pa220_web_http_reset();
	tcp_core_init(&pa220_web_connection);
	(void)tcp_core_listen(&pa220_web_connection, local_ip,
			      PA220_WEB_TCP_PORT, next_iss);
}

static int pa220_web_build_error_response(void)
{
	/* NULL request is intentionally mapped by the existing handler to 400. */
	if (http_handler_build(NULL, NULL, pa220_web_response,
			       sizeof(pa220_web_response),
			       &pa220_web_response_len))
		return -1;
	pa220_web_response_off = 0;
	pa220_web_response_ready = 1;
	return 0;
}

static int pa220_web_build_request_response(void)
{
	if (http_handler_get_device_info(&pa220_web_info))
		memset(&pa220_web_info, 0, sizeof(pa220_web_info));

	if (http_handler_build(&pa220_web_parser.request, &pa220_web_info,
			       pa220_web_response, sizeof(pa220_web_response),
			       &pa220_web_response_len))
		return pa220_web_build_error_response();

	pa220_web_response_off = 0;
	pa220_web_response_ready = 1;
	return 0;
}

/*
 * Drain TCP RX before examining CLOSE-WAIT.  tcp_core_input() has already
 * copied payload from a FIN-bearing segment into rx_data before changing the
 * state, so this also preserves the final request bytes.
 */
static void pa220_web_service_http_rx(u32 now)
{
	int count;
	enum http_req_status status;

	if (pa220_web_response_ready ||
	    (pa220_web_connection.state != TCP_CORE_ESTABLISHED &&
	     pa220_web_connection.state != TCP_CORE_CLOSE_WAIT))
		return;

	for (;;) {
		count = tcp_core_read(&pa220_web_connection, pa220_web_rx,
				      sizeof(pa220_web_rx));
		if (count <= 0)
			break;

		if (!pa220_web_request_started) {
			pa220_web_request_started = 1;
			pa220_web_request_deadline =
				now + PA220_WEB_REQUEST_TIMEOUT_MS;
		}

		status = http_request_parser_feed(&pa220_web_parser,
						  pa220_web_rx, (u16)count);
		if (status == HTTP_REQ_COMPLETE) {
			(void)pa220_web_build_request_response();
			return;
		}
		if (status == HTTP_REQ_ERROR) {
			(void)pa220_web_build_error_response();
			return;
		}
	}

	/* FIN before the terminating CRLFCRLF: answer deterministically with 400. */
	if (pa220_web_connection.state == TCP_CORE_CLOSE_WAIT &&
	    !pa220_web_response_ready &&
	    pa220_web_request_started)
		(void)pa220_web_build_error_response();
}

static int pa220_web_time_reached(u32 now, u32 deadline)
{
	return (s32)(now - deadline) >= 0;
}

static void pa220_web_service_request_timeout(u32 now)
{
	if (!pa220_web_response_ready && pa220_web_request_started &&
	    pa220_web_request_deadline &&
	    pa220_web_time_reached(now, pa220_web_request_deadline))
		(void)pa220_web_build_error_response();
}

/*
 * Submit exactly one segment at a time.  tcp_core_write() owns the pending
 * segment until its complete ACK arrives.  The response offset advances only
 * by the positive byte count actually accepted by the TCP core.
 */
static void pa220_web_service_tx(u32 now)
{
	u16 remaining, limit;
	int accepted;

	if (!pa220_web_response_ready || pa220_web_connection.pending)
		return;
	if (pa220_web_connection.state != TCP_CORE_ESTABLISHED &&
	    pa220_web_connection.state != TCP_CORE_CLOSE_WAIT)
		return;
	if (pa220_web_write_retry_deadline &&
	    !pa220_web_time_reached(now, pa220_web_write_retry_deadline))
		return;

	if (pa220_web_response_off < pa220_web_response_len) {
		remaining = pa220_web_response_len - pa220_web_response_off;
		limit = pa220_web_connection.peer_mss;
		if (!limit || limit > TCP_CORE_MAX_MSS)
			limit = TCP_CORE_MAX_MSS;
		if (limit > TCP_CORE_TX_CAPACITY)
			limit = TCP_CORE_TX_CAPACITY;
		if (limit > pa220_web_connection.peer_window)
			limit = pa220_web_connection.peer_window;
		if (!limit)
			return; /* Zero window: preserve the exact response offset. */
		if (remaining > limit)
			remaining = limit;

		accepted = tcp_core_write(&pa220_web_connection,
					  pa220_web_response +
					  pa220_web_response_off,
					  remaining, now, tcp_ipv4_tx_emit,
					  &pa220_web_peer);
		if (accepted > 0) {
			if (accepted > remaining) {
				/* Defensive invariant: never trust an impossible result. */
				pa220_web_response_ready = 0;
				return;
			}
			pa220_web_response_off += (u16)accepted;
			pa220_web_write_retry_deadline = 0;
		} else if (accepted < 0) {
			pa220_web_write_retry_deadline =
				now + PA220_WEB_WRITE_RETRY_MS;
		}
		return;
	}

	/* All response bytes have been ACKed: only now may we send FIN. */
	if (!pa220_web_connection.pending) {
		if (!tcp_core_close(&pa220_web_connection, now,
				    tcp_ipv4_tx_emit, &pa220_web_peer)) {
			pa220_web_response_closing = 1;
			pa220_web_close_retry_deadline = 0;
		} else {
			pa220_web_close_retry_deadline =
				now + PA220_WEB_CLOSE_RETRY_MS;
		}
	}
}

static void pa220_web_service(u32 now)
{
	if (pa220_web_connection.state == TCP_CORE_LISTEN) {
		if (pa220_web_peer.valid)
			tcp_ipv4_tx_clear(&pa220_web_peer);
		pa220_web_http_reset();
		return;
	}

	pa220_web_service_http_rx(now);
	pa220_web_service_request_timeout(now);

	if (pa220_web_response_ready) {
		pa220_web_service_tx(now);
		return;
	}

	/*
	 * A peer may close without sending any request bytes.  Do not leave a
	 * half-open application session behind; reply with a bounded 400.
	 */
	if (pa220_web_connection.state == TCP_CORE_CLOSE_WAIT)
		(void)pa220_web_build_error_response();

	(void)pa220_web_close_retry_deadline;
	(void)pa220_web_response_closing;
}

static void pa220_web_tcp_rx(const uchar *segment, unsigned len,
			     IPaddr_t src_ip, IPaddr_t dst_ip,
			     const uchar src_mac[6])
{
	struct tcp_segment_view view;
	u8 src[4], dst[4];
	u32 now = (u32)get_timer(0);

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
		if (!pa220_web_peer.valid ||
		    memcmp(src, pa220_web_peer.ip, sizeof(src)))
			return;
		if (!tcp_ipv4_tx_peer_matches(&pa220_web_peer, src, src_mac)) {
			pa220_web_reset_listener();
			return;
		}
	}

	(void)tcp_core_input(&pa220_web_connection, src, dst, segment,
			     (u16)len, now, tcp_ipv4_tx_emit,
			     &pa220_web_peer);

	/* Process buffered payload after TCP has recorded any FIN in this segment. */
	pa220_web_service(now);
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
	pa220_web_http_reset();
	net_set_tcp_handler(pa220_web_tcp_rx);
	puts("pa220 web: bounded read-only HTTP service on TCP port 80\n");
	puts("One request per connection; GET/HEAD; five read-only URIs.\n");
	puts("Keep-Alive, pipelining and concurrent peers are disabled.\n");
	puts("Press Ctrl-C to stop.\n");

	while (!ctrlc()) {
		u32 now;

		WATCHDOG_RESET();
		eth_rx();
		now = (u32)get_timer(0);
		pa220_web_service(now);
		(void)tcp_core_poll(&pa220_web_connection, now,
				    tcp_ipv4_tx_emit, &pa220_web_peer);
		pa220_web_service(now);
	}

	puts("\npa220 web: foreground service stopped by user\n");
	ret = 0;

cleanup:
	net_set_tcp_handler(NULL);
	pa220_web_http_reset();
	tcp_ipv4_tx_clear(&pa220_web_peer);
	tcp_core_init(&pa220_web_connection);
	if (eth_attempted)
		eth_halt();
	return ret;
}
