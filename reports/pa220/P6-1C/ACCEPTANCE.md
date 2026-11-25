# P6-1C Acceptance — receive-time SHA-256/CRC32 and read-only format recognition

Date: 2026-11-25  
Integration repository: sdzbzyh/uboot-qsdk12.5-build  
Branch: feature/pa220-ubootkit-web  
Validation commit: b366a278ec15445a60f12441739d172b94392e3e  
Fixed PA-220 source: sdzbzyh/pa220-openwrt@801717ba6a8a59ed2cfd53bf3e1b19841fecd2e8

**SOFTWARE ACCEPTED**

## 1. Scope delivered

P6-1C is implemented only on the TFTP GET receive-completion path:

- After the final DATA block, flush the received DRAM range with flush_dcache_range() before reading it for verification.
- Calculate CRC32 using the pinned U-Boot crc32_wd() implementation.
- Calculate SHA-256 using the pinned U-Boot sha256_csum_wd() implementation.
- Print received length, recognized format, CRC32 and SHA-256.
- Perform read-only, bounded format recognition for legacy uImage, FIT/FDT, ELF32 and ELF64.
- Validate fixed header lengths, declared payload/total sizes, FDT block ranges, ELF program/section table ranges and checked arithmetic before accepting.
- TftpGetState can become COMPLETE only after the receive-state/length/reservation checks, both digest calculations and format recognition succeed.
- Unsupported or malformed input calls the existing GET failure path, enters FAILED, clears NetBootFileXferSize and invalidates filesize/fileaddr metadata.
- No image loader is called. No data is executed or written to persistent storage.

TFTP PUT and Server/WRQ behavior were not changed. No saveenv call was added.

## 2. Host verification

Actions [#45600860179](https://github.com/sdzbzyh/uboot-qsdk12.5-build/actions/runs/45600860179) — **SUCCESS**.

The Host Harness injects the exact GET_STATE_HELPERS, P6-1C verifier helpers and verified completion gate exported by the production patch generators.

| Host mode | Cases | Assertions | Failures |
|---|---:|---:|---:|
| GCC | 17 | 31 | 0 |
| ASan/UBSan | 17 | 31 | 0 |

Coverage includes:

- legacy uImage success, truncated header and declared payload mismatch;
- FIT/FDT success, total-size overflow and structure-range overflow;
- ELF32 and ELF64 success, truncated identification, program-table overflow and unsupported ELF class;
- unsupported magic;
- CRC32 IEEE reference vector 123456789;
- successful completion, cache synchronization and SHA-256 call;
- unsupported-format failure, FAILED state and metadata invalidation;
- reservation-length failure cannot publish a valid file;
- P6-1B reservation, block commit and bounded tsize parser remain covered by the same harness.

**Host SHA boundary:** the Host test stubs sha256_csum_wd() to assert that the production completion gate invokes it with the received buffer and length. The cryptographic implementation itself is the pinned U-Boot library and is compiled/linked in the OCTEON target build; this Host Harness does not claim an independent known-answer test of that library.

## 3. OCTEON and complete target build

- SDK: Cavium OCTEON SDK 5.1.0, GCC 4.7.0.
- SDK archive SHA-256: cd8cd3232078094c89accf32a82a13e7847e3466c94e9b5e12dd11b741102d99.
- net/tftp.o: ELF32, big-endian, MIPS N32 / ABI2 / OCTEON; compile PASS.
- Standalone N32 relocatable link: PASS.
- Complete u-boot-octeon_pa220 final link: PASS.
- u-boot-octeon_pa220.bin generation and OCTEON image-header generation: PASS.
- Generator second-run check: P6-1C_ALREADY_GENERATED=verified.
- Patch generation uses git diff --binary; no hand-authored or manually counted unified-diff hunks.

### Artifact SHA-256

| Artifact | SHA-256 |
|---|---|
| 0006-pa220-tftp-get-verification.patch | 34941096eef9983fe7f951bb29c88e88e31f7d29d330ea4118fd83fcf5a9b32b |
| tftp.o | bec5e494f7e5efee894fa23e902ead5894c468fc40159e25569daeeec549d433 |
| cmd_net.o | 4412f25a65989c4007655eb446ca91df53801a905596ef78d1ecea108b12820a |
| tftp-n32-relocatable.o | 113774a56705ac6f9baca4592cf3d7fe3ccb33d2256ae92753c1b7c1387cf8c2 |
| u-boot-octeon_pa220 | e4501aa273442bdf961b078a23afbeb4d12fed578f6241bd87b4c03e997a9472 |
| u-boot-octeon_pa220.map | b1d1bc2c1ecece940ab0ad1429c887bfe5fd2863222fe3b82246b7e3019acd2c |
| System-octeon_pa220.map | 75df00959c988645229846a44171d90409d1d5826464ced400289103bffbd0fb |
| u-boot-octeon_pa220.bin | fc7649674c13b3e1c3bde21e482b96042b7e0f1d87c3743320b4ba5d7ac65b51 |

The build log contains the PA-220 baseline's existing configuration macro-redefinition warnings. The P6-1C TFTP object compiled, the N32 relocatable link succeeded and the full target final link succeeded.

## 4. Validation artifact

- Actions run: [45600860179](https://github.com/sdzbzyh/uboot-qsdk12.5-build/actions/runs/45600860179)
- Artifact ID: 15015526236
- Artifact: [pa220-p6-1c-tftp-verify-b366a278ec15445a60f12441739d172b94392e3e](https://github.com/sdzbzyh/uboot-qsdk12.5-build/actions/runs/45600860179/artifacts/15015526236)
- Artifact ZIP SHA-256: 9164f94cbaf22f50dd96c220d839bf3c1e1a9acaf4d0c0e8136bb77ce02504b4

The artifact contains the generated patch, Host logs, SDK identity/hash, generator idempotence output, full target build log, ELF headers, N32 relocatable object, tftp.o, cmd_net.o, final U-Boot ELF/map files and final BIN.

## 5. Checksum semantics and limitations

The TFTP receive path computes and reports CRC32 and SHA-256 over the received bytes. TFTP does not provide an authenticated expected digest in this flow, so these values are fingerprints for comparison with a trusted host-side value; they are not, by themselves, a cryptographic authenticity decision. The current completion gate rejects length/state failures and malformed or unsupported file formats. It does not claim to validate image signatures, uImage header/data CRC fields, FIT signatures or ELF load-segment semantics.

The format recognizer is intentionally a bounded header recognizer, not a loader or a complete semantic validator. A future boot/execute path would require a separate security review.

## 6. Not performed; next stage

- No real TFTP GET, no file transfer and no live RAM write.
- No TFTP PUT, Server/WRQ, HTTP upload or multipart behavior.
- No saveenv, flash/eMMC/NAND/SPI/CPLD/EEPROM write, erase, reset or reboot.
- P6-A has not yet been performed.

**Next work unit: P6-A** — read-only inspection of the live __tmp_load descriptor, DRAM range and complete reservation feasibility. Only after P6-A is reviewed may a separate user-confirmed RAM-only TFTP GET test be considered. At the end of each device test round, power the device off.
