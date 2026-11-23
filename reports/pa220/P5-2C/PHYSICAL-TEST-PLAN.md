# PA-220 P5-2C physical HTTP/TCP validation procedure

Date: 2026-11-23
Branch: feature/pa220-ubootkit-web
Cloud checkpoint: 384366d22223e3169f355476ca1eb7673612725c

## Evidence boundary

- P5-2C Host lifecycle: PASS (29 scenarios / 161 assertions).
- OCTEON N32 relocatable link and complete PA-220 U-Boot build: PASS.
- P5-1D physical Ethernet/TCP handshake and 10-byte payload ACK: PASS, 2026-11-21.
- P5-2C physical HTTP 200 lifecycle: NOT YET VERIFIED.
- The Host harness uses a deterministic Ethernet boundary and is not physical-device evidence.

## Known physical network from P5-1D evidence

| Item | Observed value |
|---|---|
| PA-220 IPv4 | 192.168.2.2/24 |
| Windows peer IPv4 | 192.168.2.10/24 |
| TCP port | 80 |
| U-Boot active Ethernet interface | octrgmii0 |
| Link | 1000 Mbps full duplex |
| Client MAC observed | 3C-97-0E-D4-E1-0E |
| PA-220 Ethernet source MAC observed | 08-30-6B-E1-BE-08 |

These are historical observed values. Reconfirm them from the current console and link before use.

## Critical safety gate

The P5-1D physical report records an already launched RAM-resident test image and explicitly prohibits reboot, reset, power-cycle, another go, CPLD writes, saveenv, SPI erase/write/update, eMMC/NAND writes, and restarting pa220 web without first confirming the prior foreground service has exited and obtaining explicit approval.

The P5-2C BIN is a Cavium OCTEON PA-220 Stage-3 image, not a generic firmware-upgrade payload. It has a Cavium BOOT header and was built for the existing PA-220 stage-2 startup chain. Do not flash it. Do not launch it while the existing session state is unknown. A complete backup and recovery image/console path must be independently verified before any future boot-path change.

## Phase 0 — non-invasive current-state confirmation

1. Do not interrupt an active serial console or foreground command.
2. Capture the current serial terminal output and timestamp.
3. Determine whether pa220 web is still running. Do not send Ctrl-C merely to test it.
4. If the console is at a U-Boot prompt, record read-only output only:
   version
   printenv ipaddr netmask ethact ethaddr
5. Do not use saveenv, cpld write, sf erase/write/update, mmc write, reset, or boot commands.
6. Record link LEDs and physical cable path. Historical interface octrgmii0 must be reconfirmed.

Stop if the prior service is active or board state is uncertain.

## Phase 1 — host-side capture

On Windows, identify the correct wired NIC:

    tshark -D

Start capture on the PA-220-connected Ethernet NIC before any client request:

    tshark -i <N> -f "host 192.168.2.2 and tcp port 80" -w .\pa220-p5-2c-http.pcapng

Replace <N> with the interface number reported by tshark -D. Do not capture on Wi-Fi or another adapter.

In a separate PowerShell window, use the already validated host address 192.168.2.10/24 only if it is still configured and the isolated link is confirmed. Do not add a gateway or DNS server for this test.

## Phase 2 — HTTP/TCP client tests

Only after Phase 0 confirms a safe prompt, the user explicitly authorizes starting the P5-2C service, and the exact P5-2C image is known to be running:

    .\scripts\Test-Pa220Web.ps1 -Target 192.168.2.2 -Port 80 -TimeoutMs 10000 -LogPath .\pa220-web-test.log

The script does not configure, boot, reset, or flash the board. It expects the foreground service to be running already.

Coverage:
- GET /api/board, expect HTTP 200 and PA-220 JSON.
- GET /, /api/status, /api/memory, /api/network, expect HTTP 200.
- GET unknown URI, expect HTTP 404.
- POST /api/status, expect HTTP 405.
- Each complete request uses a fresh connection and client FIN; the client reads until server EOF and validates Content-Length.
- One connection is aborted with RST, then a fresh GET verifies listener recovery.

The server supports one request per connection. Do not test keep-alive, pipelining, or concurrent clients.

## Phase 3 — packet analysis

Stop capture with Ctrl-C after the client script completes. Export packet fields:

    .\scripts\Analyze-Pa220WebCapture.ps1 -PcapPath .\pa220-p5-2c-http.pcapng -Target 192.168.2.2 -Port 80 -CsvPath .\pa220-web-packets.csv

Wireshark display filter:

    ip.addr == 192.168.2.2 && tcp.port == 80

Inspect each successful TCP stream:
1. SYN, SYN-ACK, ACK.
2. Full HTTP request payload from client.
3. HTTP response status line and complete response payload.
4. ACKs covering all response bytes.
5. Server FIN only after all response bytes are acknowledged.
6. Client final ACK.
7. Connection state returns to listener; next independent request succeeds.
8. RST test does not prevent a later valid request.

Save raw PCAPNG, CSV export, client log, complete serial log, image SHA-256, and exact environment values. Do not publish PASS based on a client-only log.

## Acceptance criteria

| Test | PASS criterion |
|---|---|
| Ethernet | Link active; expected interface/MAC/IP documented |
| TCP handshake | Physical capture contains SYN/SYN-ACK/ACK |
| HTTP parser/handler | Correct 200/404/405 responses and exact bodies |
| Full response | Content-Length equals received body bytes; EOF follows |
| Orderly close | Client FIN, response ACK progression, server FIN, final ACK |
| Repeated requests | Each new connection receives a complete valid response |
| RST recovery | A later valid request succeeds after abort |
| Cleanup | No stale connection blocks subsequent requests |
| Evidence | Matching serial log, PCAPNG, CSV, client log, image hash |

Missing criteria remain FAIL or NOT TESTED. Host and cross-build results cannot substitute for physical evidence.

## Additional historical physical FIN evidence (P5-1D follow-up)

The repository also contains a later Windows capture/test set at PA220-Test/PA220-FIN-20261121-153150. The packet field export records two empty-payload TCP connections to 192.168.2.2:80:
- Stream 1 (client port 8140): SYN, SYN-ACK, ACK, client FIN+ACK, server ACK, server FIN+ACK, final client ACK.
- Stream 2 (client port 8141): the same orderly FIN exchange.
- The client log records SERVER_EOF with zero response bytes on the first connection and SECOND_CONNECT_SUCCESS on the second.

This is real physical evidence for TCP orderly close on an empty request and subsequent connection acceptance. It is not evidence for an HTTP request, HTTP response body, response ACK-drain ordering, or P5-2C application cleanup after a complete response. Those remain the purpose of the new P5-2C test.
