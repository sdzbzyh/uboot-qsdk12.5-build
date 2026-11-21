# P5-1B implementation unit 1 — native IPv4 TCP dispatch patch

- Base integration HEAD: `bdf3accbe3f20c907d752e2a6315d6a6f3f53cf3`
- Fixed PA-220 source: `801717ba6a8a59ed2cfd53bf3e1b19841fecd2e8`
- Target source files fetched from `sdzbzyh/pa220-openwrt`; TCP core source/header copied verbatim from the integration branch.
- Patch order verified: 0001 and 0002 only; this is 0003.
- P4 collision check: branch HEAD equals the checkpoint commit; no subsequent shared-branch commit exists. No P4-owned files are changed.
- Baseline blob IDs:
  - net.c `5b9ac34a0e50e3010b1efcaed7499c4059dca449`
  - net.h `0802fad8761364d97f04a799e91d01ca4cd2eb06`
  - net/Makefile `4c9a97f04d668d95299d9998f941260d3f3c6c07`
- TCP core blob IDs:
  - tcp.c `ea8cb05a9eb6aabbaff3dbbdbe461128a70789b0`
  - tcp.h `c4d71e0afb05e4f6d54b1ca3c6c68ded1dee33b5`

## Changes

1. Add `tcp_rxhand_f` and `net_set_tcp_handler()`; NULL disables dispatch.
2. Clear TCP registration in the existing network handler cleanup path.
3. Add IPv4 protocol 6 dispatch after existing IP validation, destination filtering and `NetDefragment()`.
4. Require post-defragment IPv4 total length to equal the available datagram length; independently bound TCP segment length to [20, 65535].
5. Add `tcp.o` to the `CONFIG_CMD_NET` object list.
6. Add existing TCP core source and header unchanged.

The callback receives a transient segment pointer, length, source IP and destination IP. `tcp_core_input()` performs TCP header/data-offset/options and pseudo-header checksum validation.

## Patch provenance

The unified diff was generated programmatically from the exact fetched baseline contents and modified file contents. No hunk count was manually edited. The cloud workflow must apply-check/apply this patch against the pinned source before building.

- Patch path: `patches/pa220/0003-pa220-native-ipv4-tcp-dispatch.patch`
- Patch text bytes: 22716
- Paths: u-boot/net/net.c, u-boot/include/net.h, u-boot/net/Makefile, u-boot/net/tcp.c, u-boot/include/net/tcp.h

## Pending

Cloud patch-apply check, full Cavium OCTEON GCC 4.7.0 build, final ELF/map/symbol proof, and separate RAM boot/physical TCP validation.
