# PA-220 P5-2C acceptance record

Date: 2026-11-23

## Result

**Cloud-side P5-2C acceptance passed.** The production foreground command is exercised end-to-end in the Host Harness, and the pinned OCTEON N32/full U-Boot build completed successfully.

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

## Remaining boundary

This establishes Host lifecycle behavior and cloud-side target integration/build. The Host foreground test uses a deterministic fake Ethernet boundary; the native Ethernet transmitter has separate P5-1D regression/target-build coverage. **Physical P5-2C HTTP application validation is still required** before claiming P5-2C hardware acceptance.


## Physical validation status and preparation

**P5-2C physical HTTP/TCP acceptance: NOT YET VERIFIED.** The Host harness and OCTEON build are cloud-side results only.

A separate P5-1D physical test report exists at `reports/pa220/P5-1D/30-physical-tcp-validation.md`. It records real PA-220 Ethernet ARP, TCP SYN/SYN-ACK/ACK, and ACK of a 10-byte payload on 2026-11-21 using PA-220 `192.168.2.2/24`, Windows peer `192.168.2.10/24`, TCP port 80, interface `octrgmii0`, 1000 Mbps full duplex. This does not establish P5-2C HTTP parser/handler behavior.

The P5-2C build artifact was downloaded and inspected on 2026-11-23:
- Artifact ZIP SHA-256: `ab13f3b5540ac9bdf18aff898e921dd7992bdb0930980849e896e11d8bfa0283`.
- BIN size: 1,380,488 bytes.
- BIN SHA-256: `825b0e56312da6f4158cd94a5e2da2796b188bfe5d9dcecb5928d9304ad67343`.
- The binary begins with the Cavium `BOOT` header and is a PA-220 OCTEON Stage-3 image, not a generic firmware-upgrade image. **Do not flash this BIN.**

The repository's P5-1D physical report carries explicit safety constraints: do not reboot, reset, power-cycle, execute another `go`, write CPLD/SPI/eMMC/NAND, or restart the foreground service until its state has been confirmed and the user explicitly authorizes the action. These constraints remain in force. No board-side action was taken during this cloud preparation.

Added:
- `reports/pa220/P5-2C/PHYSICAL-TEST-PLAN.md`: safe staged procedure and acceptance criteria.
- `scripts/Test-Pa220Web.ps1`: one-request-per-connection HTTP client, client FIN/EOF and Content-Length checks, 200/404/405 cases, RST followed by recovery request.
- `scripts/Analyze-Pa220WebCapture.ps1`: tshark packet-field CSV exporter.

The test script requires the P5-2C foreground service to already be running. It does not boot, reset, configure, or flash the PA-220. Physical acceptance requires matching serial output, client log, PCAPNG, packet analysis, and the exact running image hash.


A later P5-1D follow-up capture in `PA220-Test/PA220-FIN-20261121-153150` records two real TCP connections with empty payload: client FIN+ACK, server ACK, server FIN+ACK, and final client ACK. The Windows client log also records EOF on the first connection and successful establishment of a second connection. This is physical TCP close/listener-reacceptance evidence only; it does not verify HTTP parsing, response payload transmission, response ACK-drain ordering, or the P5-2C application lifecycle.
