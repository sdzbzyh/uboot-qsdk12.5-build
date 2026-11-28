# P7-3A PA-220 live read-only partition inventory acceptance

- Date: 2026-11-28
- Branch: `feature/pa220-ubootkit-web`
- Fixed PA-220 source: `sdzbzyh/pa220-openwrt@801717ba6a8a59ed2cfd53bf3e1b19841fecd2e8`
- Software build: Actions [#45988344341](https://github.com/sdzbzyh/uboot-qsdk12.5-build/actions/runs/45988344341), Artifact [15192058990](https://github.com/sdzbzyh/uboot-qsdk12.5-build/actions/runs/45988344341/artifacts/15192058990)
- Result: **PASS — live MBR/EBR inventory matches the independently recorded PA-220 eMMC layout**

## RAM-only Stage3 transfer

The Stage3 wrapper was received from TFTP at `0x20000000`.

- Wrapper: 1,399,688 bytes (`0x155b88`)
- Format: legacy uImage
- CRC32: `439f61c8`
- SHA-256: `7f6854b91e75b88203b42bfc09495423da0bb4900e674ef944e86c2f3a87bb30`
- Inner payload: `0x155b48` bytes, copied from `0x20000040` to `0x81000000`
- `cmp.b` confirmed all 1,399,624 payload bytes identical after copy.

A transient RGMII receive warning (code 11) appeared during TFTP, but the transfer completed and the receiver reported verified size, format, CRC32 and SHA-256.

## Live partition inventory

Command:

```
pa220 partition scan 0
```

Observed result:

```
partition table: DOS/MBR, capacity=57802752 sectors, count=7
  #1 PRIMARY start=16 sectors=3952 type=0x83
  #2 PRIMARY start=3968 sectors=8000064 type=0x83
  #3 PRIMARY start=8004032 sectors=8000064 type=0x83
  #5 LOGICAL start=16004112 sectors=24000048 type=0x83
  #6 LOGICAL start=40004176 sectors=8000048 type=0x83
  #7 LOGICAL start=48004240 sectors=3952 type=0x82
  #8 LOGICAL start=48008208 sectors=9794544 type=0x83
partition inventory: READ_ONLY; target mapping: UNMAPPED
```

The seven data partitions and their on-disk indices (1, 2, 3, 5, 6, 7, 8) match the known PA-220 eMMC inventory. The extended container at index 4 is correctly not exposed as a filesystem/data target. The result is consistent with the prior read-only eMMC evidence; this test does not claim a raw-sector EBR byte-for-byte audit.

## Memory and safety boundary

`pa220 memory` reported:

- `loadaddr=0x20000000`
- `__tmp_load=[0x20000000,0x30000000)`
- no persistent load base or load size published.

The inventory reports `READ_ONLY` and `UNMAPPED`. No partition mapping was inferred from the generic MBR type codes. No eMMC/SPI/NAND write or erase, `saveenv`, updater, image load, or boot operation was performed by the inventory command.

## Acceptance

P7-3A live inventory acceptance: **PASS**.

P7-3B remains the next work unit: implement manifest-gated dry-run planning. Until an explicitly reviewed PA-220 install manifest exists, every image component must remain `UNMAPPED` and the plan must reject with `NO_APPROVED_MANIFEST`. No executable write script or storage-write API may be produced or called.

This report records the supplied serial-console evidence; it does not claim a raw UART capture artifact.
