# P7-3B manifest-gated dry-run implementation checkpoint

- Date: 2026-11-28
- Branch: `feature/pa220-ubootkit-web`
- Fixed PA-220 source: `sdzbzyh/pa220-openwrt@801717ba6a8a59ed2cfd53bf3e1b19841fecd2e8`
- Result: **SOFTWARE IMPLEMENTATION + HOST/SANITIZER + CAVIUM FULL LINK PASS; LIVE DRY-RUN PASS; LEGACY BOOT PATH MISMATCH UNDER INVESTIGATION**

## Implementation

- Planner API: `u-boot/board/octeon/pa220/pa220_install_plan.c`
- Public model: `u-boot/include/pa220_install_plan.h`
- Production command: `pa220 install dry-run [mmc-device]`, integrated by `scripts/generate-pa220-p7-partition-patch.py` and staged by `build-pa220.sh`.
- The command requires completed `fileaddr/filesize` metadata and revalidates the complete image range against the live `__tmp_load` reservation; parses the image; reads the selected MMC device using the read-only sector callback; and passes the unified MBR/EBR or GPT inventory to the planner.
- There is currently **no approved PA-220 install manifest**. The production command therefore invokes the planner with no manifest and must report `NO_APPROVED_MANIFEST`, `target mapping=UNMAPPED`, `actions=NONE`. This is intentional fail-closed behavior, not a missing default target.
- The planner API has no storage-write callback. It requires an explicit user selection for every component; it never auto-selects by partition number, MBR type, filename, FIT node name or load/entry address. A reviewed manifest constrains image formats, board type, component size and allowed MBR type/GPT type GUID; the user supplies the actual partition selection. Duplicate targets, unverified components, source overflow/overlap, device/partition capacity violations and target-type mismatch reject.
- Production code must set manifest state `APPROVED` only after matching a reviewed manifest against a firmware-pinned digest. An approval field inside an untrusted RAM/file manifest is never trusted.
- No partition creation/deletion/move/format, no eMMC/SPI/NAND write, no `saveenv`, no updater and no boot operation are part of P7-3B.

## Host and sanitizer evidence

Actions [#46113003495](https://github.com/sdzbzyh/uboot-qsdk12.5-build/actions/runs/46113003495) completed SUCCESS; Artifact [15249864977](https://github.com/sdzbzyh/uboot-qsdk12.5-build/actions/runs/46113003495/artifacts/15249864977).

- Host GCC strict build: 15 assertions, 0 failures.
- AddressSanitizer + UndefinedBehaviorSanitizer: 15 assertions, 0 failures.
- Coverage includes no manifest, unreviewed manifest, approved synthetic MBR/GPT policies, explicit target selection, format/type mismatch, component integrity, source range overflow/overlap, duplicate target, partition capacity and device-capacity bounds, and board mismatch.

## Pinned OCTEON build and RAM test artifact

Actions [#46113156556](https://github.com/sdzbzyh/uboot-qsdk12.5-build/actions/runs/46113156556) completed SUCCESS; Artifact [15250271914](https://github.com/sdzbzyh/uboot-qsdk12.5-build/actions/runs/46113156556/artifacts/15250271914).

- Pinned Cavium SDK 5.1.0 / GCC 4.7.0 complete PA-220 U-Boot final link: PASS.
- Final ELF includes `pa220_install_plan_build` and `pa220_install_dry_run_cmd`.
- Final BIN: 1,402,408 bytes; SHA-256 `3144ea1e15fd1f74464908f09ee4feeaaae62f6252a3401d2dc35dedd197d23f`.
- RAM Stage3 wrapper: legacy uImage, 1,402,472 bytes (`0x156668`); SHA-256 `ecc0ba7d35c697c11adbaa9f86e8eda2b52a9113c616b5bf02f6891714b471d8`.
- Wrapper header CRC32 `fa4cbe08`; payload CRC32 `7f2db379`.
- Wrapper payload copy remains offset `0x40` to RAM `0x81000000`; the wrapper itself must not be executed.
- Existing fixed-source default boot command has not been changed: load `/vmlinux.oct3-mp` from `mmc 0:1`, fallback to `.bak`, then `bootoctlinux`; root remains `mmc 0:2` per the board config comment. This is the expected legacy boot path, not yet live-boot acceptance.

## Next acceptance step

Perform one RAM-only device round:
1. Boot this Stage3 wrapper using the already-proven TFTP/copy/compare/GO sequence.
2. Interrupt autoboot at the new U-Boot prompt.
3. TFTP the wrapper again so the new U-Boot publishes fresh verified `fileaddr/filesize`.
4. Run `pa220 install dry-run 0`; expect the live DOS/MBR inventory, `NO_APPROVED_MANIFEST`, `UNMAPPED`, and `actions=NONE`.
5. Then run the unchanged `run bootcmd` to test whether the original eMMC OpenWrt system still boots from p1 with root on p2. This is read-only with respect to persistent storage; do not run `saveenv` or any write command.

P7-3B remains pending live device acceptance. This report records cloud/software evidence only.


## Live device acceptance update (2026-11-28)

The live `pa220 install dry-run 0` test passed its intended fail-closed behavior: the device reported a read-only DOS/MBR inventory and `NO_APPROVED_MANIFEST`, `UNMAPPED`, `actions=NONE`. The subsequent unchanged legacy `run bootcmd` did not boot Linux because neither `/vmlinux.oct3-mp` nor its `.bak` fallback exists at the p1 filesystem root; `bootoctlinux` consequently found no ELF at `0x21000000`. The kernel was not launched.

Live read-only investigation found the actual kernel ELF at `mmc 0:3 /boot/vmlinux-3.10.87-oct3-mp` (10,387,920 bytes, ELF64 big-endian MIPS). The p2 ext label is `sysroot0`; p3 is `sysroot1`, matching the previously inspected p3 `/etc/fstab` root label. Thus the current installation's root is p3, contrary to the old board-header comment that says p2. Kernel built-in command-line/root-selection behavior remains unverified; no Linux boot acceptance is claimed. Full evidence: `reports/pa220/P7-3/P7-3B-DEVICE-ACCEPTANCE.md`.
