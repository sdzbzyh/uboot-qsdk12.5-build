# PA-220 U-BootKit — P5-1D Completion Checkpoint

**P5-1D code and cloud-build acceptance: COMPLETE. Physical TCP acceptance: PENDING.**

- Branch: `feature/pa220-ubootkit-web`
- Pre-checkpoint branch HEAD: `f57471910282cf1197eeeee00840705f2659de72`
- Fixed PA-220 source: `801717ba6a8a59ed2cfd53bf3e1b19841fecd2e8`
- P5-1D patch: `patches/pa220/0005-pa220-native-tcp-ipv4-ethernet-output.patch`
- Patch SHA-256: `24589f80c99538e5b2f4a62b67185cc6f58ace358999ec134de85c63438fb24f`
- Successful Actions run: `44867249332`
- Successful evidence commit before this checkpoint: `f57471910282cf1197eeeee00840705f2659de72`

## Completion evidence

- Three focused Host tests passed, including real TCP-core SYN-to-SYN-ACK emission through the real IPv4/Ethernet output callback.
- P5-1D patch was generated from the pinned PA-220 source plus accepted P5-1C patches, using actual Git diff output.
- Private SDK checksum passed; Cavium GCC 4.7.0 / SDK Build 52 completed the full PA-220 OCTEON build.
- `tcp.o`, `tcp_ipv4_tx.o`, and `cmd_pa220_web.o` compiled and were included in the final link.
- Final ELF, map, System.map and six required symbols passed verification.
- Source, patch, object and final artifact SHA-256 values are recorded in `01-implementation-checkpoint.md` and the full build log.

## Safety and scope

- Single on-link IPv4 peer only; no generic routing or ARP pending queue.
- Separate static TX frame; synchronous `eth_send()` return is honored.
- Failed retransmissions are bounded; peer state is cleared on session reset/timeout/exit.
- No HTTP parser or handler exists.
- No persistent environment, SPI, eMMC or CPLD writes were made.

## Next task

Only perform physical TCP validation using a non-persistent test method. Capture and verify SYN/SYN-ACK/ACK, then exercise data, FIN, RST, retransmission and timeout. Do not proceed to HTTP parser work until those board-level checks pass.
