# P4-2 Unified RAM Write-Range Validator — Implementation Record

Date: 2026-11-24
Integration repository: `sdzbzyh/uboot-qsdk12.5-build`
Branch: `feature/pa220-ubootkit-web`
Verified integration commit: `908a58f43017f3e252b3ce0c613b4630072203e9`
Validator implementation baseline: `fce0485d1f1eaeeac5cc1732d5822077cfa68a20`
Fixed PA-220 source: `sdzbzyh/pa220-openwrt@801717ba6a8a59ed2cfd53bf3e1b19841fecd2e8`

## Status

**P4-2 validator Host and OCTEON build: PASS. TFTP DATA pre-write guard Host and OCTEON compile/link: PASS. Actual TFTP receive: NOT RUN. P4 overall: IN PROGRESS. P6: NOT STARTED.**

The current result supersedes the earlier 32/121 Host result and the earlier `PENDING Actions execution` text. The authoritative validation is GitHub Actions run [#45374041373](https://github.com/sdzbzyh/uboot-qsdk12.5-build/actions/runs/45374041373), checked out at integration commit `908a58f43017f3e252b3ce0c613b4630072203e9`.

## Production validator

Public interface:

```c
int octeon_load_range_check(uint64_t addr, uint64_t len);
```

Sources:
- `u-boot/arch/mips/cpu/octeon/octeon_load_range.c`
- `u-boot/arch/mips/include/asm/arch-octeon/octeon_load_range.h`

Implementation properties:
- The authoritative writable interval is queried at runtime from `cvmx_bootmem_find_named_block(OCTEON_BOOTLOADER_LOAD_MEM_NAME)`, whose fixed-source name is `__tmp_load`.
- Descriptor base, size and name are copied immediately into a function-static snapshot; a second lookup is compared to detect a changed descriptor.
- Descriptor name, nonzero base/size, checked end arithmetic and consistency with `load_reserved_addr/load_reserved_size` are validated.
- `__uboot_code_data` must agree with `gd->bd->bi_uboot_ram_addr/bi_uboot_ram_used_size`; this reservation covers U-Boot code/data, malloc, stack, board/global data and boot parameters.
- The load window is rejected if it overlaps the low 1 MiB or a present known reserved named block: Linux reserved, U-Boot exception, FDT, bootloader environment, low reserved, MAC memory info or idle-core loop.
- 32-bit identity/physical, KSEG0/KSEG1 and XKPHYS CCA aliases are normalized. Unsupported 64-bit virtual/IO forms, KSEG segment crossings and XKPHYS MBZ bits are rejected.
- Checked arithmetic and half-open intervals are used. Zero length, overflow, wraparound and out-of-window ranges are rejected. The validator itself performs no write.

### Descriptor concurrency limitation

The fixed API performs the named-block lookup under its internal lock, releases that lock, then copies fields into its static snapshot. The validator does not claim an atomic snapshot against arbitrary concurrent bootmem writers. It relies on the current foreground U-Boot command model being serialized and secondary cores remaining idle. If asynchronous or multi-core bootmem mutation is introduced, this must be replaced by a lock-protected snapshot API.

## Host validation

### Validator Host Harness

`tests/pa220/ram-range/run.sh` compiles the production validator C source itself against mocked platform dependencies.

- Normal GCC run: `cases=42 assertions=161 failures=0`
- ASan + UBSan run: `cases=42 assertions=161 failures=0`
- Both outputs are asserted by the workflow.
- Rejected ranges leave the guarded destination unchanged and do not increment the write count.

### TFTP DATA pre-write Host Harness

`tests/pa220/tftp-guard/run.sh` imports the `STORE_BLOCK` function text from `scripts/generate-pa220-load-range-makefile-patch.py`, inserts that exact production function into a minimal host harness, and compiles/runs it with a mocked validator and intercepted `memcpy`.

- Normal GCC run: `cases=14 assertions=64 failures=0`
- ASan + UBSan run: `cases=14 assertions=64 failures=0`
- Covered: valid first block, exact exclusive-end boundary, wrapped block offset, zero-length terminal block, below-window, cross-end, oversized payload, zero block size, invalid negative block, wrap underflow, target-width overflow, validator rejection and maximum Ethernet block size.
- The harness proves that rejected DATA blocks do not reach the intercepted copy. It is a focused function-level harness; it does not emulate a complete TFTP UDP session or the entire `TftpHandler`.

## TFTP production integration and target evidence

The generator `scripts/generate-pa220-load-range-makefile-patch.py`:
1. Verifies the exact fixed PA-220 source commit.
2. Adds the validator header to the fixed source `u-boot/net/tftp.c`.
3. Replaces `store_block()` with a CN70XX-specific path that checks block-size validity, block index multiplication, wrap offset, destination addition and target `ulong` width before calling `octeon_load_range_check(dest64, len)`.
4. Calls the validator before the RAM `memcpy`; an error sets `NETLOOP_FAIL` and returns without copying.
5. Adds checked block-wrap arithmetic and updates the two wrap call sites and the DATA store call site with fail-closed handling.
6. Generates the TFTP patch using `git diff --binary`, not hand-edited hunk counts.

The generator modifies only the disposable PA-220 checkout created by `build-pa220.sh`; the generated patch is retained in the Actions artifact, not committed as a standalone patch under `patches/pa220/`.

Run #45374041373 confirms:
- The generated TFTP patch SHA-256 is `464e54a49f7a4b761c743280faa35a0c7bcfbe8b7124015385c90eea6fc30e38`.
- The generated Makefile patch SHA-256 is `7de64ed7e0cd5e6f16761f1d1ad0bd60dafde929bbf8384d69b75e722cce751d`.
- The modified production `tftp.c` compiled as `tftp.o`; `tftp.o` was included in `libnet.o`.
- The production validator object is ELF32, big-endian, N32 ABI, OCTEON; object text size is 1,440 bytes.
- Standalone N32 relocatable link passed.
- Complete PA-220 U-Boot final link passed; validator symbol was checked in the final ELF, map and System.map.
- SDK 5.1.0 GCC 4.7.0 archive checksum verification passed.

Actions:
- [Validator-only baseline run #45346000355](https://github.com/sdzbzyh/uboot-qsdk12.5-build/actions/runs/45346000355), commit `fce0485d1f1eaeeac5cc1732d5822077cfa68a20`.
- [Validator + TFTP guard Host and OCTEON build run #45374041373](https://github.com/sdzbzyh/uboot-qsdk12.5-build/actions/runs/45374041373), commit `908a58f43017f3e252b3ce0c613b4630072203e9`.
- Latest artifact ID: `14909658286`; name `pa220-p4-2-ram-validator-908a58f43017f3e252b3ce0c613b4630072203e9`; ZIP SHA-256 `d3cd880db8be404327a9fc953ea096a8362145bacbd52541e9ad02e77d7ab6d0`.

### Fixed-source TFTP path inventory

A recursive tree check at the pinned PA-220 source commit found only three TFTP-related files:
- `u-boot/net/tftp.c`: protocol implementation.
- `u-boot/net/tftp.h`: protocol declarations.
- `u-boot/arch/mips/cpu/octeon/commands/cmd_octeon_tftp.c`: the OCTEON `tftp` command is only an alias for `do_tftpb`; it contains no separate receive implementation.

Within `u-boot/net/tftp.c`:
- The only receive-side RAM copy is the `memcpy((void *)(load_addr + offset), src, len)` inside `store_block()`. The DATA handler has exactly one `store_block(TftpBlock - 1, pkt + 2, len)` call.
- TFTP GET/RRQ and TFTP Server/WRQ both deliver DATA through the same `TFTP_DATA` handler and therefore the same guarded `store_block()` call. Multicast receive also shares this store function.
- Both `update_block_number()` call sites are rewritten to stop processing when checked block-wrap arithmetic fails.
- TFTP PUT uses `load_block()` to copy bytes from local `save_addr` into an outbound packet. That is a RAM read, not a receive-side RAM write; source-read confidentiality/range protection is not covered by this validator. PUT remains outside this acceptance.
- The optional `CONFIG_SYS_DIRECT_FLASH_TFTP` branch contains `flash_write()`. The generated CN70XX guard validates the destination against the live RAM reservation before either the RAM copy or this branch, so non-RAM destinations fail closed. The direct-flash configuration and flash path were not runtime-tested; persistent writes remain prohibited.

This inventory is source-level evidence at the fixed commit, not proof of runtime protocol behavior.

## Explicitly not validated / still blocked

- No PA-220 runtime `__tmp_load` descriptor re-check has been performed after this build.
- No TFTP GET, TFTP Server/WRQ, or TFTP PUT transfer was run. No actual DATA packet was received and no live RAM write was attempted.
- No HTTP upload/payload/chunk writer exists; HTTP remains read-only.
- OCTEON ELF64 PT_LOAD and section-fallback writes are not guarded by this validator; the separate ELF64 tail-width issue remains.
- DHCP Client and Ping runtime tests remain incomplete.
- PA-220 remains powered off. No saveenv, SPI/eMMC/NAND/CPLD/EEPROM write, flash, reset, reboot, power-cycle or device receive test was performed.

## Next stage gate

P4-2's code/Host/target-build gates are now satisfied. This does **not** authorize a real receive test. Before P6 RAM-only receive acceptance:
1. Perform read-only PA-220 boot/runtime checks and confirm the live `__tmp_load` descriptor, its aliases and reserved-block separation.
2. Select one receive path and define a bounded RAM-only test plan, file size, checksum and failure cleanup behavior.
3. Keep HTTP upload and ELF writes disabled until their own pre-write reservations and tests are implemented.
4. Do not perform persistent-storage writes. Any such operation remains a separate stage requiring explicit review and authorization.
