# PA-220 U-BootKit — P5-1D Implementation Checkpoint

## Current state

- Stage: P5-1D — native single-peer TCP IPv4/Ethernet output and neighbor handling
- Status: implementation, focused Host tests, patch generation, full OCTEON build, final link and symbol checks passed
- Hardware status: not yet tested on a physical PA-220; do not claim an on-wire TCP handshake
- Fixed upstream source: `sdzbzyh/pa220-openwrt`
- Fixed source commit: `801717ba6a8a59ed2cfd53bf3e1b19841fecd2e8`
- Integration branch: `feature/pa220-ubootkit-web`
- Branch HEAD immediately before this checkpoint: `f57471910282cf1197eeeee00840705f2659de72`

## Implemented

1. Added `u-boot/net/tcp_ipv4_tx.c` and `u-boot/include/net/tcp_ipv4_tx.h`.
2. Added a synchronous TCP emit callback that:
   - validates the peer IP is on the configured local subnet;
   - learns and pins the peer MAC from the source MAC supplied by the current `NetReceive()` dispatch;
   - rejects zero, multicast, broadcast, self MAC, invalid IP, off-link IP, invalid/non-contiguous masks, network/broadcast host addresses (except RFC 3021 /31);
   - uses `NetSetEther()`, `net_set_ip_header()`, `NetCksum()`, and native `eth_send()`;
   - builds IPv4 + TCP in a private aligned 1600-byte static TX buffer;
   - pads Ethernet frames to 60 bytes and enforces a 1500-byte IPv4 MTU;
   - returns success only when `eth_send()` returns zero.
3. Extended the registered TCP receive callback to carry the source MAC from the current Ethernet frame. The Web layer no longer depends on `NetRxPacket` remaining valid.
4. Kept the first release strictly on-link and single-peer. It does not use the legacy global ARP pending buffer, `NetTxPacket`, or generic routing. The inbound SYN supplies the peer MAC; the IPv4 subnet check proves the peer is directly attached.
5. Bound the real emitter to `pa220 web`. RST, timeout, MAC identity change, Ctrl-C and normal cleanup clear peer and connection state.
6. Fixed two send-failure edges in the existing TCP core: duplicate-SYN retransmission now propagates emitter failure, and failed timer retransmissions consume bounded retry attempts with a deadline rather than retrying on every poll.
7. No HTTP parser, URI handler, remote write path, IPv6, dynamic allocation, persistent environment change, or SPI/eMMC/CPLD write was added.

## Changed integration files in generated P5-1D patch

- `u-boot/common/cmd_pa220_web.c`
- `u-boot/include/net.h` — TCP receive callback signature includes source MAC
- `u-boot/net/net.c` — passes `et->et_src` to the TCP callback
- `u-boot/net/Makefile` — links `tcp_ipv4_tx.o`
- `u-boot/net/tcp.c` — bounded failed retransmission handling
- `u-boot/net/tcp_ipv4_tx.c`
- `u-boot/include/net/tcp_ipv4_tx.h`

The generator starts from the pinned upstream commit, applies the accepted 0001–0004 patches, makes the P5-1D changes, and creates 0005 from the actual staged Git diff. It does not replace the P5-1C TCP source/header or remove the existing TCP link anchor.

## Host verification

GitHub Actions run: `44867249332` — SUCCESS

- `tcp_ipv4_tx: PASS`
- `tcp_core_retry: PASS`
- `tcp_synack_tx: PASS`

The integrated SYN-to-SYN-ACK test uses the real TCP core and real IPv4/Ethernet emitter. It checks Ethernet addresses/type, IPv4 source/destination/length/checksum, TCP ports, sequence/acknowledgment, SYN|ACK flags, header length, MSS, window and TCP pseudo-header checksum. It also checks that a failed Ethernet send does not advance the connection into SYN-RECEIVED.

## OCTEON verification

- SDK release: `octeon-sdk-5.1.0`
- Toolchain: Cavium GCC 4.7.0, SDK Build 52
- Target: `mips64-octeon-linux-gnu`
- SDK download/checksum: PASS
- Fixed source checkout: PASS
- Patch 0005 application: PASS
- Full PA-220 build: PASS
- Required objects: `net/tcp.o`, `net/tcp_ipv4_tx.o`, `common/cmd_pa220_web.o` — all present
- Final ELF: PASS
- Link map: PASS
- System.map: PASS
- Final ELF symbols: `tcp_ipv4_tx_emit`, `tcp_ipv4_tx_learn_peer`, `do_pa220_web`, `tcp_core_input`, `tcp_core_poll`, `net_set_tcp_handler` — all present

## SHA-256 evidence

### Integration source and patch

- `u-boot/net/tcp.c`: `f82e39fbd268970d33a3d22ed25eaa4cfc0a5cc2c52648d4e45be97a344f58d3`
- `u-boot/include/net/tcp.h`: `3593909580af16b3e4909d6af95a77bb759d3ffaeaf4caa3b9b879aa01b217f6`
- `u-boot/net/tcp_ipv4_tx.c`: `ca659cd2363b35e5d8c78ed7da52a1622aeecefef63a19972911e75bce8c7a2e`
- `u-boot/include/net/tcp_ipv4_tx.h`: `9a0ebb306b3be68278ab52e27f8c978ef18b3fd8b7002d8e06510dbb30aab67c`
- `u-boot/common/cmd_pa220_web.c`: `5d2ab89db669995fd07e035356774c85774c353c5a7b229750752932c1cb2fee`
- `patches/pa220/0005-pa220-native-tcp-ipv4-ethernet-output.patch`: `24589f80c99538e5b2f4a62b67185cc6f58ace358999ec134de85c63438fb24f`

### Target objects and final artifacts

- `net/tcp.o`: `69e9abe7ec4964c2e038ef47bb6c736a22d5cb5846258c228b0dbd5f2ab7a336`
- `net/tcp_ipv4_tx.o`: `229a18fd8edb39e29b3cd958e2915b4676b644afbbcd9c7d1bc4e4015ee83a09`
- `common/cmd_pa220_web.o`: `f3995ccbe847c63d46a755793242ced7c66851168d740226af2c9d04a37597ad`
- Final ELF: `d72b416add1c43bb5e82e1bf42c5e82712666d6c6e8478674f3d302148c657f0`
- Link map: `f7e313faf0363045d3129cd79f5ee7817999e492ee3b7b70a734a2e9ad8debfc`
- System.map: `e946ee24cf2df71e2a9a34bc3dd579d74c456508201f2e5fba220f868a38be1a`
- Final binary: `63ec085a77e5a415dad4f52affb5a19889110484aaf7fd15c0a57336bc97c69d`

## Evidence files

- `reports/pa220/P5-1D/02-host-tests.log`
- `reports/pa220/P5-1D/03-patch-generation.log`
- `reports/pa220/P5-1D/04-patch-sha256.txt`
- `reports/pa220/P5-1D/10-octeon-integrated-build.log`
- `reports/pa220/P5-1D/12-final-elf-header.txt`
- `reports/pa220/P5-1D/13-final-elf-symbols.txt`
- `reports/pa220/P5-1D/14-p5-1d-link-map-extract.txt`
- `reports/pa220/P5-1D/15-p5-1d-final-elf-symbols.txt`
- `reports/pa220/P5-1D/16-p5-1d-system-map-extract.txt`
- `reports/pa220/P5-1D/18-final-verification-summary.md`

The full build log and output artifact bundle are also attached to Actions run `44867249332`.

## Remaining acceptance items

- Physical PA-220 RAM-boot and Ethernet capture validation has not been performed.
- Confirm SYN/SYN-ACK/ACK, retransmission, data, FIN, RST, send failure and timeout behavior on the actual board.
- HTTP parser and read-only handlers remain out of scope and must not be claimed as implemented.

## Next unique objective

Perform controlled, non-persistent PA-220 hardware validation of the TCP handshake and packet output. Do not write SPI/eMMC/CPLD or persistent environment. Do not begin HTTP implementation until the on-wire TCP behavior is verified.
