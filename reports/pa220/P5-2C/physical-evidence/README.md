# PA-220 P5-2C physical evidence

Physical HTTP test date: 2026-11-23 (local time UTC+08:00)

## Evidence files

Upload the following raw files into this directory without renaming or modifying them:

- `pa220-p5-2c-http.pcapng`
- `pa220-web-test.log`
- `pa220-web-packets.txt` (the complete TShark field export)
- `pa220-serial.log` (complete serial capture from RAM image launch through HTTP test completion)
- `u-boot-octeon_pa220.bin` (the exact binary loaded by TFTP, only if repository size/policy permits; otherwise record its SHA-256 and size below and retain the binary in the build artifact)

## Host-side evidence hashes

| File | Size (bytes) | SHA-256 |
|---|---:|---|
| pa220-p5-2c-http.pcapng | 24984 | 27B0F5C0537B8140F6773725FDB7D0E1493896DBED6A094DC76632AFCE31768C |
| pa220-web-test.log | 1087 | 47F316FE2EC28DB187A66EA9C167CFFEC20190613059A698693543D0543CE943 |

## Device image identity

Expected cloud build image SHA-256:

`825b0e56312da6f4158cd94a5e2da2796b188bfe5d9dcecb5928d9304ad67343`

Actual TFTP-served file SHA-256 and size: pending local verification.

## Test result

PowerShell client summary: `SUMMARY PASS cases=8 rst_recovery=PASS`.

The packet capture contains the eight test flows, including 200 responses for the five successful GET endpoints, 404, 405, the abort/reset flow, and a subsequent successful GET /api/status. Review notes must preserve the observed detail that the abort connection (client port 5950) sent FIN before any HTTP payload, then received HTTP 400 and ended with RST+ACK; the next connection (client port 5951) successfully received HTTP 200.

## Evidence boundary

This directory is the raw evidence bundle. The acceptance record must separately identify which evidence was independently inspected. Do not treat the host log alone as sufficient physical-device proof.
