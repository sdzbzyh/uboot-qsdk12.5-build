# P6-A — PA-220 live DRAM descriptor read-only preflight

- Date: 2026-11-26
- Status: **PASS — P6-A read-only preflight**
- Integration branch: `feature/pa220-ubootkit-web`
- Branch HEAD at report creation: `29fba8b26a5a8a0e27178e247be6545e77ff4c7f`
- Fixed PA-220 source: `sdzbzyh/pa220-openwrt@801717ba6a8a59ed2cfd53bf3e1b19841fecd2e8`
- Software gates P6-1A / P6-1B / P6-1C: already accepted; not rerun here.
- Hardware evidence source: user-provided raw serial output from the PA-220 running the newly built image.

## 1. Running image identity

Device output:

```text
U-Boot 2013.07 (Build time: Nov 25 2026 - 11:04:13)
mips64-octeon-linux-gnu-gcc (Cavium Inc. Version: SDK_BUILD build 52) 4.7.0
GNU ld (Cavium Inc. Version: SDK_BUILD build 52) 2.23.52
```

GitHub Actions P6-1C run [#45600860179](https://github.com/sdzbzyh/uboot-qsdk12.5-build/actions/runs/45600860179) built the full PA-220 image at 2026-11-25 11:04:10–11:05:06 UTC. The device build timestamp 11:04:13 matches the target-build step start window and the P6-1C image generation. The run completed SUCCESS. This is strong build-identity correlation; the UART version string alone is not a cryptographic image hash.

## 2. Live device output

Relevant values copied from the supplied serial log:

| Item | Live value |
|---|---|
| `bdinfo.memstart` | `0x80000000` |
| `bdinfo.memsize` | `0x200000000` (8 GiB) |
| `loadaddr` | `0x20000000` |
| `autoload` | `n` |
| `ipaddr` / `serverip` | not defined |
| `ethact` / `ethprime` | `octrgmii0` |
| `__tmp_load` | base `0x20000000`, size `0x10000000` (256 MiB) |
| `__uboot_code_data` | base `0x20f000000`, size `0x01000000` |
| `__tmp_reserved_linux` | base `0x00100000`, size `0x0f000000` |
| `__fdt` | `0x00080000`, size `0x00020000` |
| `__tmp_uboot_exception` | `0x000ff000`, size `0x00000500` |
| `idle-core-loop` | `0x000ffff0`, size `0x00000010` |
| `__mac_meminfo` | `0x0fff55a0`, size `0x00000610` |
| free block 1 | base `0x0f100000`, size `0x00ef55a0` |
| free block 2 | base `0x30000000`, size `0x1df000000` |

The command sequence was read-only: `version`, `bdinfo`, `printenv`, `pa220 memory`, `namedprint`, `freeprint`. No transfer, environment mutation, flash access, reset or reboot was performed.

## 3. Address-domain reconciliation

The apparent mismatch between `bdinfo.memstart=0x80000000` and `__tmp_load=0x20000000` is resolved by the fixed CN70XX address map; these values are not competing DRAM bases.

The validator source defines the PA-220 DDR layout:

- DDR0 physical: `[0x00000000, 0x10000000)`
- Boot-bus aperture (not DRAM): `[0x10000000, 0x20000000)`
- DDR1/DDR2 begins at physical `0x20000000`
- For the reported 8 GiB total, high DDR ends at `0x210000000`

The board's cached DRAM base `0x80000000` is an address alias; it must not be compared directly to the physical bootmem descriptor. The descriptor `0x20000000` is in the DDR1 identity-mapped range. The production validator explicitly normalizes the supported identity/KSEG/XKPHYS address forms and rejects the boot-bus hole.

## 4. Reservation and overlap arithmetic

The live descriptor is:

`__tmp_load = [0x20000000, 0x30000000)` (256 MiB).

This exactly matches the fixed-source automatic 8 GiB load-memory algorithm: it allocates the 256 MiB maximum at physical base 0x20000000 and, on successful allocation, sets `loadaddr`, `load_reserved_addr` and `load_reserved_size` from the same base/size values.

The live `loadaddr=0x20000000` equals the descriptor base. The `octeon_reserved_mem_load_base` and `octeon_reserved_mem_load_size` *environment overrides* are absent; this means the automatic default is in use, not that the runtime named block is missing. `pa220 memory` prints the actual descriptor independently of those optional overrides.

Cross-checks from the live named/free block lists:

- Linux reserved: `[0x00100000, 0x0f100000)`, disjoint from load block.
- Low 1 MiB, FDT, exception, idle-core loop and MAC memory info are all below the load block.
- The second free block begins exactly at `0x30000000`, the exclusive end of `__tmp_load`.
- U-Boot code/data reservation: `[0x20f000000, 0x210000000)`, at the high end of the 8 GiB physical DDR map and disjoint from the load block.
- The first free block ends below `0x20000000`; it also does not overlap the load block.

The 32 MiB P6-1B maximum GET reservation from `loadaddr` would be:

`[0x20000000, 0x22000000)`

This is contained in the 256 MiB live descriptor with 224 MiB remaining. The initial 4 KiB fixture is also contained. The production validator will still re-query the live descriptor and verify `load_reserved_addr/load_reserved_size` against it before accepting the GET reservation; any mismatch fails closed before the RRQ/DATA write path.

## 5. P6-A gate result

| Gate | Result | Evidence |
|---|---|---|
| Running image corresponds to P6-1C target build | PASS (timestamp correlation) | UART version time matches Actions target build window |
| 8 GiB runtime DRAM size | PASS | `bdinfo` |
| Live `__tmp_load` exists and has expected name/base/size | PASS | `namedprint`, `pa220 memory` |
| `loadaddr` equals descriptor base | PASS | `printenv`, `pa220 memory` |
| Descriptor lies in actual DDR, not boot-bus hole | PASS | live base plus fixed CN70XX DDR decode/validator |
| Known reserved blocks do not overlap descriptor | PASS | `namedprint`, `freeprint`, checked half-open intervals |
| `autoload=n` | PASS | `printenv` |
| 32 MiB complete-file reservation fits | PASS | arithmetic against live 256 MiB descriptor |
| Runtime legacy-global consistency | ENFORCED AT GET GATE | P6-1B reservation calls the production validator, which compares both globals with the live descriptor before any RRQ/data receive |

**Conclusion:** P6-A read-only DRAM/descriptor/reservation preflight is accepted. The descriptor and load address match the fixed-source automatic 8 GiB layout; the free/reserved map is internally consistent. The production GET gate remains the final runtime assertion for the legacy reservation globals and will fail closed if they do not match.

## 6. Explicitly not performed

- No TFTP GET, TFTP PUT, Server/WRQ or HTTP upload.
- No `setenv`, `saveenv`, `tftpboot`, `tftpsrv`, `tftpput`, `bootm`, `bootelf`, `booti`.
- No SPI/eMMC/NAND/EEPROM/CPLD/flash writes or erases.
- No reset, reboot or power-cycle in this round.

P6-B RAM-only TFTP GET remains a separate test and requires explicit confirmation before it is started. The current absence of `ipaddr` and `serverip` is not a P6-A failure; they are network-client settings for the device's later TFTP GET and are unrelated to the final DHCP-server requirement for allocating an address to the maintenance PC.
