# P5-2A execution summary

- Host regression: 35 cases, 95 assertions, zero failures; see 10-host-tests.log and 30-test-matrix.md.
- Host parser structure size: 13224 bytes.
- OCTEON SDK: octeon-sdk-5.1.0, Cavium GCC 4.7.0; compiler target triple mips64-octeon-linux-gnu.
- Configured target ABI: ELF32, big-endian, MIPS N32 ABI2, OCTEON / MIPS64r2; see 21-parser-object-elf-header.txt.
- Target parser object: 26864 bytes; SHA-256 ae6d2491f5eea61bf70051ecd9bca9da5e91275e79020eae1f694592cdd0a008.
- Undefined references: memcmp, memcpy, memset only; these are U-Boot memory/string primitives. No malloc/free or other libc dependency is introduced.
- Relocatable target link: PASS; linked ELF ABI recorded in 25-linked-object-elf-header.txt.
- Linked object SHA-256: 5ce690b203c403a8895cb6fcfb09bb4e9a74f7fc3b685bd28e079fc25b31ebc0.
- Target parser structure size: 0x33a8 = 13224 bytes; see 27-parser-static-size.txt.
- Target source baseline: IronDemiGod/pa220-openwrt@801717ba6a8a59ed2cfd53bf3e1b19841fecd2e8.
- Build input integration commit: 3ff73d0627bcc7007e9f1d6ba8e705cc895330da.
- GitHub Actions run: 44887007342 (success).
- Scope: standalone P5-2A parser only. No TCP or pa220 web lifecycle changes; no P5-2B URI handler.
