# PA-220 P5-1D — CLOSE-WAIT foreground lifecycle repair

Date: 2026-11-21  
Base integration commit: `e429a209034d82d92caf799ce611fc3cb67447e0`  
Scope: application-owned close policy after peer FIN; no TCP core redesign.

## Root cause

The TCP core already acknowledges a valid peer FIN, advances `rcv_nxt`, sets `fin_received`, and transitions ESTABLISHED to CLOSE-WAIT. `tcp_core_close()` already emits FIN|ACK and transitions CLOSE-WAIT to LAST-ACK. The foreground `pa220 web` lifecycle did not call this primitive, leaving the connection to the 30-second idle-abort path instead of orderly close.

## Repair

- Added `pa220_web_service_close_wait()` to the foreground Web lifecycle.
- Invoke the policy after RX dispatch and from the foreground poll loop.
- The current service has no HTTP parser or response handler, so after the TCP core has processed and acknowledged peer FIN there is no application response to drain; the application then requests local close through `tcp_core_close()`.
- A failed FIN emission leaves the core in CLOSE-WAIT and is retried at 1-second intervals.
- Successful close transitions to LAST-ACK. The state gate prevents a second `tcp_core_close()` call while waiting for the peer's final ACK.
- Final ACK is handled by the existing TCP core LAST-ACK path, which clears the connection and restores LISTEN. The existing foreground loop then clears the learned peer.
- Listener reset and service cleanup clear the close retry deadline.

## Focused regression

Added `tcp_close_wait_test.c` and included it in `tests/host/tcp_ipv4_tx/run.sh`. It exercises passive open, handshake, peer FIN/ACK, CLOSE-WAIT, application close, local FIN|ACK sequence/acknowledgment, duplicate-close rejection, final ACK cleanup, and acceptance of a subsequent SYN.

## Acceptance boundaries

This source change and Host regression do not establish an OCTEON build or physical FIN close. Those results are recorded separately from the GitHub Actions run and subsequent PA-220 / Windows PktMon evidence. No hardware test has been claimed by this report.
