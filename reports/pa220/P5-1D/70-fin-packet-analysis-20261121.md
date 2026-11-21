# PA-220 P5-1D — CLOSE-WAIT FIN packet-level physical validation

Date: 2026-11-21  
Repository: `sdzbzyh/uboot-qsdk12.5-build`  
Branch: `feature/pa220-ubootkit-web`  
CLOSE-WAIT repair commit: `3d0a867`  
Deployed image: `u-boot-octeon_pa220.bin`  
Image size: 1,370,376 bytes (`0x14e908`)  
Image SHA-256: `2318987d4d3a4ea3817809cb3bf00ab4297009b78f153adfcadc42426318d57d`

## Scope and safety

This report records packet-level validation of the already-running RAM-deployed PA-220 `pa220 web` foreground service. No rebuild, re-download, TFTP deployment, reboot, service restart, persistent flash/eMMC write, or environment save was performed during this validation.

## Capture and evidence

User-uploaded evidence directory:

`PA220-Test/PA220-FIN-20261121-153150/`

- Original PktMon ETL: 85,717,742 bytes (local capture; not uploaded because of size).
- Converted PCAPNG: 94,891,324 bytes (local source capture; not uploaded because of size).
- PktMon conversion reported 121,752 packets and **0 dropped packets**.
- Wireshark/TShark 4.6.10 filtered the existing PCAPNG; no new capture was made.
- Filter: `ip.addr == 192.168.2.10 && ip.addr == 192.168.2.2 && tcp.port == 80`.
- Filtered packet count: **21** across three TCP streams.
- Filtered PCAPNG: 2,028 bytes.
- Text packet fields: `fin-packets.txt`.
- Verbose packet decode: `fin-tcp-details.txt`.
- Client test log: `fin-client-test-retry.txt`.
- Conversion/filter report: `fin-filter-report.txt`.

## Packet-level results

TCP flag values in the field export:
- `0x0002`: SYN
- `0x0012`: SYN, ACK
- `0x0010`: ACK
- `0x0011`: FIN, ACK

### Stream 0 — earlier test connection, client port 8111

Frames 50174–50180 show a complete handshake and orderly four-packet close:

| Frame | Direction | Flags | Relative Seq | Relative Ack |
|---:|---|---|---:|---:|
| 50174 | 192.168.2.10:8111 -> 192.168.2.2:80 | SYN | 0 | 0 |
| 50175 | 192.168.2.2:80 -> 192.168.2.10:8111 | SYN, ACK | 0 | 1 |
| 50176 | 192.168.2.10:8111 -> 192.168.2.2:80 | ACK | 1 | 1 |
| 50177 | 192.168.2.10:8111 -> 192.168.2.2:80 | FIN, ACK | 1 | 1 |
| 50178 | 192.168.2.2:80 -> 192.168.2.10:8111 | ACK | 1 | 2 |
| 50179 | 192.168.2.2:80 -> 192.168.2.10:8111 | FIN, ACK | 1 | 2 |
| 50180 | 192.168.2.10:8111 -> 192.168.2.2:80 | ACK | 2 | 2 |

The first client-script attempt had a client-side socket exception after FIN submission, so its application log is not used as the primary acceptance evidence. The packet capture nevertheless records the complete FIN exchange on this stream.

### Stream 1 — explicit FIN test, client port 8140

Frames 86948–86954 show:

| Frame | Direction | Flags | Relative Seq | Relative Ack |
|---:|---|---|---:|---:|
| 86948 | 192.168.2.10:8140 -> 192.168.2.2:80 | SYN | 0 | 0 |
| 86949 | 192.168.2.2:80 -> 192.168.2.10:8140 | SYN, ACK | 0 | 1 |
| 86950 | 192.168.2.10:8140 -> 192.168.2.2:80 | ACK | 1 | 1 |
| 86951 | 192.168.2.10:8140 -> 192.168.2.2:80 | FIN, ACK | 1 | 1 |
| 86952 | 192.168.2.2:80 -> 192.168.2.10:8140 | ACK | 1 | 2 |
| 86953 | 192.168.2.2:80 -> 192.168.2.10:8140 | FIN, ACK | 1 | 2 |
| 86954 | 192.168.2.10:8140 -> 192.168.2.2:80 | ACK | 2 | 2 |

This directly proves the required sequence: client FIN; PA-220 ACK; PA-220 FIN|ACK; client final ACK. The PA-220 FIN acknowledges the client's FIN (Ack 2), and the client's final ACK acknowledges the PA-220 FIN (Ack 2).

The corresponding client log records:
- `CLIENT_FIN_SENT`
- `SERVER_EOF: Receive() returned 0`
- `FIRST_RESULT eof=True received_bytes=0`

### Stream 2 — subsequent connection, client port 8141

Frames 86955–86961 show a successful subsequent three-way handshake followed by a complete orderly close:

| Frame | Direction | Flags | Relative Seq | Relative Ack |
|---:|---|---|---:|---:|
| 86955 | 192.168.2.10:8141 -> 192.168.2.2:80 | SYN | 0 | 0 |
| 86956 | 192.168.2.2:80 -> 192.168.2.10:8141 | SYN, ACK | 0 | 1 |
| 86957 | 192.168.2.10:8141 -> 192.168.2.2:80 | ACK | 1 | 1 |
| 86958 | 192.168.2.10:8141 -> 192.168.2.2:80 | FIN, ACK | 1 | 1 |
| 86959 | 192.168.2.2:80 -> 192.168.2.10:8141 | ACK | 1 | 2 |
| 86960 | 192.168.2.2:80 -> 192.168.2.10:8141 | FIN, ACK | 1 | 2 |
| 86961 | 192.168.2.10:8141 -> 192.168.2.2:80 | ACK | 2 | 2 |

The client log independently records `SECOND_CONNECT_SUCCESS` for local port 8141. This demonstrates that the service accepted a new connection after the previous connection's close.

## Findings

1. **Peer FIN reception and acknowledgment: PASS.** The PA-220 emits ACK with Ack 2 after the peer's FIN.
2. **Application-driven local FIN: PASS.** The PA-220 emits FIN|ACK from server port 80 after the peer FIN.
3. **Peer final ACK: PASS.** The client emits the final ACK with Ack 2 after the PA-220 FIN.
4. **Orderly close: PASS.** The complete FIN/ACK exchange is present on both the 8140 and 8141 streams; it is also present on the earlier 8111 stream.
5. **Listener recovery / subsequent accept: PASS.** A new connection on port 8141 completes its TCP handshake after the 8140 close; client log records `SECOND_CONNECT_SUCCESS`.
6. **Reset observation: PASS.** None of the 21 filtered packets has the TCP RST flag.
7. **Capture integrity: PASS.** PktMon reported zero dropped packets for the 121,752-packet capture.

## Acceptance

**P5-1D CLOSE-WAIT FIN orderly-close physical validation: PASS.**

This result is based on actual Ethernet/IPv4/TCP packets captured from the physical PA-220 test, not solely on source inspection or UART messages. The client EOF and second-connect log, PA-220 CLOSE-WAIT UART messages, and packet-level FIN/ACK sequence are mutually consistent.

The earlier `40-fin-close-physical-validation.md` describes a separate earlier capture in which no local FIN was observed. This report records the later repaired-image validation and supersedes that earlier incomplete result for the repaired build.
