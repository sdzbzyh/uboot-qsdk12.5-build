# PA-220 P5-1D CLOSE-WAIT build artifact

Date: 2026-11-21  
Integration commit built: `8abc749cf5808d0f65f034cdfc172d68f9b981c5`  
Evidence commit / branch HEAD at report creation: `1586330569a6403c1cc64f1d275cdc83beaa131c`

## Build result

- GitHub Actions run: [44879234047](https://github.com/sdzbzyh/uboot-qsdk12.5-build/actions/runs/44879234047) — SUCCESS
- Artifact ID: `14697431062`
- Artifact name: `pa220-p5-1d-native-tcp-ipv4-ethernet`
- Artifact ZIP size: 4,405,863 bytes
- Artifact ZIP SHA-256: `f772ad09a990b43ebc6f62c3ba5f7641b0fe66dbddafc3de4ef1e8f7fbfd2008`
- Artifact expiry: 2026-12-21
- Build output: `bin/pa220/u-boot-octeon_pa220.bin`
- BIN size: 1,370,376 bytes (`0x14E908`)
- BIN SHA-256: `2318987d4d3a4ea3817809cb3bf00ab4297009b78f153adfcadc42426318d57d`

## Target identity

- PA-220 source commit: `801717ba6a8a59ed2cfd53bf3e1b19841fecd2e8`
- Toolchain: octeon-sdk-5.1.0 / Cavium GCC 4.7.0 / SDK Build 52
- Compiler target: `mips64-octeon-linux-gnu`
- Final ELF: ELF32, big-endian, MIPS, flags include `octeon` and `mips64r2`
- Entry point: `0xC0000000`
- Patch 0005 SHA-256: `25ca4536b135bb4e18d97249537842bd566a2a6b1e9cd1a49c73628b05011838`

## Acceptance

- Host tests: `tcp_ipv4_tx`, `tcp_core_retry`, `tcp_synack_tx`, `tcp_close_wait` — all PASS
- Patch generation and reverse-apply validation: PASS
- SDK download and checksum: PASS
- Exact pinned source checkout: PASS
- Patch application: PASS
- Full OCTEON build: PASS
- Required objects, final ELF, map, System.map and symbols: PASS

This is a successful code/Host/OCTEON build result only. It does not claim deployment or physical FIN orderly-close validation. The physical test must still observe peer FIN, PA-220 ACK, application-triggered local FIN, peer final ACK, connection cleanup, listener recovery and a subsequent connection on the wire.
