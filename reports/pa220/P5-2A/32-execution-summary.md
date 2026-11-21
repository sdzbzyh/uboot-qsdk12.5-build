# P5-2A execution summary

- Host regression: 35 cases, 95 assertions, zero failures; see 10-host-tests.log and 30-test-matrix.md.
- Host parser structure size: 13224 bytes.
- OCTEON SDK: octeon-sdk-5.1.0, Cavium GCC 4.7.0; compiler target triple mips64-octeon-linux-gnu.
- Configured target ABI: ELF32, big-endian, MIPS N32 ABI2, OCTEON / MIPS64r2; see 21-parser-object-elf-header.txt.
- Target parser object: 26800 bytes; SHA-256 9d0609254d6dd83c269d96080e0391b581d192693d16861039b807dd936b4d94.
- Undefined references: memcmp, memcpy, memset only; these are U-Boot memory/string primitives. No malloc/free or other libc dependency is introduced.
- Relocatable target link: PASS; linked ELF ABI recorded in 25-linked-object-elf-header.txt.
- Linked object SHA-256: 7e569b5a4183835617b75d0406a7cfd19cc02df3d02ae31b605a6936ad020156.
- Target parser structure size: 0x33a8 = 13224 bytes; see 27-parser-static-size.txt.
- Target source baseline: IronDemiGod/pa220-openwrt@801717ba6a8a59ed2cfd53bf3e1b19841fecd2e8.
- Build input integration commit: 5a02e0b7701cedab6e355f47d55f8f59679a2221.
- GitHub Actions run: 44886769145 (success).
- Scope: standalone P5-2A parser only. No TCP or pa220 web lifecycle changes; no P5-2B URI handler.
