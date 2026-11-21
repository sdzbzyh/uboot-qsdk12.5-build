# P5-1D Physical FIN Close Validation

Date: 2026-11-21  
Repository: `sdzbzyh/uboot-qsdk12.5-build`  
Branch: `feature/pa220-ubootkit-web`  
Reviewed branch HEAD: `0346a8310e1db8ada0fa2301a888fcbc307bca9a`  
Parent: `567474c203810bac4afdc941c6a30ccb0d57be1f`

## Scope and safety

This was one Windows-initiated TCP connection to the already-running PA-220 foreground `pa220 web` service. No PA-220 reboot, reset, power cycle, service restart, rebuild, network reconfiguration, or persistent-storage write was performed. The previous successful handshake and 10-byte payload test was not repeated as a separate test; this capture necessarily contains the handshake for the single FIN test connection.

## Evidence files

Uploaded by the user under `PA220-Test/FIN-20261121-144743/`:

- `fin-capture.pcapng` — 270,704 bytes; blob SHA `3906505fd09288d88867db12bec8a7a2d24b3ec6`
- `fin-capture.etl` — 130,809 bytes; blob SHA `28143e79fd791f07bbc06caeac243024fe5bac18`
- `fin-test.log` — 3,478 bytes; blob SHA `2a84d274f174309547571c8e6d49e84e5a9cdd37`

The PowerShell log records a successful capture start, TCP connect, client Close(), capture stop, and ETL-to-PCAPNG conversion. PktMon reported 458 packets, zero dropped packets. The test endpoint was Windows `192.168.2.10:7050` and PA-220 `192.168.2.2:80`.

## Packet analysis

The PCAPNG was decoded and the Ethernet/IPv4/TCP flow was filtered by the exact four-tuple. Five packets for this connection were present:

| Order | Direction | Flags | Sequence | Acknowledgment |
|---:|---|---|---:|---:|
| 1 | Windows -> PA-220 | SYN | 1772778290 | 0 |
| 2 | PA-220 -> Windows | SYN, ACK | 3232238603 | 1772778291 |
| 3 | Windows -> PA-220 | ACK | 1772778291 | 3232238604 |
| 4 | Windows -> PA-220 | FIN, ACK | 1772778291 | 3232238604 |
| 5 | PA-220 -> Windows | ACK | 3232238604 | 1772778292 |

Findings:

1. The client FIN consumes one sequence number: client sequence `1772778291` becomes next expected sequence `1772778292`.
2. PA-220's ACK acknowledges exactly `1772778292`. The packet-level evidence therefore confirms that PA-220 received and correctly acknowledged the peer FIN.
3. No PA-220 FIN was observed in the capture, and consequently no final Windows ACK of a PA-220 FIN was observed.
4. Thus, the four-way orderly close was **not** completed in the observed capture. The observed wire behavior is the peer half-close acknowledged by PA-220.

## Source review at the captured branch HEAD

Reviewed:

- `u-boot/net/tcp.c`
- `u-boot/common/cmd_pa220_web.c`
- `u-boot/include/net/tcp.h`

The TCP core's FIN input path increments `rcv_nxt`, sets `fin_received`, transitions `TCP_CORE_ESTABLISHED` to `TCP_CORE_CLOSE_WAIT`, and emits an ACK. This matches the observed packet sequence.

The TCP core also implements `tcp_core_close()`: when called in `TCP_CORE_CLOSE_WAIT`, it sends `FIN|ACK` and transitions to `TCP_CORE_LAST_ACK`. However, the current foreground `cmd_pa220_web.c` RX callback does not call `tcp_core_close()` after receiving the peer FIN, and the foreground loop only calls `tcp_core_poll()`. Therefore the available close primitive is not integrated into this service lifecycle.

The current idle timeout is 30,000 ms. If the connection remains idle in CLOSE-WAIT, `tcp_core_poll()` eventually calls `tcp_abort_connection()`, which resets the single connection context to LISTEN (because the local port is retained); the foreground loop then clears the learned peer. This is timeout-based cleanup, not a graceful FIN/ACK close. The present 8-second capture does not prove that this timeout cleanup occurred on the physical device.

## Result

- **Peer FIN reception and ACK:** PASS, directly evidenced on the wire.
- **PA-220-initiated FIN and final ACK:** NOT OBSERVED; orderly four-way close is incomplete.
- **Application-driven CLOSE-WAIT -> LAST-ACK integration:** GAP confirmed by source review; `tcp_core_close()` exists but is not called by the foreground Web lifecycle.
- **Physical connection cleanup after idle timeout:** NOT VERIFIED by this capture. Source review indicates a 30-second idle-timeout path returns the single connection object to LISTEN and the foreground loop clears its peer, but no physical post-timeout observation was collected.

No further device action was taken. The running `pa220 web` foreground service should remain untouched. The next implementation task is to integrate a deliberate application close policy for a received FIN and then validate the resulting FIN/ACK sequence and listener recovery in a separately approved test window. Do not claim P5-1D FIN orderly-close validation as passed.
