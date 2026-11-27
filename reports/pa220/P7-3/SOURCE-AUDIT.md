# P7-3 Source Audit and Implementation Gate

- Date: 2026-11-27
- Fixed source: `sdzbzyh/pa220-openwrt@801717ba6a8a59ed2cfd53bf3e1b19841fecd2e8`
- Integration branch: `feature/pa220-ubootkit-web`
- Status: **SOURCE AUDIT COMPLETE; STRICT PARTITION VALIDATOR AND DRY-RUN PLAN NOT YET IMPLEMENTED**
- Scope: read-only design and source audit only; no target storage access or writes.

## 1. Existing U-Boot partition APIs are not sufficient as the P7 trust boundary

### DOS/MBR (`u-boot/disk/part_dos.c`)

The pinned implementation's `le32_to_int()` returns signed `int`, and extended-partition traversal uses `int` for sector arithmetic. Its recursive `print_partition_extended()` / `get_partition_info_extended()` traversal does not maintain a visited-EBR set or a bounded hop count. It also does not prove every logical partition stays within the original extended-container half-open range before reporting it.

This native path is useful for ordinary U-Boot partition enumeration, and the PA-220 live inventory already confirms it can enumerate the current logical partitions. It is not sufficient for stricter P7-3 requirements: 64-bit checked arithmetic, EBR cycle detection, extended-container containment, duplicate/overlap detection and fail-closed classification.

### GPT (`u-boot/disk/part_efi.c`)

The pinned implementation's `is_gpt_valid()` validates the single caller-supplied Header and its Entry Array CRC. Its callers in `print_part_efi()` and `get_partition_info_efi()` pass `GPT_PRIMARY_PARTITION_TABLE_LBA`. It does not read and cross-compare the backup Header and backup Entry Array. Header geometry checks also do not establish all P7 requirements (bounded entry count/entry size, backup placement, primary/backup field agreement, unique GUIDs, non-overlap and usable-LBA containment).

Therefore a successful native `mmc part` or `get_partition_info()` result is not a P7 GPT primary/backup consistency acceptance.

## 2. Existing PA-220 image parser boundary

- `pa220_image_parse()` parses a RAM image container and FIT preview semantics.
- `pa220_image_preview()` deliberately reports `target mapping: UNMAPPED` and `actions: NONE`.
- No approved PA-220 install manifest has been established by the SPI backup, eMMC partition inventory, FIT metadata or image load/entry addresses.
- The live eMMC evidence establishes DOS/MBR plus EBR logical partitions, but does not establish which firmware component belongs in which partition.
- The SPI backup is a 16 MiB static image; it does not prove a live eMMC GPT layout or an install mapping.

A dry-run implementation must not infer partition roles from directory names such as `images`, `releases`, `cfgdb`, `mgmt`, nor from generic Linux MBR type 0x83, FIT image names, or load/entry addresses.

## 3. P7-3 implementation split

### P7-3A — bounded read-only partition inventory core

Implement a parser independent of legacy recursive partition routines, with a caller-supplied sector-read callback and fixed-size bounded workspace. It must not call a block-write API.

Unified partition record fields: source table format (DOS/MBR or GPT), partition number/index, start LBA and sector count (64-bit), type byte or type GUID, unique GUID when present, bounded UTF-16LE GPT name, attributes and primary/logical/extended-container classification.

DOS/MBR checks: sector 0 signature and four entries; reject hybrid/protective ambiguity; validate boot indicator and empty-entry consistency; checked 64-bit start+length and device-capacity bounds; identify 0x05/0x0f/0x85 extended containers; traverse EBR links relative to extended base; visited-LBA set and strict hop limit; validate EBR signature, logical extent, link extent and container containment; reject cycles, duplicate starts, overlaps, multiple extended containers, malformed entries and read errors; never expose extended container as target.

GPT checks: validate PMBR with one 0xEE entry and no hybrid entries; primary Header at LBA 1 and backup Header at last device LBA; signature/revision/header size/reserved/header CRC/current+alternate LBA; disk GUID and usable bounds; bounded entry-array location/count/size and overflow; independently read both Entry Arrays and validate both CRCs; require critical Header fields and complete Entry Array bytes to agree; validate unique GUIDs, ranges, non-overlap and bounded names/attributes; reject unsupported geometry, duplicates, overlaps, CRC mismatch and primary/backup inconsistency.

Auto-detection: no 0x55AA => UNKNOWN/REJECT; PMBR 0xEE => strict GPT path and reject any extra MBR entries as HYBRID; ordinary MBR => strict DOS/EBR path; malformed/conflicting signatures => AMBIGUOUS/CORRUPT/REJECT; never repair or write.

### P7-3B — manifest policy and dry-run plan

The plan engine consumes only (1) a fully validated partition inventory, (2) parsed image components with source offset/length and integrity status, and (3) an explicit, reviewed PA-220 install manifest with provenance and component-to-target rules.

Until an approved manifest exists, every component remains UNMAPPED and the report must hard-reject with `NO_APPROVED_MANIFEST`. A syntactically valid partition table alone is not authority to select a target.

A report includes source format, disk identity/capacity, component source offsets/lengths, integrity status, target partition/range only if manifest-resolved, target capacity, overlap/overflow checks, unresolved evidence and final decision. No executable write script or write callback is emitted.

## 4. Host test matrix required before target integration

- MBR: invalid signature, valid primary entries, actual PA-220 table, EBR logical entries, all extended types, EBR self-loop and multi-node cycle, EBR/logical extent outside container, overlaps, duplicate extended container, arithmetic overflow, out-of-device range, malformed boot flag and read failure.
- GPT: valid primary+backup pair, bad primary/backup Header CRC, bad primary/backup Entry CRC, incorrect current/alternate LBA, bad usable bounds, entry-array overflow/out-of-device range, unsupported count/entry size, hybrid PMBR, duplicate unique GUID, overlapping/out-of-range partitions, mismatched disk GUID, mismatched entry arrays and read failure.
- Policy: no manifest always rejects mapping; unknown component, unsupported external FIT data, failed hash, unverified signature where policy requires it, target capacity shortfall, arithmetic overflow and component target overlap all reject.
- Run GCC plus ASan/UBSan; then Cavium GCC 4.7.0 target compile, N32 link and complete U-Boot final link. Only after those gates may a read-only live partition inventory be considered.

## 5. Current acceptance statement

P7-2A/B/C remain accepted. P7-3 is **not accepted**. This audit adds no install mapping, does not validate GPT on the live PA-220 (which is MBR), and authorizes no storage writes or environment persistence.