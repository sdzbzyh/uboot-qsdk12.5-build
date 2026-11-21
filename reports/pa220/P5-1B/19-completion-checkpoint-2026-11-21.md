# PA-220 P5-1B completion checkpoint — 2026-11-21

Status: COMPLETE

- Completion commit before this checkpoint: `fd32e1a4d00a27c3c321173b8a68432c55f36bac`
- Final implementation commit: `3f7cf4927dbd03762b2444d0d1d9c3757eaa8905`
- Actions run: `44861736535` — SUCCESS
- Fixed source: `sdzbzyh/pa220-openwrt@801717ba6a8a59ed2cfd53bf3e1b19841fecd2e8`
- SDK: `octeon-sdk-5.1.0`, Cavium GCC 4.7.0 / SDK Build 52
- Final artifact: `14690151455`

## Acceptance

- [x] TCP core source/header included through patch 0003.
- [x] Explicit TCP RX handler registration; NULL/default keeps TCP disabled.
- [x] IPv4 protocol 6 dispatch after IPv4 validation, destination filtering and fragment reassembly.
- [x] Independent TCP segment length validation.
- [x] Existing ARP/ICMP/UDP receive paths unchanged.
- [x] `net/tcp.o` compiled by the real OCTEON compiler.
- [x] `net/tcp.o` included in `net/libnet.o`.
- [x] TCP core and registration symbols retained in final U-Boot ELF despite `--gc-sections`.
- [x] Final ELF, linker map and System.map inspected.
- [x] Full build log, object/ELF/map/symbol reports and checksums committed under `reports/pa220/P5-1B/`.
- [x] Roadmap updated with completion and next-stage entry.

## Evidence

See `10-octeon-integrated-build.log`, `11-tcp-object-elf-header.txt`, `12-libnet-symbols.txt`, `13-final-elf-header.txt`, `14-final-elf-symbols.txt`, `15-tcp-link-map-extract.txt`, `16-tcp-final-elf-symbols.txt`, `17-tcp-system-map-extract.txt`, and `18-final-verification-summary.md`.

All workflow acceptance return codes were 0. The final ELF is MIPS N32 big-endian OCTEON, entry `0xc0000000`, 7,792,898 bytes. Final BIN is 1,367,048 bytes.

## Explicitly not verified

- RAM boot on the PA-220.
- Physical Ethernet TCP receive/send.
- TCP output framing, MAC neighbour resolution, `pa220 web` lifecycle, HTTP parser and URI handlers.

## Next-stage entry

**P5-1C — foreground Web lifecycle and TCP connection management integration.** Register/unregister the TCP handler from the Web lifecycle, connect the receive callback to `tcp_core_input()`, define the single connection context and cleanup rules. Keep all writes/upload/install/reboot features excluded. RAM boot and physical TCP testing remain separate hardware-validation tasks.
