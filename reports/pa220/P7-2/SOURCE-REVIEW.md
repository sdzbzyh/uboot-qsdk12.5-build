# P7-2 source review: FIT semantics, SPI update path and eMMC DOS/MBR reader

- Date: 2026-11-27
- Fixed PA-220 source: `sdzbzyh/pa220-openwrt@801717ba6a8a59ed2cfd53bf3e1b19841fecd2e8`
- Integration branch: `feature/pa220-ubootkit-web`
- Scope: `image-fit.c`, `libfdt`, PA-220 configuration, `cmd_octeon_bootloader.c`, `part_efi.c`, `cmd_gpt.c`, `env_sf.c`.
- Status: **SOURCE REVIEW COMPLETE; P7-2C LIVE DOS/MBR INVENTORY ACCEPTED; P7-2B FIT PREVIEW IMPLEMENTED, VALIDATION PENDING**.

## 1. FIT is enabled, but FIT signature verification is not enabled in the reviewed board configuration

- `octeon_common.h` enables `CONFIG_FIT` and defaults `CONFIG_OF_LIBFDT` / `CONFIG_OF_CONTROL` on.
- `image.h` enables CRC32, MD5 and SHA1 hash calculation when `CONFIG_FIT` is set.
- `CONFIG_FIT_SIGNATURE` was not found in the PA-220 board, common or command configuration headers reviewed. `image.h` defines `IMAGE_ENABLE_VERIFY=0` when `CONFIG_FIT_SIGNATURE` is absent. Therefore the current pinned configuration does not establish FIT signature verification or trusted-key policy.
- The target build sets `IMAGE_ENABLE_IGNORE=1`; the legacy U-Boot hash verifier may skip a hash node carrying `uboot-ignore`. A maintenance preview must report that state explicitly and must not silently describe an ignored hash as verified.
- `fit_image_verify()` obtains image data and iterates `hash*` child nodes, recomputing the declared algorithm and comparing the stored value. It only processes `signature*` child nodes when `IMAGE_ENABLE_VERIFY` is enabled.
- `fit_image_get_data()` in this pinned 2013.07 tree reads only the inline `data` property. It does not implement FIT external-data `data-offset` / `data-position` handling. P7 preview must label external-data images unsupported rather than interpreting offsets as inline payloads.
- `fit_check_format()` checks root description, timestamp (when enabled), and `/images`; configuration selection is separate. `fit_conf_get_node()` resolves `/configurations/default` when no explicit configuration is selected, and `fit_conf_get_prop_node()` resolves the configuration’s `kernel`, `ramdisk` and `fdt` references into `/images`.
- `fit_image_load()` is not a preview API: after validation it may copy component bytes to the FIT `load` address using `memmove()`. Do not call `fit_image_load()`, `bootm`, `bootstage3` or other load/boot paths from P7 preview.
- The existing P7-1 parser only validates FDT header/offset bounds. P7-2 must add bounded token traversal, node/property validation, configuration reference resolution, inline-data bounds, hash metadata and hash calculation. It must distinguish structure-valid, component hash PASS/FAIL/IGNORED/UNSUPPORTED, and signature NOT VERIFIED.

## 2. libfdt validation boundary

- `fdt_check_header()` in this tree checks magic and version compatibility; it does not by itself prove the full blob fits within the received `filesize`.
- The FDT header contains separate structure, strings and memory-reservation offsets/sizes. Before calling traversal/property APIs, P7 must prove all blocks lie within the FDT totalsize, do not overlap improperly, and the memory-reservation map terminates within bounds.
- Structure traversal must reject malformed tags, unterminated node/property strings, invalid property name offsets, truncated values, unbalanced nodes and missing `FDT_END`.
- All pointer arithmetic must be checked against both the FIT totalsize and the received file size. No FDT mutator API is permitted in preview.

## 3. SPI bootloader layout and update path

- The shared OCTEON header defines a packed 192-byte base `bootloader_header_t`, magic `BOOT`, and image types: Stage 2 = 1, Stage 3 = 2, NOR = 3, PCI = 4, U-Boot environment = 5, Pre-U-Boot = 6, Stage 1 = 7.
- `octeon_common.h` sets `CONFIG_OCTEON_SPI_STAGE2_START=0x10000`; the original SPI image’s valid Stage 2 header is exactly at that offset.
- `cmd_octeon_bootloader.c` contains SPI map scanning/reordering code. The map scans from the configured Stage 2 start, validates headers and payloads, rounds image spans to erase boundaries, and the reorder/update path can move bootloaders, erase sectors and write images.
- The SPI update branch is conditional on `CONFIG_OCTEON_SPI_BOOT_END`. That symbol was not found in the PA-220 board/common/command headers reviewed in this pass; final target macro expansion still needs explicit confirmation before making any claim about which updater code is linked.
- The original SPI backup is analyzed separately in [SPI-IMAGE-ANALYSIS.md](SPI-IMAGE-ANALYSIS.md). Its valid header offsets and FDTs provide static SPI evidence only. Never treat its contents as eMMC GPT evidence.
- Do not execute `bootloaderupdate`, `sf erase`, `sf write`, `sf update` or any SPI updater during P7-2.

## 4. eMMC DOS/MBR live inventory and write hazards

- PA-220 configuration includes EFI/GPT support, but that is a compiled capability, not proof that the installed card uses GPT. The live `mmc part` output identifies this device's partition table as DOS/MBR.
- The live table contains primary partitions 1–3, extended container 4, and logical partitions 5–8. `mmc part` successfully enumerates the logical entries inside the extended partition. Exact start LBA, sector count and MBR type are recorded in [EMMC-DEVICE-ACCEPTANCE.md](EMMC-DEVICE-ACCEPTANCE.md).
- Live table values match the independent offline `fdisk`, `parted` and `sfdisk` reports for the full User Area image stored on `main`. The live card identity/capacity also matches the recorded 29,595,009,024-byte image.
- Live `ext2ls` root-directory reads succeeded on partitions 1, 2, 3, 5, 6 and 8 (with the historical 3/5/6/8 results reused rather than repeated). Partition 4 is an extended container; partition 7 is Linux swap.
- This is not a GPT device. GPT primary/backup header and entry-array consistency checks are therefore not applicable. Raw MBR/EBR sector dumps and byte-level EBR chain checksums were not captured, so do not claim that deeper audit.
- `gpt write` is not a verifier: `cmd_gpt.c` builds a partition description and calls `gpt_restore()`, which writes primary and backup GPT structures through `block_write()`. It must not be used on this DOS/MBR device.
- `mmc write`, `mmc erase`, `gpt write`, `ext4write` and `fatwrite` are persistent write paths. Do not use them for P7-2 evidence collection.
- No install manifest is approved. All eMMC firmware install targets remain `UNMAPPED`.

## 5. Environment persistence discrepancy

The PA-220 board header begins with a comment claiming the environment is RAM-only, but the same header defines `CONFIG_ENV_IS_IN_SPI_FLASH`, redundant environment, `CONFIG_ENV_OFFSET=0xe00000`, `CONFIG_ENV_OFFSET_REDUND=0xe10000`, `CONFIG_ENV_SIZE=8 KiB` and `CONFIG_ENV_SECT_SIZE=64 KiB`. The command configuration enables `CONFIG_CMD_SAVEENV`.

The pinned `env_sf.c` redundant save path erases one 64 KiB sector, writes the new 8 KiB environment, restores the remaining sector contents and marks the old copy obsolete. Therefore `saveenv` is a persistent SPI NOR write, not a RAM-only operation. `setenv` alone changes the runtime environment; it does not persist until `saveenv` is called.

The P7-2A backup scan found both configured environment sectors entirely erased. The new Stage 3 boot log’s bad-CRC/default-environment warning is consistent with the backup, but the backup alone does not prove the live chip has not changed since it was taken. Never call `saveenv` in P7-2.

## 6. P7-2 work-unit status

| Work unit | Scope | Status |
|---|---|---|
| P7-2A | Static SPI NOR backup inventory, OCTEON headers, embedded FDTs, erased regions and environment-sector bytes | PASS; see [SPI-IMAGE-ANALYSIS.md](SPI-IMAGE-ANALYSIS.md) |
| P7-2B | FIT configurations/subimages, inline data bounds, hash states and signature capability classification | Implementation staged; Actions target build and dedicated FIT vectors pending; see [FIT-IMPLEMENTATION.md](FIT-IMPLEMENTATION.md) |
| P7-2C | Live eMMC DOS/MBR inventory and read-only filesystem root probes | PASS; see [EMMC-DEVICE-ACCEPTANCE.md](EMMC-DEVICE-ACCEPTANCE.md) |

No install manifest is approved by this review. P7-2 remains in progress and all install mappings remain `UNMAPPED`.

## 7. Evidence reuse and remaining scope

P7-2C live evidence is already available in [EMMC-DEVICE-ACCEPTANCE.md](EMMC-DEVICE-ACCEPTANCE.md), including the 2026-11-27 `version` / `mmc list` / `mmc info` / `mmc part` results, the 2026-11-18 filesystem directory probes, and cross-checks against the full User Area image metadata on `main`.

The physical device and eMMC have remained unchanged. Do not repeat the directory probes or request another offline image upload merely to recreate evidence already captured. Further raw MBR/EBR sector capture is not required for the current inventory acceptance; it would be a separate deeper forensic task only if a later requirement needs byte-level EBR validation.

No install manifest is approved by this review. P7-2 remains in progress: P7-2B FIT semantic preview code is staged but not yet accepted pending target build and dedicated FIT vectors. All eMMC/SPI install mappings remain `UNMAPPED`.
