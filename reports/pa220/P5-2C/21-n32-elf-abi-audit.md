# PA-220 P5-2C OCTEON N32 ELF / ABI audit

Run: [45137814338](https://github.com/sdzbzyh/uboot-qsdk12.5-build/actions/runs/45137814338)  
Integration commit tested: `3526880c4987fe4e7ee03109f06faf10e163a3a2`  
PA-220 source: `801717ba6a8a59ed2cfd53bf3e1b19841fecd2e8`

## Toolchain provenance

- Release: `sdzbzyh/pa220-toolchains`, `octeon-sdk-5.1.0`
- Archive: `octeon-sdk-5.1.0-tools-gcc-4.7.tar.gz`
- Archive SHA256: `cd8cd3232078094c89accf32a82a13e7847e3466c94e9b5e12dd11b741102d99`
- SHA256 sidecar verification: passed (`sha256sum -c`)
- Compiler: Cavium GCC 4.7.0, SDK Build 52
- Target triple: `mips64-octeon-linux-gnu`
- Build configuration: `octeon_pa220_config`, OCTEON_CN70XX

## Objects compiled

The Runner compiled the foreground command, TCP Core, TCP IPv4/Ethernet transmitter, HTTP parser, HTTP handler and U-Boot data provider with the OCTEON N32 target flags and `-fstack-usage`.

All six object compilations completed successfully. Their individual ELF headers are retained in the Actions Artifact.

## Relocatable link

- Link mode: N32 relocatable link (`-mabi=n32 -mabicalls -msoft-float -G 0 -nostdlib -Wl,-r`)
- Output: `pa220-http-lifecycle-n32.o`
- ELF class: ELF32
- Endianness: big-endian
- ELF type: REL (Relocatable file)
- Flags: `abi2, octeon, mips64r2`
- Link result: PASS
- SHA256: `e6b67dabfaeba0f75a1743b7c4e899345669cc942fe59ea30b927d46c0bebd95`

## Undefined symbols and allocation dependency

The combined object retains expected U-Boot integration symbols such as Ethernet, network, timer, watchdog, console and memory primitives. The undefined-symbol report contains no `malloc`, `calloc`, `realloc` or `free` dependency. The workflow's explicit allocation-symbol rejection check passed.

This is a relocatable object audit, not a final U-Boot executable link. It does not establish that every external symbol is resolved by the complete firmware link.

## Evidence boundary

The successful N32 compile and relocatable link are verified cloud results. Full firmware image linking, runtime execution and PA-220 physical-device validation remain outstanding.
