# PA-220 U-BootKit — P5-1D RAM-boot safety audit

Date: 2026-11-21
Stage: P5-1D physical TCP validation preparation
Status: **RAM transfer method documented; execution blocked by explicit no-CPLD-write constraint**

## Repository and build provenance

- Repository: `sdzbzyh/uboot-qsdk12.5-build`
- Branch: `feature/pa220-ubootkit-web`
- Branch HEAD audited before this report: `9786461fcb54c54f45c4d0ec64a0bde898516cca`
- Fixed PA-220 source: `sdzbzyh/pa220-openwrt@801717ba6a8a59ed2cfd53bf3e1b19841fecd2e8`
- P5-1D Actions run: `44867249332` — SUCCESS
- Actions build commit: `e0a528acfcd4d023d87afc5096e939c132bebece`
- Artifact: `pa220-p5-1d-native-tcp-ipv4-ethernet` (artifact ID `14692751342`)
- Artifact ZIP SHA-256: `66344d5efc88b8efc38119e2f60701910bedbb32b1f94736f60dca6dbf413b2f`
- Final BIN: `bin/pa220/u-boot-octeon_pa220.bin`
- Final BIN SHA-256: `63ec085a77e5a415dad4f52affb5a19889110484aaf7fd15c0a57336bc97c69d`
- Final ELF SHA-256: `d72b416add1c43bb5e82e1bf42c5e82712666d6c6e8478674f3d302148c657f0`

The current branch is exactly two commits ahead of the Actions build commit. The comparison shows those later commits add/update only P5-1D evidence reports; no implementation source or patch files changed after the successful build. Therefore the artifact corresponds to the current P5-1D implementation.

## Artifact format and addresses

The downloaded artifact was inspected directly:

- BIN size: 1,370,120 bytes.
- ELF: 32-bit, big-endian MIPS executable, N32 ABI, MIPS64r2 / OCTEON flags.
- ELF entry: `0xC0000000`.
- PT_LOAD virtual/physical address: `0xC0000000`.
- PT_LOAD file size: `0x0014e808`; memory size: `0x002e912c`.
- Link/text base: `0xC0000000`.
- This is the PA-220 Cavium OCTEON bootloader-header BIN, not a generic Linux kernel or FIT image.

The ELF entry/link address is not the documented TFTP RAM destination. The project documentation distinguishes the two.

## Existing non-persistent transfer/launch procedure

The repository's `doc/PA220.md` and the fixed upstream `docs/INSTALLATION.md` document the original Palo Alto U-Boot RAM test:

1. Set temporary network variables in the original U-Boot RAM environment.
2. TFTP the custom U-Boot image to `0x81000000`.
3. Run `inv_icache`.
4. Transfer control with `go 0x81000000`.
5. The expected custom prompt is `pa-220#`; power cycling returns to the original SPI boot chain.

The upstream installation guide explicitly describes this as a RAM start and says nothing is written before the later, separate SPI-flash step. No `saveenv`, `sf erase/write/update`, eMMC write, or SPI write is required for the RAM transfer/launch sequence.

Historical P3 evidence is weaker: the recovered user-observed record confirms a TFTP transfer to `0x81000000` and `bootloadervalidate: PASS`, but does **not** record a `go` execution. It is not proof that the current P5-1D binary has already executed on the user's board.

## Hard blocker: normal board startup writes CPLD registers

The fixed-source audit `doc/pa220/audit/P1-A5-CALLCHAIN-RAM-CONVERGENCE.md` establishes that the custom Stage-3 U-Boot startup path includes automatic CPLD writes:

- `early_board_init()` calls `pa220_early_stat_led()`, which remaps CPLD CS0 and writes CPLD registers 7 and 6.
- Later, `octeon_boot_bus_late_init()` calls `octeon_boot_bus_board_post_init()`, which reads CPLD register d and pulses bit 0 low-to-high as the board boot-watchdog handoff.

These writes occur as part of normal startup; they are not optional operator commands. The same audit notes that the CPLD register-d pulse is part of the existing board startup path.

The user instruction for this phase explicitly prohibits **CPLD writes**. Therefore executing the documented `go 0x81000000` path would violate the current constraint even though the image is loaded into RAM and no SPI/eMMC image is written.

## Decision

**Do not execute the image on the physical PA-220 under the current instruction set.**

The TFTP transfer and image validation can be separated from execution, but cannot validate TCP transmission. There is no evidence in the audited material of a supported runtime switch that suppresses both startup CPLD write paths without modifying/rebuilding board initialization. Do not assume one exists.

Required resolution before hardware execution:

1. Explicitly authorize the two existing automatic board-startup CPLD initialization actions (registers 7/6 and register d pulse), while continuing to prohibit SPI/eMMC writes and persistent environment changes; **or**
2. Authorize a separately reviewed test-only startup variant that suppresses all CPLD writes, with its own build and audit. This would be a distinct change and is not performed in this phase.

Until one of those conditions is resolved, P5-1D physical TCP validation remains **BLOCKED**. Do not proceed to HTTP parser work.

## Scope assurance

- No fixed upstream source was modified.
- No P5-1A/B/C/D implementation was repeated or changed.
- No build was rerun.
- No device command was executed.
- No SPI, eMMC, CPLD, or persistent environment was written by this audit.
- No claim of physical TCP functionality is made.
