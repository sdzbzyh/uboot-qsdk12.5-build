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

This establishes Host lifecycle behavior and cloud-side target integration/build. The Host foreground test uses a deterministic fake Ethernet boundary; the native Ethernet transmitter has separate P5-1D regression/target-build coverage. **Physical PA-220 Ethernet/TCP runtime validation is still required** before claiming hardware acceptance.
