# PA-220 P5-1C final verification summary

Status: **OCTEON target build and final link PASS.** Foreground lifecycle and TCP receive-core integration are present in the final image. A usable TCP/Web session is intentionally not claimed because the required TCP IPv4/Ethernet/ARP output path is not implemented.

## Revisions and build

- Integration branch: `feature/pa220-ubootkit-web`
- P5-1C implementation commit: `70c6c48d4f84a5a81089ec68dcd97590ac5960bd`
- Evidence commit before this report: `54cd5b33ccc54f1ca899e896e00d460fc5a936f4`
- Fixed PA-220 source: `sdzbzyh/pa220-openwrt@801717ba6a8a59ed2cfd53bf3e1b19841fecd2e8`
- Toolchain: `sdzbzyh/pa220-toolchains` release `octeon-sdk-5.1.0`
- Compiler: Cavium GCC 4.7.0, SDK Build 52, target `mips64-octeon-linux-gnu`
- GitHub Actions run: [44863474734](https://github.com/sdzbzyh/uboot-qsdk12.5-build/actions/runs/44863474734) — SUCCESS

## Pipeline result

All final status codes are zero:

| Check | Result |
|---|---:|
| SDK download | 0 |
| SDK SHA-256 | 0 |
| Exact source checkout | 0 |
| Ordered patch application via `build-pa220.sh` | 0 |
| Full PA-220 target build | 0 |
| Web command object | 0 |
| Final ELF | 0 |
| Link map | 0 |
| System.map | 0 |
| Required final symbols | 0 |

The workflow applies the patch series only through `build-pa220.sh`; it does not pre-apply patches or invoke the full build more than once in the job. P5-1A Host regression and P5-1B completed validation were not repeated.

## Final ELF evidence

- ELF: ELF32, big-endian, MIPS N32, OCTEON / MIPS64r2, statically linked.
- Entry point: `0xc0000000`.
- Text / data / BSS: `1,338,304 / 30,108 / 1,679,916` bytes.
- Final image size: `1,368,456` bytes.

Required symbols are present in the final ELF, link map and System.map:

| Symbol | Address |
|---|---:|
| `do_pa220_web` | `0xc00b75c4` |
| `net_set_tcp_handler` | `0xc0106834` |
| `tcp_core_poll` | `0xc0109a94` |
| `tcp_core_input` | `0xc0109dc8` |

### SHA-256

| Artifact | SHA-256 |
|---|---|
| `common/cmd_pa220_web.o` | `3d8fd10605a9cae11f94611b9a14dfb63a162d26d15ca94d63bdbab44d03a779` |
| `u-boot-octeon_pa220` | `594f2e877ba6e8d40ad3119efb470a3216dd91f159fee6bc20c2b0733c546305` |
| `u-boot-octeon_pa220.map` | `868be28e7bc5e5823d744154e8e514a744d761806a3f20942200ed69ec82b2b7` |
| `System-octeon_pa220.map` | `33992854eb3d5f6861440b1027227f31d010c156412b435bdadc58986d0345a2` |
| `u-boot-octeon_pa220.bin` | `704a9ac11bb65241dd34383e9752019634e40ac60b84cedab0af01ccda275199` |

Full object, ELF, map, System.map and binary are retained in Actions artifact `pa220-p5-1c-foreground-web-lifecycle` (artifact ID `14691016829`, SHA-256 `44292cec4d9c00d94da96f5330ddbf3004dd6a1cbb36269bce1dcd32a531879f`, expires 2026-12-21).

## Implementation boundary

- The existing `pa220` command is extended with `pa220 web`; no duplicate top-level command registration remains.
- The service initializes Ethernet, owns a foreground `eth_rx()` loop, registers the TCP RX handler after startup, and calls `tcp_core_poll()`.
- One static `tcp_core_connection` is used.
- The RX handler passes received segments to `tcp_core_input()`.
- Ctrl-C unregisters the handler before clearing the connection context and halting Ethernet.
- The original NetLoop and DHCP/Ping/TFTP command source paths are not modified.
- No HTTP parser, URI handler, upload, arbitrary command execution, environment write, storage write, CPLD write, firmware installation, reboot or shutdown operation is added.

## TCP output limitation — not a working Web server

The output callback returns failure until a reviewed TCP IPv4/Ethernet/ARP emitter exists. It does not falsely report a discarded SYN-ACK or ACK as transmitted. Consequently, a passive TCP handshake cannot complete; the service is a foreground lifecycle / RX integration harness, not a functioning TCP server or Web server.

The existing TCP core contains connection-state, receive-buffer, FIN/RST and timeout handling, and the foreground loop invokes its poll API. However, established-session data delivery, peer close exchange and timeout behavior have not been demonstrated over the wire because no connection can be established without output. This remains an explicit follow-up dependency, not a passed acceptance item.

## P4 isolation and hardware boundary

The P4 plan remains IN PROGRESS. This P5-1C change did not modify P4 source files or P4 reports. DHCP/Ping hardware tests remain unperformed, and TFTP receive remains blocked.

RAM boot, physical Ethernet/TCP exchange, and any future TCP output validation remain separate hardware-validation work. Cross-compilation is not presented as a substitute for those tests.
