# PA-220 P6-0 Source Review and RAM Preflight Preparation

Date: 2026-11-24
Repository: `sdzbzyh/uboot-qsdk12.5-build`
Branch: `feature/pa220-ubootkit-web`
Branch HEAD at review start: `1a318fd562cdb3b475d92abdc5768b436cf154fd`
P4-2 verified code/build commit: `20acf9790b6ee15ed758db7006eaee69af0f2bf2`
Fixed PA-220 source: `sdzbzyh/pa220-openwrt@801717ba6a8a59ed2cfd53bf3e1b19841fecd2e8`
Fixed uBootKit audit source: `chenxin527/uboot-qsdk12.5-build@9ce315735ac88fd0a7c7697bc527a8616ed0a972`

## 1. Git checkpoint recovery

- The branch ref resolves to `1a318fd562cdb3b475d92abdc5768b436cf154fd`.
- User's earlier `git ls-remote` value `4168cbd5fa285e8a93a40e195e7bff09ab965024` is an ancestor, not current HEAD.
- Branch comparison: current HEAD is 18 commits ahead of `4168cbd5...`.
- `20acf979...` is the P4-2 verified code/build commit. Current HEAD is one documentation-only commit after it.
- The one-commit delta from `20acf979...` to `1a318fd562cdb3b475d92abdc5768b436cf154fd` updates `doc/pa220/P6-PLAN.md` and `reports/pa220/P4-2/IMPLEMENTATION.md`; no production source changed in that delta.
- The current execution container has no local clone of this repository. The user's Debian working-tree status cannot be asserted from this session; remote branch state was checked through GitHub.

## 2. Formal documents reviewed

Read the current branch versions of:
- `doc/pa220/P6-PLAN.md`
- `doc/pa220/README.md`
- `doc/pa220/ROADMAP.md`
- `doc/pa220/TEST-PLAN.md`
- `doc/pa220/FEATURE-MATRIX.md`
- `doc/pa220/DECISIONS.md`
- `reports/pa220/P4-2/IMPLEMENTATION.md`
- `reports/pa220/P5-2C/ACCEPTANCE.md`

The README and P6 plan correctly describe P6 as design/preflight preparation and actual receive as not run. Historical sections in ROADMAP, TEST-PLAN, FEATURE-MATRIX and DECISIONS still contain earlier P4-2/P6 statements (including 32/121 Host results and target build pending). Later records supersede those statements, but a current-state overlay is needed to prevent stale text being mistaken for the active state.

## 3. Validator source review

Reviewed:
- `u-boot/arch/mips/cpu/octeon/octeon_load_range.c`
- `u-boot/arch/mips/include/asm/arch-octeon/octeon_load_range.h`
- `scripts/generate-pa220-load-range-makefile-patch.py`

Confirmed in current branch source:
- `__tmp_load` is queried at runtime; the returned static descriptor fields are copied immediately.
- The descriptor is queried twice and base/size/name snapshots are compared.
- Zero base, zero size, checked-end overflow, non-DRAM range and descriptor/global mismatch fail closed.
- `load_reserved_addr` / `load_reserved_size` are consistency checks, not the authority.
- `__uboot_code_data` must exactly match `gd->bd->bi_uboot_ram_addr` and `bi_uboot_ram_used_size`.
- The complete `__tmp_load` reservation is checked against low reserved memory and known named reservations: `__tmp_reserved_linux`, `__tmp_uboot_exception`, `__fdt`, `__bootloader_env`, `__low_reserved`, `__mac_meminfo`, and `idle-core-loop`.
- Address normalization covers 32-bit physical/identity, KSEG0/KSEG1 and XKPHYS CCA aliases; unsupported forms, alias crossings and reserved XKPHYS bits are rejected.
- Checked arithmetic and half-open intervals are used; zero-length writes and out-of-range spans are rejected.

Concurrency qualification: the two reads are not an atomic lock-protected snapshot. The implementation relies on the current foreground single-core U-Boot model and idle secondary cores, with no concurrent bootmem writer. This assumption must remain explicit.

No validator defect was established by this source review. This is not a replacement for the already-passed P4-2 Host/target validation and does not constitute live descriptor validation.

## 4. Fixed-source TFTP receive-path review

Read the fixed-source files at `801717ba6a8a59ed2cfd53bf3e1b19841fecd2e8`:
- `u-boot/net/tftp.c`
- `u-boot/net/tftp.h`
- `u-boot/arch/mips/cpu/octeon/commands/cmd_octeon_tftp.c`

Confirmed:
- GET/RRQ and Server/WRQ DATA packets converge on the `TFTP_DATA` handler.
- `store_block()` is the sole receive-side RAM copy site.
- The generated CN70XX replacement checks block size, block index multiplication, wrap offset, destination addition and target `ulong` width before calling `octeon_load_range_check()`.
- The guard is called before the RAM `memcpy`; rejection sets `NETLOOP_FAIL` and returns without copying that DATA block.
- Block-wrap overflow is checked and the two call sites stop processing on error.
- `CONFIG_SYS_DIRECT_FLASH_TFTP` contains a `flash_write()` branch, but the RAM validator must pass first; the flash branch is not runtime-tested and remains prohibited for this project.
- TFTP PUT uses `load_block()` to read from local `save_addr` into an outbound packet. It is not protected by the receive-side guard and is outside P6 GET acceptance.

The TFTP Host Harness imports the exact `STORE_BLOCK` template from the generator and intercepts memcpy. It is a focused function-level test, not a complete UDP/TFTP session simulation.

## 5. Build and generator review

Reviewed `build-pa220.sh`, the generator and `.github/workflows/pa220-p4-2-ram-validator.yml`.

- The independent `./build-pa220.sh` path remains separate from Qualcomm `build.sh`.
- The build script checks the exact PA-220 source pin, applies PA-220 patches, stages the validator source/header into the disposable source checkout, then runs the generator.
- The generator verifies the fixed source revision and produces patches with `git diff --binary`; it does not hand-count unified-diff hunk lengths.
- The generator modifies the disposable PA-220 checkout in place and writes the generated patch files into the P4-2 report directory.
- The workflow tests the production validator source and the generator's exact `STORE_BLOCK` function template, downloads/verifies the pinned SDK, builds the OCTEON object and complete U-Boot, inspects ELF/map/symbol evidence, and reruns the generator to check idempotence.

The workflow file itself is not treated as evidence of success; the actual Actions jobs and artifact metadata were checked.

## 6. Actions and artifact evidence

| Run | Result | Scope |
|---|---|---|
| `45346000355` | completed / success | Validator baseline |
| `45374041373` | completed / success | Validator + TFTP guard Host and target build |
| `45375848020` | completed / success | Full validation plus generator idempotence |

Latest run `45375848020` has a completed successful job `validator-host-and-octeon`; all reported steps completed successfully.

Host results recorded by the P4-2 report:
- Validator: 42 scenarios / 161 assertions / 0 failures; GCC and ASan/UBSan PASS.
- TFTP DATA guard: 14 scenarios / 64 assertions / 0 failures; GCC and ASan/UBSan PASS.

Target evidence recorded by the report and run:
- Validator object: ELF32 big-endian N32 OCTEON, text size 1,440 bytes.
- N32 relocatable link: PASS.
- Modified `tftp.o` compiled and included in `libnet.o`.
- Complete PA-220 U-Boot final link: PASS.
- Final ELF/map/System.map symbol checks: PASS.
- Generator idempotence: PASS.

Latest artifact:
- ID: `14910393863`
- Name: `pa220-p4-2-ram-validator-20acf9790b6ee15ed758db7006eaee69af0f2bf2`
- ZIP SHA-256: `8003d3160733133269efd05ee74bc5eecc7decdbd70b4f8855bd1e15cc8f27f1`

No P4-2 test was rerun in this review.

## 7. Read-only device preflight

The existing `patches/pa220/0001-pa220-read-only-maintenance-command.patch` implements `pa220 memory`. Its `pa220_print_memory_info()`:
- reads `loadaddr` and legacy load-reservation environment values with `getenv()`;
- calls `cvmx_bootmem_find_named_block("__tmp_load")`;
- copies `base_addr` and `size` immediately;
- prints the descriptor base and size;
- does not allocate/free a named block, write environment, touch CPLD, or write persistent storage.

This is the preferred direct descriptor display. `namedprint` and `freeprint` remain useful for cross-checking the broader named/free-block map. The descriptor query uses an internal static scratch descriptor but does not mutate the named-block allocation table.

Minimal serial preflight, after the user chooses to power the device:
```text
version
bdinfo
printenv autoload loadaddr ipaddr serverip ethact ethprime
pa220 memory
namedprint
freeprint
```

All commands above are intended as read-only observations. Capture the complete raw UART output. Do not append `tftpboot`, `tftpsrv`, `tftpput`, `setenv`, `saveenv`, reset or reboot commands to this preflight.

The device was reported powered off. No power-on is requested by this report.

## 8. Deterministic first-file test vector (design only)

Proposed plain-text payload: 4,096 bytes, formed by repeating the 23-byte ASCII string `PA220-P6-TFTP-GET-TEST\n` and truncating to exactly 4,096 bytes.

- Length: 4,096 bytes.
- CRC32 (IEEE): `e32b0c57`.
- SHA-256: `dac1fcd877c5ef356b2cdd44f6202f154a2d393cd458975b521ef6cfc05b8356`.
- Planned block size: 512 bytes, to be confirmed from the negotiated TFTP block size before DATA acceptance.
- Expected DATA sequence for an exact 4,096-byte file at 512-byte blocks: eight 512-byte DATA payloads, followed by a ninth zero-length terminal DATA packet.
- Expected completed file size: 4,096 bytes.

The payload is ordinary ASCII test data, not a firmware image or executable. This vector is a proposed test fixture only; it has not been transferred.

Destination must be selected from the live `__tmp_load` descriptor. Let descriptor base/size be B/S and the normalized physical target be T. Before transfer require T >= B and 4096 <= S - (T - B), with checked subtraction/addition, and require the complete half-open interval [T, T+4096) to pass the production validator. Do not assume `0x81000000`, `0x20000000`, or any historical address is safe. If the existing `loadaddr` is not inside the live reservation, stop; any volatile load-address adjustment requires a separately reviewed post-preflight step and must never be saved.

After successful receive, synchronize RAM writes using the PA-220 data-write synchronization semantics (`flush_dcache_range()` / `syncw`) before checksum/readback. Do not use per-block global `flush_cache()`; executable-code I-cache handling is a separate concern.

## 9. P6 gate and failure-state requirements

The P4-2 guard only protects each DATA block before that block's write. It does not yet provide a complete-file reservation, SHA-256 acceptance state, format recognition, or an explicit committed/valid file lifecycle.

Before the first real GET:
1. Complete P6 software design/implementation for bounded total length and full-file reservation.
2. Test success, exact boundary, over-limit rejection, wrap arithmetic, timeout/interruption and malformed/unsupported file paths.
3. Ensure partial data is never marked as a completed/valid file; clear or invalidate the transfer result on every failure path.
4. Compute and compare the expected final length, CRC32 and SHA-256.
5. Add read-only magic/header classification only; never execute the received file.
6. Confirm live descriptor and all reserved-block relationships on the device.
7. Obtain explicit user confirmation to begin the RAM-only transfer.

Therefore the real TFTP GET is **NOT AUTHORIZED / NOT RUN** at this checkpoint. No TFTP Server/WRQ, PUT, HTTP upload, ELF segment write, or persistent-storage operation is authorized.

## 10. Current status

- P4-2 validator production code: implemented.
- Validator Host: PASS.
- TFTP DATA guard: implemented.
- TFTP guard Host: PASS.
- OCTEON object / N32 link / full U-Boot link: PASS.
- P4-2 generator idempotence: PASS.
- P5-2C: previously accepted; no retest performed.
- Live `__tmp_load` descriptor: NOT RUN.
- TFTP GET actual receive: NOT RUN.
- TFTP Server/WRQ and PUT: NOT RUN.
- HTTP upload and ELF PT_LOAD/section protection: BLOCKED.
- DHCP/Ping physical verification: NOT RUN.
- Device: reported powered off.
- Persistent writes: prohibited.


## 11. Additional P6 finding: stale TFTP result metadata on failure

A follow-up trace of the fixed-source command/network lifecycle found a P6-specific failure-state gap:

- `u-boot/common/cmd_net.c::do_tftpb()` dispatches GET through `netboot_common(TFTPGET, ...)`.
- `u-boot/net/net.c::NetLoop()` writes `filesize` and `fileaddr` only in the `NETLOOP_SUCCESS` case, when `NetBootFileXferSize > 0`.
- The `NETLOOP_FAIL` case runs `net_cleanup_loop()`, invalidates the last protocol and returns, but does not clear prior `filesize` / `fileaddr` values.
- Therefore, if a previous transfer succeeded and a later GET writes some DATA blocks but then fails (for example on a later out-of-range block or timeout), the old metadata pair can remain while the buffer contains a partial new file.

This is not a failure of the P4-2 per-block range check; it is an unimplemented P6 transfer-result lifecycle requirement. It must be fixed before the first real GET: invalidate the volatile `filesize` and `fileaddr` result markers before a GET can write any DATA, fail closed if invalidation cannot be completed, and only let the normal successful completion path publish new metadata. Do not call `saveenv`; this must remain an in-RAM environment update only. The host test must cover both old markers present, successful invalidation, each individual invalidation failure, and confirmation that both deletion attempts occur. This fix and its target integration are **NOT IMPLEMENTED / NOT VERIFIED** at this checkpoint.
