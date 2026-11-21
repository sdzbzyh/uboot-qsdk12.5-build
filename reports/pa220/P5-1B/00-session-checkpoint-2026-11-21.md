# PA-220 U-BootKit P5-1B session checkpoint

- Date: 2026-11-21
- Integration repository: `sdzbzyh/uboot-qsdk12.5-build`
- Branch: `feature/pa220-ubootkit-web`
- Checkpoint base HEAD: `57c2e350ec3906abcdeebc2a220005b6d3d53052`
- Fixed PA-220 source: `sdzbzyh/pa220-openwrt@801717ba6a8a59ed2cfd53bf3e1b19841fecd2e8`
- Cloud build environment: GitHub Actions; private SDK release `sdzbzyh/pa220-toolchains@octeon-sdk-5.1.0`; secret `OCTEON_SDK_READ_TOKEN` is configured.

## Phase handoff

P5-1A is complete to the following evidence level:
- Standalone TCP core Host focused regression: PASS, 76 assertions passed / 0 failed.
- Standalone TCP core compiled with Cavium GCC 4.7.0 / SDK Build 52 as PA-220 OCTEON N32 relocatable object `net/tcp.o`: PASS.
- Object ELF inspection: PASS; 49,564 bytes; SHA-256 `16ccf75c9cb4e779753ee99fdfeadba21802e9d6a6ed009cb8b9d33da7240ca2`.
- PA-220 baseline full build with existing patches 0001/0002: PASS.
- TCP core final integration link into PA-220 U-Boot: NOT RUN.
- RAM boot and physical TCP network test: NOT RUN.

Evidence:
- `reports/pa220/P5-1A.3/20-octeon-tcp-object-compile.log`
- `reports/pa220/P5-1A.3/20-octeon-tcp-object-compile.md`
- `reports/pa220/cloud-validation/21-github-actions-validation.log`
- `reports/pa220/cloud-validation/21-github-actions-validation.md`
- Actions run `44858430936`: standalone OCTEON TCP object compile.
- Actions run `44856960192`: Host regression and baseline full build.

## P5-1B exact scope

Implement only the minimal IPv4 TCP receive dispatch hook in the pinned PA-220 source:
1. Carry the standalone TCP core source/header from the integration repository into the PA-220 source through a maintainable, reproducible patch generated from actual source differences (expected next patch number 0003; inspect current patch ordering before deciding).
2. Add a narrow TCP input registration/dispatch interface. TCP dispatch is inactive unless explicitly registered.
3. In `NetReceive()`, dispatch IPv4 protocol 6 only after the existing IPv4 header/length/checksum validation, destination-address filtering, and fragment handling. Validate TCP segment bounds independently before invoking the callback.
4. Preserve ARP, ICMP, UDP, and all non-TCP behavior byte-for-byte where practical; do not replace or bypass existing handlers.
5. Add the TCP object to `net/Makefile` and update only the necessary declarations in `include/net.h` (or a dedicated header, following the existing code conventions).
6. Run the actual pinned PA-220 full build in GitHub Actions. Prove `net/tcp.o` is compiled and included in the final link; inspect ELF/map/symbol evidence and record exact source/patch hashes.
7. Commit small coherent units and create a checkpoint as soon as the dispatch hook and build integration are complete.

Explicit exclusions for P5-1B:
- No TCP Ethernet/IP output implementation.
- No MAC/neighbor resolution or routing expansion.
- No foreground `pa220 web` lifecycle or NetLoop changes.
- No HTTP parser, URI handlers, pages, upload, command execution, environment mutation, storage/CPLD writes, install, reboot, or autoboot changes.
- No P4 work and no modification of its parallel work.
- No local Debian build workflow; local machine is reserved for physical-device validation only.

## Remote repository state at checkpoint

- The current integration branch contains `u-boot/net/tcp.c` and `u-boot/include/net/tcp.h` as standalone TCP-core sources.
- The PA-220 patch directory currently contains only `0001-pa220-read-only-maintenance-command.patch` and `0002-pa220-board-clock-readonly-diagnostics.patch`.
- The target-compile workflow `.github/workflows/pa220-tcp-target-compile.yml` proves standalone object compilation only; it does not yet prove integrated final linking.
- Latest branch commits include:
  - `627871edb117b6ca9ea9c921a9b7463e6331dd02`: P5-1A to P5-1B handoff in ROADMAP.
  - `57c2e350ec3906abcdeebc2a220005b6d3d53052`: record standalone TCP target object compile evidence.
- Formal scope and sequencing: `doc/pa220/ROADMAP.md`, `doc/pa220/audit/P5-1-PA220-NATIVE-TCP-HTTP-ADAPTATION.md`, and `doc/pa220/audit/P5-1A-TCP-CORE-PROGRESS.md`.

## Immediate next action for the next session

1. Read `AGENTS.md`, current remote HEAD, and the latest P5 sections of ROADMAP/FEATURE-MATRIX/DECISIONS.
2. Inspect the exact pinned PA-220 files `u-boot/net/net.c`, `u-boot/include/net.h`, and `u-boot/net/Makefile`, plus the integration TCP source/header and any P4-related changes already present in the pinned source or current shared branch.
3. Verify the current source/patch state before editing. Do not assume an uncommitted P5-1B change exists merely because the phase has been opened.
4. Design the smallest safe dispatch ABI and implement P5-1B only. Generate patch 0003 from actual file diffs; never hand-edit or guess unified-diff hunk counts.
5. Add/adjust a dedicated GitHub Actions workflow using the existing SDK Release secret. It must apply all patches to the exact pinned source, build the full PA-220 image, prove TCP object compilation and final linkage, and save logs under `reports/pa220/P5-1B/`.
6. Before every GitHub write, reread the branch HEAD; never force-push; after each write, reread the exact changed files and resulting HEAD.
7. Keep the checkpoint current and commit progress in small units. Do not repeat P5-1A.3 tests unless P5-1B changes invalidate them.

## Safety and collaboration rules

- GitHub branch is the canonical project state. Do not ask the user to synchronize or build locally.
- The user performs only real-device validation locally.
- Do not use `git reset --hard`, `git clean -fd`, or force push.
- Preserve unrelated commits and parallel P4 work.
- Do not claim TCP dispatch or integrated linking passed until the cloud build log and final ELF/map prove it.
- RAM boot and physical TCP traffic remain NOT VERIFIED until the user performs the corresponding device test.
