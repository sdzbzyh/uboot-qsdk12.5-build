# P7-3B live device acceptance and original eMMC boot-path investigation

- Date: 2026-11-28
- Branch at test start: `feature/pa220-ubootkit-web`
- Device: PA-220, original eMMC User Area, MMC device 0
- Scope: RAM-only Stage3 execution, read-only install dry-run, read-only eMMC boot-path diagnosis
- Result: **P7-3B LIVE DRY-RUN PASS; ORIGINAL BOOT PATH REQUIRES RECONCILIATION; NO LINUX BOOT ACCEPTANCE YET**

## Stage3 and dry-run

The Stage3 wrapper was TFTP-loaded to RAM at `0x20000000`, and its payload was copied to `0x81000000` and byte-compared against the wrapper payload. The downloaded wrapper metadata was:

- Size: 1,402,472 bytes (`0x156668`)
- SHA-256: `ecc0ba7d35c697c11adbaa9f86e8eda2b52a9113c616b5bf02f6891714b471d8`
- Whole-wrapper CRC32 reported by the receive verifier: `9bbb8008`
- Payload copy: source `0x20000040`, destination `0x81000000`, length `0x156628`
- `cmp.b`: all 1,402,408 bytes identical

Live command `pa220 install dry-run 0` returned:

```text
image: legacy uImage size=1402472 bytes
partition inventory: DOS/MBR count=7; READ_ONLY
install dry-run: REJECTED reason=NO_APPROVED_MANIFEST; target mapping=UNMAPPED; actions=NONE
```

This is the expected fail-closed result. The live DOS/MBR inventory was read successfully and no install target was selected. No storage-write path was invoked.

## Original boot command result

The unchanged configured command was tested:

```text
bootcmd=ext2load mmc 0:1 0x21000000 /vmlinux.oct3-mp || ext2load mmc 0:1 0x21000000 /vmlinux.oct3-mp.bak; bootoctlinux 0x21000000 numcores=4 endbootargs
## Error: "bootargs" not defined
## File not found /vmlinux.oct3-mp
## File not found /vmlinux.oct3-mp.bak
## No elf image at address 0x21000000
```

The undefined `bootargs` environment variable is diagnostic only: this command does not expand `bootargs`. The command fails because neither kernel path exists at the p1 filesystem root; consequently `bootoctlinux` receives no ELF at `0x21000000`. The kernel was not launched.

## Actual kernel and root filesystem found on eMMC

The following live read-only observations supersede the old board-header comments that describe p1 as the kernel partition and p2 as root:

1. `ext2ls mmc 0:2 /` shows only `lost+found`, `dev`, and `var`.
2. `ext2ls mmc 0:3 /` shows a complete Linux root tree, including `etc`, `usr`, `var`, `boot`, `home`, `root`, `proc`, `sys`, `opt`, and `tmp`.
3. `ext2ls mmc 0:3 /boot` shows `vmlinux-3.10.87-oct3-mp` (10,387,920 bytes) and a 23-byte `vmlinux.oct3-mp` symlink.
4. Loading `/boot/vmlinux-3.10.87-oct3-mp` from `mmc 0:3` to RAM at `0x20000000` succeeded. Its first four bytes are `7f 45 4c 46` (ELF); the ELF header identifies a 64-bit big-endian MIPS image. `filesize=0x9e81d0`.
5. ext superblock reads (magic `53 ef`) established the volume labels:
   - p2: read LBA `0xF82`; label at buffer offset `0x78` is `sysroot0`.
   - p3: read LBA `0x7A21C2`; label at buffer offset `0x78` is `sysroot1`.
6. The previously inspected p3 `/etc/fstab` identifies `LABEL=sysroot1` as the root filesystem.

Therefore the current PAN 10.2.11 installation's root filesystem is p3 (`mmc 0:3`), labelled `sysroot1`; p2 is `sysroot0`. The current kernel ELF is in p3 `/boot`, not at the configured p1 root path.

## Remaining boot-compatibility question

The fixed U-Boot board config has `CONFIG_BOOTARGS` undefined and its boot command ends at `numcores=4 endbootargs`, so it supplies no explicit Linux root argument. The p3 filesystem label is now confirmed, but the running kernel's built-in command line / root selection has not yet been established. A normal Linux boot is therefore not yet accepted.

Next, inspect the exact kernel command-line behavior or perform a separately controlled RAM-only diagnostic boot using the confirmed p3 root. Do not repeat the known-failing `run bootcmd`. Do not run `saveenv`, any eMMC write/erase command, filesystem repair, or journal-replaying mount during diagnosis.

## Persistence and safety

- The Stage3 wrapper and kernel were loaded into RAM only.
- eMMC reads were limited to partition/filesystem metadata and file reads.
- No `mmc write`, `mmc erase`, `saveenv`, SPI write/erase, partition modification, or filesystem repair was run.
- The failed legacy boot command did not launch Linux.
