# PA-220 P5-2C acceptance record

Date: 2026-11-23

## Result

**Cloud-side P5-2C acceptance passed. Physical P5-2C HTTP/TCP validation now also passes with the evidence qualifications documented below.** The production foreground command was exercised end-to-end in the Host Harness, the pinned OCTEON N32/full U-Boot build completed successfully, and the RAM-loaded PA-220 image served the tested HTTP requests on the physical Ethernet link.

## Root cause fixed

The P5-1 native network dispatcher patch references `tcp_core_link_anchor()` from `u-boot/net/net.c` to retain the TCP API table under `--gc-sections`. The staged production `u-boot/net/tcp.c` and `u-boot/include/net/tcp.h` had lost the anchor implementation and declaration. The complete U-Boot build therefore stopped at:

```
net.c:682:17: error: 'tcp_core_link_anchor' undeclared here
```

Restored the API-reference table and `tcp_core_link_anchor()` implementation in `u-boot/net/tcp.c`, and declared it in `u-boot/include/net/tcp.h`.

Source fix commit: `b0648b6e1312d45d6cf2f5b9ab8d34dacd072fe0`.

## Host verification

- Workflow: [Run #45147100639](https://github.com/sdzbzyh/uboot-qsdk12.5-build/actions/runs/45147100639)
- Tested commit: `005c75c17946bf619b0171aa0bf3ae241bd649ea`
- Component suite: **29 scenarios, 161 assertions, 0 failures**.
- Foreground command: **PASS**.
- Simulated exchange: SYN/SYN-ACK/ACK, complete `GET /api/board` request carried with FIN, HTTP 200 response, response ACK, server FIN, final ACK, and cleanup.
- The test captured 201 response bytes and verified the server did not send FIN until all 201 bytes had been acknowledged.
- Host artifact: [pa220-p5-2c-host](https://github.com/sdzbzyh/uboot-qsdk12.5-build/actions/runs/45147100639/artifacts/14805205816).

## OCTEON N32 and full target build

- Workflow: [Run #45146562558](https://github.com/sdzbzyh/uboot-qsdk12.5-build/actions/runs/45146562558)
- Tested commit: `25a2970fc3b5e25ba8f3e764ae2337899ee1d898`
- SDK: `octeon-sdk-5.1.0`, SHA-256 verified.
- PA-220 source baseline: `801717ba6a8a59ed2cfd53bf3e1b19841fecd2e8`.
- Six production objects were staged and the N32 relocatable link passed.
- Complete PA-220 U-Boot final link passed.
- Final image: `u-boot-octeon_pa220.bin`.
- Image SHA-256: `825b0e56312da6f4158cd94a5e2da2796b188bfe5d9dcecb5928d9304ad67343`.
- ELF SHA-256: `19e3ca45310fd5ae46c859d5392408109919a371fe94ff087f0d46267d5b5fcb`.
- Map SHA-256: `fa480c98a6608d61ee224d38613745956d51c64d6411334eb5bb239eaa004484`.
- Build artifact (image, ELF, map, System.map, object files, logs and stack-usage reports): [pa220-p5-2c-octeon](https://github.com/sdzbzyh/uboot-qsdk12.5-build/actions/runs/45146562558/artifacts/14803963927).

## Physical P5-2C HTTP/TCP validation

<badge>PASS WITH EVIDENCE QUALIFICATIONS</badge>

Date: 2026-11-23, local time UTC+08:00.

### Device and test setup

- PA-220 address: `192.168.2.2/24`.
- Windows test peer: `192.168.2.10/24`.
- Interface: `octrgmii0`, 1000 Mbps full duplex.
- The P5-2C image was TFTP-loaded to RAM at `0x81000000` and executed with `go 0x81000000`; this was not a flash operation.
- Reported U-Boot identity: U-Boot 2013.07, build time Nov 23 2026 09:23:48, Cavium SDK gcc 4.7.0.
- User-computed TFTP-host BIN size: 1,380,488 bytes.
- User-computed TFTP-host BIN SHA-256: `825b0e56312da6f4158cd94a5e2da2796b188bfe5d9dcecb5928d9304ad67343`, matching the cloud build image hash.
- The uploaded GitHub BIN has the same reported size. Its SHA-256 was not independently recomputed from the GitHub-hosted binary during this review.

### Client result

The foreground `pa220 web` service was already running on TCP port 80 during the test. The Windows test client reported:

| Request | Result |
|---|---:|
| GET `/api/board` | 200 |
| GET `/` | 200 |
| GET `/api/status` | 200 |
| GET `/api/memory` | 200 |
| GET `/api/network` | 200 |
| GET `/not-found` | 404 |
| POST `/api/status` | 405 |
| Abort/reset exercise followed by new GET `/api/status` | 200 |

Client log summary: **`SUMMARY PASS cases=8 rst_recovery=PASS`**.

### Packet-level review

The uploaded PCAPNG is 24,984 bytes; the TShark export is 15,846 bytes. The P5-2C test flows are frames 99–192; frames 1–98 contain earlier unrelated traffic.

The packet export corroborates the five successful GET responses, 404, 405, and the close/reset plus subsequent recovery connection.

Important qualification: the abort-test connection (client source port 5950) completed its handshake, then the client sent FIN before any HTTP request payload. The server returned HTTP 400; the client then sent RST+ACK. A new connection (source port 5951) subsequently sent GET `/api/status` and received HTTP 200. This demonstrates the observed close/reset flow and successful service on a subsequent connection. It does **not** demonstrate an RST interrupting a partially transmitted HTTP request.

### Serial evidence provenance

`reports/pa220/P5-2C/physical-evidence/pa220-serial.log` was reconstructed from the commands, outputs, and observations explicitly reported in the 2026-11-23 conversation. It is labelled as a reconstruction and is **not** an original UART/serial-terminal capture. The original serial capture was not provided. The reconstruction does not invent missing boot messages or timestamps.

### Physical evidence bundle

All five files are present under `reports/pa220/P5-2C/physical-evidence/`:

- `pa220-p5-2c-http.pcapng`
- `pa220-web-test.log`
- `pa220-web-packets.txt`
- `pa220-serial.log` (reconstructed session record, not raw capture)
- `u-boot-octeon_pa220.bin`

The PCAPNG and client-log SHA-256 values supplied by the user are recorded in that directory's README. The client log and packet capture are separate corroborating sources; the reconstructed serial record is contextual evidence only.

## Safety boundary

The tested image is a Cavium OCTEON PA-220 Stage-3 image, not a generic firmware-upgrade image. **Do not flash this BIN.**

The reported test was performed with the image loaded and executed from RAM. No saveenv, CPLD write, SPI/eMMC/NAND write, reboot, reset, or power-cycle was part of this validation. Do not perform any such operation as a follow-up to this acceptance without a separately reviewed and explicitly authorized procedure.

## Historical preparation note (superseded)

The earlier preparation text below was written before the 2026-11-23 physical HTTP test and is superseded by the PASS result above.

A separate P5-1D physical test report exists at `reports/pa220/P5-1D/30-physical-tcp-validation.md`. It records real PA-220 Ethernet ARP, TCP SYN/SYN-ACK/ACK, and ACK of a 10-byte payload on 2026-11-21. That earlier test alone did not establish P5-2C HTTP parser/handler behavior.

The P5-2C build artifact was previously downloaded and inspected:
- Artifact ZIP SHA-256: `ab13f3b5540ac9bdf18aff898e921dd7992bdb0930980849e896e11d8bfa0283`.
- BIN size: 1,380,488 bytes.
- BIN SHA-256: `825b0e56312da6f4158cd94a5e2da2796b188bfe5d9dcecb5928d9304ad67343`.
- The binary begins with the Cavium `BOOT` header and is a PA-220 OCTEON Stage-3 image.

Added during preparation:
- `reports/pa220/P5-2C/PHYSICAL-TEST-PLAN.md`
- `scripts/Test-Pa220Web.ps1`
- `scripts/Analyze-Pa220WebCapture.ps1`

A later P5-1D follow-up capture in `PA220-Test/PA220-FIN-20261121-153150` records TCP close/listener-reacceptance evidence only; it does not verify P5-2C HTTP parsing or application lifecycle.
