# PA-220 P5-2C physical evidence

Physical HTTP test date: 2026-11-23 (local time UTC+08:00).

## Evidence files

All five evidence files are present in this directory:

- `pa220-p5-2c-http.pcapng` — raw packet capture, 24,984 bytes.
- `pa220-web-test.log` — Windows PowerShell client result, 1,087 bytes.
- `pa220-web-packets.txt` — complete TShark field export, 15,846 bytes.
- `pa220-serial.log` — provenance-labelled reconstruction from the conversation record, **not an original UART/serial-terminal capture**.
- `u-boot-octeon_pa220.bin` — TFTP image, 1,380,488 bytes.

## Host-side evidence hashes

| File | Size (bytes) | SHA-256 |
|---|---:|---|
| pa220-p5-2c-http.pcapng | 24984 | 27B0F5C0537B8140F6773725FDB7D0E1493896DBED6A094DC76632AFCE31768C |
| pa220-web-test.log | 1087 | 47F316FE2EC28DB187A66EA9C167CFFEC20190613059A698693D0543CE943 |

## Device image identity

Expected cloud build image SHA-256:

`825b0e56312da6f4158cd94a5e2da2796b188bfe5d9dcecb5928d9304ad67343`

The user computed this SHA-256 for the local TFTP-served file and confirmed a size of 1,380,488 bytes. The uploaded repository BIN has the same reported size. The SHA-256 was not independently recomputed from the GitHub-hosted binary in this session; see the provenance note in `pa220-serial.log`.

## Test result and packet review

PowerShell client summary: `SUMMARY PASS cases=8 rst_recovery=PASS`.

The client log records:
- GET `/api/board`, `/`, `/api/status`, `/api/memory`, `/api/network`: HTTP 200.
- GET `/not-found`: HTTP 404.
- POST `/api/status`: HTTP 405.
- Abort/reset exercise followed by a new successful GET `/api/status`: HTTP 200.

The packet export includes unrelated earlier TCP/HTTP traffic in frames 1–98. The P5-2C test window is frames 99–192. It independently shows the five successful GET responses, 404, 405, the abort/reset flow, and the recovery GET.

Important qualification: the abort connection (client port 5950) completed its handshake, then the client sent FIN before any HTTP request payload. The server returned HTTP 400 and the client then sent RST+ACK. A new connection (client port 5951) subsequently sent GET `/api/status` and received HTTP 200. This proves the observed close/reset flow and successful service on a subsequent connection; it does **not** prove an RST interrupted a partially transmitted HTTP request.

## Serial-record provenance

`pa220-serial.log` was reconstructed from the commands, outputs, and observations explicitly reported in the 2026-11-23 conversation. It is conspicuously labelled as a reconstruction and is not represented as an original serial capture. No missing UART output or timestamps were fabricated.

## Evidence boundary

The Windows client log, PCAPNG, TShark export, and image identity support the physical HTTP/TCP test result. The serial record is secondary reconstructed context, not raw serial evidence. The image hash was checked on the user's Windows TFTP host and matches the cloud build hash; it was not recalculated from the uploaded GitHub binary in this session.
