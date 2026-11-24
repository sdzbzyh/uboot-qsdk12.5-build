# P4-2 Host Harness Results

Fixed PA-220 source: `sdzbzyh/pa220-openwrt@801717ba6a8a59ed2cfd53bf3e1b19841fecd2e8`
Verified integration commit: `908a58f43017f3e252b3ce0c613b4630072203e9`
Actions run: [#45374041373](https://github.com/sdzbzyh/uboot-qsdk12.5-build/actions/runs/45374041373)

## Production RAM validator

| Check | Result |
|---|---:|
| Normal GCC | `cases=42 assertions=161 failures=0` |
| ASan + UBSan | `cases=42 assertions=161 failures=0` |
| Rejected-range guarded write | No write; destination unchanged |

The harness compiles `u-boot/arch/mips/cpu/octeon/octeon_load_range.c` itself.

## TFTP DATA pre-write guard

| Check | Result |
|---|---:|
| Normal GCC | `cases=14 assertions=64 failures=0` |
| ASan + UBSan | `cases=14 assertions=64 failures=0` |
| Rejected DATA block | No intercepted memcpy |
| Accepted DATA block | One intercepted write in the final positive case |

The harness compiles the exact `STORE_BLOCK` function text used by the patch generator with a mocked `octeon_load_range_check()` and intercepted `memcpy()`. It covers arithmetic/boundary rejection and confirms no copy on rejected requests. It is not a complete TFTP protocol/UDP integration test.

## Target build

The same Actions run also verified the generated CN70XX TFTP source compiled as `tftp.o` and entered `libnet.o`, the validator object/N32 relocatable link, and the complete PA-220 U-Boot final link. This is compile/link evidence only, not an actual TFTP receive test.
