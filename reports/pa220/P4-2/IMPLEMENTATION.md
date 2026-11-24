# P4-2 Unified RAM Write-Range Validator — Implementation Record

Date: 2026-11-24
Integration repository: sdzbzyh/uboot-qsdk12.5-build
Branch baseline: feature/pa220-ubootkit-web
Pre-change HEAD: 4168cbd5fa285e8a93a40e195e7bff09ab965024
Fixed PA-220 source: IronDemiGod/pa220-openwrt@801717ba6a8a59ed2cfd53bf3e1b19841fecd2e8

## Scope

Implemented the independent production validator interface:

    int octeon_load_range_check(uint64_t addr, uint64_t len);

No TFTP, HTTP upload, ELF loader, DHCP, Ping, or persistent-storage behavior was enabled or tested in this change.

## Production implementation

- Source: u-boot/arch/mips/cpu/octeon/octeon_load_range.c
- Header: u-boot/arch/mips/include/asm/arch-octeon/octeon_load_range.h
- The only writable interval is obtained from cvmx_bootmem_find_named_block(OCTEON_BOOTLOADER_LOAD_MEM_NAME), where the fixed source defines the name as __tmp_load.
- The API returns a pointer to a function-static, non-reentrant descriptor snapshot. The validator copies base, size and name immediately, then obtains a second snapshot and compares it.
- The descriptor must have the expected name, nonzero base/size and a non-overflowing end. load_reserved_addr/load_reserved_size are consistency checks only.
- __uboot_code_data must match gd->bd->bi_uboot_ram_addr/bi_uboot_ram_used_size. This reservation includes U-Boot code/data, malloc, stack, board/global data and boot parameters.
- The complete candidate load block is rejected if it overlaps the fixed low 1 MiB or any present known reserved named block: Linux reserved, U-Boot exception, FDT, bootloader environment, low reserved, MAC memory info or idle-core loop.
- Address normalization accepts the PA-220 identity/physical form, KSEG0/KSEG1 aliases and XKPHYS CCA aliases. Unsupported 64-bit virtual/IO forms, KSEG segment crossings and XKPHYS MBZ bits are rejected.
- All range decisions use checked arithmetic and half-open intervals. No write occurs in the validator.

## Host Harness

The Host Harness compiles the production C file itself with mocked platform dependencies.

    HOST RAM RANGE: cases=32 assertions=121 failures=0 writes=1

- GCC C99, -Wall -Wextra -Werror: PASS
- AddressSanitizer + UndefinedBehaviorSanitizer: PASS
- Rejected requests: guarded destination unchanged and write counter unchanged.
- Covered descriptor missing/invalid/name mismatch/change, globals mismatch, boundary equality, overflow/wrap, zero length, alias normalization, KSEG crossing, reserved overlap and U-Boot reservation mismatch/overlap.

## Source-derived safety evidence

- cvmx_bootmem_find_named_block() returns a pointer to a function-static descriptor object; the source documents that it is not re-entrant.
- board_octeon.c allocates the U-Boot physical reservation as named block __uboot_code_data using the live board-info base and used size. This range covers code/data, malloc, stack, bd, gd and boot parameters.
- The fixed memory map reserves the low 1 MiB for bootloader structures, FDT/exception/bootmem metadata.
- __tmp_load is allocated through the bootmem named-block allocator; the Linux reserved block is allocated separately through the same allocator.
- For the default 8 GiB PA-220 configuration, source formulas yield the default load reservation physical interval [0x20000000, 0x30000000); this value is documentation only and is not hard-coded into the validator.
- KSEG0 alias 0x81000000 normalizes to physical 0x01000000, which is outside the default load window and inside the default Linux-reserved region.

### Descriptor concurrency limitation

The fixed API performs the named-block lookup under its internal lock, releases that lock, then copies fields into its static snapshot. The validator therefore does not claim an atomic snapshot against arbitrary concurrent bootmem writers. The current U-Boot foreground command model is serialized; secondary cores are brought to an idle loop and do not run bootmem writers. If asynchronous/multi-core bootmem mutation is introduced, this implementation must be replaced with a lock-protected snapshot API.

## Explicitly BLOCKED

- TFTP store_block() still has no call to the validator before its RAM memcpy.
- HTTP remains read-only; no payload reservation or chunk writer exists.
- OCTEON ELF64 PT_LOAD and section fallback still write without calling the validator. Its separate little-endian tail-width issue remains for ELF hardening.
- No receive path was enabled or tested.
- DHCP Client and Ping hardware verification remain incomplete.

## Target build

A dedicated GitHub Actions workflow has been added to run the production Host Harness, download and checksum the pinned Cavium OCTEON SDK 5.1.0, copy the production validator into the fixed PA-220 source checkout, generate the OCTEON Makefile diff using git diff against the actual pinned source, build the validator object and complete PA-220 U-Boot, and inspect N32/ELF ABI, undefined symbols, object size, final ELF, map, System.map and SHA-256.

Target build and final-link status are PENDING Actions execution at the time of this report. This report does not claim target PASS.

## Device safety

PA-220 remains powered off. No saveenv, SPI/eMMC/NAND/CPLD/EEPROM writes, flash, reset, reboot, power-cycle or file-receive test was performed.
