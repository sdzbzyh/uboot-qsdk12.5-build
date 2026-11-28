# PA-220 OEM SPI executable scan

Read-only analysis of the original SPI backup. No device access or writes.

- SPI file: /home/runner/work/_temp/pa220-spi-normal.img
- SPI size: 16777216 bytes (0x1000000)

## OEM FDT signatures

- FDT at SPI offset 0x1c9460, size 10850 (0x2a62)

## Embedded MIPS64 big-endian ELF candidates

- No structurally valid ELF64 big-endian MIPS payload found by signature scan.
## Bootloader strings

~~~text
0x5b928: U-Boot
0x67048: coremask_override
0x6af60: U-Boot 11.0.0.0-71 (Build time: Mar 12 2024 - 10:35:29)
0x6d910: OCTEON %s, Core clock: %lld MHz
0x1ccd54: coremask    - mask of cores to run on.  Anded with coremask_override environment
0x1ce6c2: coremask    - mask of cores to run on.  Anded with coremask_override
0x1ceac5: coremask    - mask of cores to run on.  Anded with coremask_override
0x1d1ef0: U-Boot
0x1e45f0: coremask_override
0x1e5820: coremask=
0x1e9378: U-Boot 11.0.0.0-71 (Build time: Mar 12 2024 - 10:35:56)
0x1e9498: OCTEON %s, Core clock: %lld MHz
0x1e9f48: OCTEON MMC: ERROR: Only native GPIO pins supported
0x1f6710: PA-220
~~~

## Interpretation limits

- An ELF signature scan can miss a flat or compressed U-Boot image.
- A stripped ELF may not retain function names; absence of symbols is not proof that code is absent.
- The scan does not assume that the FDT's location identifies the start or full extent of Stage-3.
