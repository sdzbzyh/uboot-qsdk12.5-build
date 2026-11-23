# PA-220 P5-2C initial repository and TCP lifecycle audit

UTC: 2026-11-23
Branch: feature/pa220-ubootkit-web
Audit base HEAD: b76ab17203fa59ebb35062209d52c92706b7da9b

## Repository recovery
- GitHub branch ref was read from refs/heads/feature/pa220-ubootkit-web and resolved to b76ab17203fa59ebb35062209d52c92706b7da9b.
- P5-2B archive directory reports/pa220/P5-2B/ exists and contains host logs, OCTEON build logs, ELF/undefined-symbol/object-size/stack reports, design notes, and linked object.
- P5-2B sources exist: u-boot/net/http_request.c, u-boot/net/http_handler.c, u-boot/net/http_handler_uboot.c and corresponding headers.
- No P5-2C implementation is present at this audit checkpoint.

## Actual TCP API contract
Source: u-boot/net/tcp.c and u-boot/include/net/tcp.h.
- TCP_CORE_RX_CAPACITY = 4096 bytes.
- TCP_CORE_TX_CAPACITY = 1460 bytes.
- TCP_CORE_MAX_MSS = 536 bytes (not 1460).
- The passive-open path advertises TCP_CORE_MAX_MSS.
- tcp_core_read() drains up to caller capacity from rx_data and returns the number of bytes read, or -1 for invalid arguments.
- tcp_core_write() accepts only ESTABLISHED or CLOSE_WAIT and only when no segment is pending. It returns -1 on invalid state/pending/oversized data/emission failure, 0 when peer_window is zero or smaller than the requested length, and the accepted byte count on success. The accepted segment is retained for retransmission; only one segment may be outstanding.
- tcp_core_write() limits a segment to min(peer_mss, TCP_CORE_TX_CAPACITY).
- tcp_ack_pending() clears the outstanding segment only after its sequence range is acknowledged; partial ACKs trim retry_data and advance retry_seq.
- tcp_core_close() sends FIN only from ESTABLISHED or CLOSE_WAIT and only when no segment is pending.
- The core changes ESTABLISHED to CLOSE_WAIT on peer FIN. The foreground application currently calls tcp_core_close() immediately after processing FIN.

## Lifecycle integration gap
Source: u-boot/common/cmd_pa220_web.c.
- The current foreground loop registers the TCP RX hook and polls TCP, but does not call the HTTP parser or URI handler.
- pa220_web_service_close_wait() currently sends local FIN as soon as CLOSE_WAIT is observed, without waiting for an HTTP response. P5-2C must replace this policy with response-drain-aware close handling.
- TCP RX callback currently invokes tcp_core_input(), then CLOSE-WAIT handling; it does not drain RX data into an application parser.
- Listener reset and peer identity handling already exist and should be preserved.

## P5-2C design constraints established by source
- The HTTP handler response capacity is 1460 bytes; actual peer MSS is at most 536 bytes. The application must preserve a response offset and submit no more than min(peer_mss, peer_window, TCP_CORE_TX_CAPACITY) per ACK cycle.
- A successful tcp_core_write() advances the response offset exactly by its positive return value. A return of zero or -1 must not advance it.
- No next chunk may be submitted while conn->pending is set.
- The HTTP response buffer and parser must have static bounded storage; the parser's full object is substantially larger than its 4096-byte RX array because it also embeds bounded header records.
- A peer FIN must not cause response data to be discarded. CLOSE-WAIT remains writable by tcp_core_write() and closable by tcp_core_close() after all response bytes are acknowledged.
- P5-2A and P5-2B sources must not be rewritten or reimplemented.

## Validation status
- This document is an initial source audit only.
- No P5-2C code changes, host tests, OCTEON build, ELF/ABI checks, or P5-2C Actions run have been performed at this checkpoint.
- No PA-220 physical-device validation has been performed for P5-2C.
