# P7-2A SPI NOR backup forensic report

- Date: 2026-11-27
- Repository: `sdzbzyh/uboot-qsdk12.5-build`
- Integration branch: `feature/pa220-ubootkit-web`
- Fixed PA-220 source: `sdzbzyh/pa220-openwrt@801717ba6a8a59ed2cfd53bf3e1b19841fecd2e8`
- Input branch: `main` at `da755efd49df32f3e85d231745212674737feba5`
- Input: `pa220-spi-normal.img` (main branch; not copied into the integration branch)
- Analysis: read-only Actions run [#45962029417](https://github.com/sdzbzyh/uboot-qsdk12.5-build/actions/runs/45962029417)
- Artifact: [15181630025](https://github.com/sdzbzyh/uboot-qsdk12.5-build/actions/runs/45962029417/artifacts/15181630025)
- Result: **SPI IMAGE INVENTORY PASS** (format/CRC evidence only; not install authorization)

## 1. Source image identity

| Property | Value |
|---|---|
| Image size | 16,777,216 bytes (`0x1000000`, 16 MiB) |
| SHA-256 | `4899a1c5ed06e5eafbb8c92625a4afae14cad399806df13979fd2c70ee1ed922` |
| Whole-image CRC32 | `0x4775cc0d` |
| OCTEON header candidates | 11 |
| Fully valid OCTEON headers | 4 |
| CRC-valid embedded legacy uImages | 0 |
| Bounded embedded FDTs | 2 |

The image size matches the previously recorded PA-220 SPI probe result for Winbond W25Q128BV (16 MiB, 4 KiB erase size). This size match is corroborative; it does not independently prove which live chip-select supplied the backup.

## 2. Valid OCTEON bootloader headers

The scanner used the PA-220 OCTEON header layout and the same bounded header/data CRC calculation used by the P7 parser. All four records below passed length bounds, header CRC and data CRC.

| SPI offset | Header type | Board type | Header | Data | Total | Link address | Header CRC | Data CRC |
|---:|---|---:|---:|---:|---:|---|---|---|
| `0x000000` | Pre-U-Boot (6) | 28 generic | `0xc0` | `0x1cb0` | `0x1d70` | `0xffffffff81000000` | `0x0b6a0ff4` | `0x2dc6c8fb` |
| `0x001d80` | Pre-U-Boot (6) | 28 generic | `0xc0` | `0x1fe0` | `0x20a0` | `0xffffffff81000000` | `0x6e5c7fd8` | `0x44309488` |
| `0x010000` | Stage 2 (1) | 20021 PA-220 | `0xc0` | `0x5f368` | `0x5f428` | `0xffffffffc0000000` | `0xa722c456` | `0xe91eb68d` |
| `0x100000` | NOR bootloader (3) | 20021 PA-220 | `0xc0` | `0x140008` | `0x1400c8` | `0xffffffffc0000000` | `0x3bd60552` | `0x103c4bcb` |

The fixed source enum describes type 6 as `BL_HEADER_IMAGE_PRE_UBOOT` (“bootloader before U-Boot (stage 1/1.5)”). The image contains the strings “SPI stage 1 bootloader” at offset `0x19e8` and “SPI stage 1.5 bootloader” at `0x3af0`, corresponding to the first two records. The evidence-consistent chain is therefore:

1. Pre-U-Boot / Stage 1 at SPI offset `0x000000`.
2. Pre-U-Boot / Stage 1.5 at SPI offset `0x001d80`.
3. PA-220 Stage 2 at SPI offset `0x010000`.
4. PA-220 NOR bootloader at SPI offset `0x100000`.

This is a static-image reconstruction supported by valid headers, enum definitions and embedded strings. It is not a live trace of which slot the device selected during its last boot.

The Stage 2 record ends at `0x06f428`; the Stage 3/NOR record ends at `0x2400c8`. The image contains all-`0xff` space from `0x070000` to `0x100000` (589,824 bytes), and from `0x250000` to `0x1000000` (14,352,384 bytes).

## 3. Embedded FDTs

Two bounded FDTs were found and structurally traversed by the read-only scanner.

### Stage 2 FDT

- SPI offset: `0x058030` (inside the Stage 2 image)
- Total size: `0x0f59`
- Root model/compatible: `cavium,generic_spi_stage2`
- Node count: 16
- Contains an OCTEON SPI controller and `nor-flash@2` compatible with `micron,n25q128`; also describes an OCTEON NAND interface.

### Stage 3 / PA-220 FDT

- SPI offset: `0x1c9460` (inside the NOR bootloader image)
- Total size: `0x2a62`
- Root model: `pan,pa-220`
- Root compatible: `cavium,tb7100`
- Node count: 59
- Contains an OCTEON SPI controller and `flash@0` compatible with `w25q128 | is25lp128`.
- Contains an OCTEON MMC host (`cavium,octeon-6130-mmc`) with an 8-bit slot.
- Contains a CPLD node compatible with `kingfisher-mp,cpld`.

The Stage 2 FDT and Stage 3 FDT describe different SPI NOR compatible strings / chip-select nodes. Do not collapse them into one physical mapping without confirming the boot-stage device tree and live SPI bus/CS routing. The Stage 3 FDT’s MMC node proves only that the controller is described; it does not reveal the live eMMC GPT or partition names.

## 4. Environment sectors and erased regions

The PA-220 board header defines:

- `CONFIG_ENV_IS_IN_SPI_FLASH`
- `CONFIG_SYS_REDUNDAND_ENVIRONMENT`
- primary offset `0xe00000`
- redundant offset `0xe10000`
- environment payload size 8 KiB
- erase sector size 64 KiB
- SPI bus 0 / CS 0

The backup bytes at all three offsets below are entirely `0xff` for a full 64 KiB sector:

| Region | Offset | Sector contents | SHA-256 |
|---|---:|---|---|
| Configured primary environment | `0xe00000` | all `0xff` | `71189f7fb6aed638640078fba3a35fda6c39c8962e74dcc75935aac948da9063` |
| Configured redundant environment | `0xe10000` | all `0xff` | `71189f7fb6aed638640078fba3a35fda6c39c8962e74dcc75935aac948da9063` |
| Legacy OEM environment offset mentioned in comment | `0xef0000` | all `0xff` | `71189f7fb6aed638640078fba3a35fda6c39c8962e74dcc75935aac948da9063` |

This agrees with the new Stage 3 boot log reporting invalid environment CRC and falling back to defaults. It does not prove the current live SPI contents are unchanged from this backup. The board-header comment claiming a legacy OEM environment at `0xef0000` is not supported by this backup’s bytes.

The board-header comment also says the custom U-Boot area is `0x250000-0x39afff`; that whole region is erased in this original backup. This is evidence that the backup contains no image in that area, not permission or a verified install mapping for a future image.

## 5. Conclusions and limits

- The backup contains four CRC-valid OCTEON headers and a coherent Stage 1 / Stage 1.5 / Stage 2 / NOR bootloader layout.
- The Stage 3 FDT describes PA-220, the W25Q128-compatible SPI flash, the OCTEON MMC controller and the Kingfisher CPLD.
- The configured environment sectors are erased in this backup. The runtime bad-CRC message is consistent with that state.
- No eMMC GPT, eMMC partition entry, filesystem layout or eMMC capacity can be inferred from this SPI NOR image.
- No CRC-valid legacy uImage was found inside the SPI image. The embedded FDTs are standalone FDT blobs, not proof of a FIT image.
- CRC validity is integrity evidence, not signature authenticity, OEM authorization or installation authorization.
- No device was accessed. No SPI/eMMC write, erase, updater, environment save or boot command was executed during this analysis.
- All P7 install target mappings remain `UNMAPPED`.

