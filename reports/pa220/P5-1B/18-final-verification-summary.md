# PA-220 P5-1B final verification summary

## Final result

P5-1B passed the full GitHub Actions target build and final-link acceptance.

- Actions run: https://github.com/sdzbzyh/uboot-qsdk12.5-build/actions/runs/44861736535
- Actions result: SUCCESS
- Integration branch: `feature/pa220-ubootkit-web`
- Build commit: `3f7cf4927dbd03762b2444d0d1d9c3757eaa8905`
- Fixed PA-220 source: `801717ba6a8a59ed2cfd53bf3e1b19841fecd2e8`
- SDK: private `octeon-sdk-5.1.0`, Cavium GCC 4.7.0 / SDK Build 52
- Artifact ID: `14690151455`; SHA-256: `3240a7430493937dfcb1c96d9b42a06ecefbea999531f2645b1909a129953cb0`

## Build products

| Product | Size | SHA-256 |
|---|---:|---|
| `net/tcp.o` | 53,016 bytes | `93dc548ef969f2e8aaf80f7e92ce108a58c4d1ca487749b7f3cc86f2959055ff` |
| `net/libnet.o` | 401,774 bytes | `a16f11fd172cf8a9d81485275e6f7eacc308c2e7a80d9846821474be1552b459` |
| `u-boot-octeon_pa220` | 7,792,898 bytes | `1895888acbb6ee5248bd84d6cb4c856c59c32f6493fba8cc32d9a630a1987b4b` |
| `u-boot-octeon_pa220.map` | 580,617 bytes | `fd37b6748bad1810cec52b9f63a07f3dd8c069354024de326aba662b0ee098fb` |
| `System-octeon_pa220.map` | 101,337 bytes | `513fbfb5cff40cd2d1cde49d9a33df9d8879e83c3e5bdf27b8ae13426cacd6a7` |
| `u-boot-octeon_pa220.bin` | 1,367,048 bytes | `98e7c30370aff5a7900b6eb062da38eea81b64f1c5f7bff6e72a7c3e2c00bb7d` |

## Final ELF symbols

The final ELF and System.map independently contain these symbols:

| Symbol | Address |
|---|---:|
| `net_set_tcp_handler` | `0xc0106560` |
| `tcp_seq_before` | `0xc0108e2c` |
| `tcp_seq_after` | `0xc0108e38` |
| `tcp_core_init` | `0xc0108f20` |
| `tcp_core_listen` | `0xc0108f38` |
| `tcp_core_read` | `0xc0108fc0` |
| `tcp_checksum_ipv4` | `0xc0109068` |
| `tcp_checksum_ipv4_valid` | `0xc01091c0` |
| `tcp_core_close` | `0xc01095b4` |
| `tcp_core_write` | `0xc010965c` |
| `tcp_core_poll` | `0xc01097c0` |
| `tcp_segment_parse` | `0xc01098d0` |
| `tcp_core_input` | `0xc0109af4` |
| `tcp_core_link_anchor` | `0xc010a03c` |

## Status boundary

P5-1B is complete. RAM boot and physical TCP Ethernet receive/send remain NOT VERIFIED. TCP output, neighbour resolution, Web foreground lifecycle and HTTP parsing are outside P5-1B. Next development entry is P5-1C.
