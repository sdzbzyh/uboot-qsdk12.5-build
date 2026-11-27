# P7-3A implementation progress

- Date: 2026-11-27
- Branch: `feature/pa220-ubootkit-web`
- Fixed PA-220 source: `801717ba6a8a59ed2cfd53bf3e1b19841fecd2e8`
- Status: **CORE IMPLEMENTED; INITIAL HOST/SANITIZER/TARGET-OBJECT GATES PASS; P7-3A NOT YET ACCEPTED**

## Implemented

- Added `u-boot/include/pa220_partition.h` and `u-boot/board/octeon/pa220/pa220_partition.c`.
- Parser uses a caller-supplied one-sector read callback and a 64-bit device capacity; there is no block-write callback or storage mutation path.
- DOS/MBR path handles primary entries and bounded EBR traversal, checks boot flags, checked ranges, extended-container containment, cycles, partition overlaps and multiple extended containers.
- GPT path checks Protective MBR shape, primary/backup Header CRCs and geometry, bounded Entry Array size, both Entry Array CRCs and byte-for-byte agreement, disk GUID, unique partition GUIDs, usable-LBA bounds and partition overlap.
- Results use one common partition record and preserve source format / primary-logical-GPT classification.
- Workspace is fixed and bounded; GPT Entry Array buffers are static rather than consuming a large target stack frame.

## Validation evidence

Actions run [#45985125302](https://github.com/sdzbzyh/uboot-qsdk12.5-build/actions/runs/45985125302) completed SUCCESS at commit `cccf093c760449fc19d69ffefe78987e4f8d17f7`.

- Host GCC strict build and positive/negative fixture suite: PASS.
- Host AddressSanitizer + UndefinedBehaviorSanitizer: PASS.
- Pinned Cavium SDK 5.1.0 / GCC 4.7.0 target object compile: PASS.
- Artifact [15191546135](https://github.com/sdzbzyh/uboot-qsdk12.5-build/actions/runs/45985125302/artifacts/15191546135) contains the logs and target object evidence.
- The sparse PA-220 MBR fixture reproduces the observed 57,802,752-sector device, three primary Linux entries, one extended container and four logical partitions, including the observed LBA/count boundaries. It does not require re-reading or uploading the 29.6 GB live eMMC image.

A further test expansion was committed after that successful run: GPT partition overlap, duplicate unique GUID, usable-LBA overflow, two-node EBR cycle, and correction of the sparse fixture's first EBR link length. Actions run [#45985342164](https://github.com/sdzbzyh/uboot-qsdk12.5-build/actions/runs/45985342164) is the current validation for that expanded matrix and must complete successfully before the expanded suite is accepted.

## Not yet complete

- The parser is not yet integrated into the PA-220 `pa220 partition scan` command or the production board Makefile.
- No full U-Boot final link has yet been run with this parser included.
- The complete P7-3A adversarial matrix still needs further review (including read errors, primary/backup GPT geometry mismatch and malformed EBR variants).
- No live eMMC read has been performed through this new validator; do not claim live P7-3A inventory acceptance yet.
- P7-3B manifest-gated dry-run has not started. There is still no approved PA-220 install manifest; all image components must remain `UNMAPPED` with `NO_APPROVED_MANIFEST`.

No eMMC/SPI/NAND write, erase, `saveenv`, updater, boot, or image-load operation was performed.
