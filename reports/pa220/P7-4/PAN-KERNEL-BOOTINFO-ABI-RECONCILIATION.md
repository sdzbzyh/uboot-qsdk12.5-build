# PA-220 PAN kernel bootinfo ABI reconciliation

Date: 2026-11-28

## Evidence

The exact exported PAN-OS kernel ELF is stored at:
artifacts/pa220/original-kernel/pa220-original-vmlinux

- Size: 10,387,920 bytes
- SHA-256: eff6e9d16cf11144642404ddd010bb9d1f62616df185460491a9cd1fd7639e6d
- Build ID: b305d1bd17f2baaf2776ace2712707b47a73a3ae

Its compiled MIPS64 code reads the bootinfo fields at these offsets:

| Field | PAN kernel offset |
|---|---:|
| major_version | 0 |
| minor_version | 4 |
| legacy core_mask | 52 |
| PAN extension | 152..175 |
| fdt_addr | 176 |
| ext_core_mask | 184..311 |

The pinned U-Boot source uses the generic SDK 5.1.0 structure:

| Field | Generic U-Boot offset |
|---|---:|
| major_version | 0 |
| minor_version | 4 |
| legacy core_mask | 52 |
| fdt_addr | 152 |
| ext_core_mask | 160..287 |
| structure size | 288 |

This is a 24-byte layout mismatch after config_flags.

## Symptom correlation

The kernel's prom_init reads bytes 152, 153 and 154 as a PAN-specific extension, then copies 20 bytes from 155..174. It uses byte 153 to append cpuid=N to the kernel command line.

The current U-Boot writes fdt_addr=0x0fff2da0 at offset 152. Its first two big-endian bytes are 00 00, which explains why the kernel independently generates cpuid=0 even though that argument was not supplied by the user.

The kernel's device_tree_init reads fdt_addr at offset 176. With the generic U-Boot layout, that location falls inside the extended coremask and is zero for the current four-core mask, so the kernel selects its internal device tree.

The kernel's prom_init reads ext_core_mask at offset 184. With the generic U-Boot layout, this is also shifted by 24 bytes, so the kernel receives the wrong coremask data.

This single ABI mismatch explains the unexpected cpuid=0, internal-FDT fallback, and likely one-core-only behavior.

## Correction to earlier investigation

An earlier note attributed an unpopulated ext_core_mask to the misspelled CVMX_BOOTINFO_MIN_VERR guard. That was incomplete: the normal non-hplug boot path already copies pcm into cvmx_bootinfo_array[core].ext_core_mask before the later guarded refresh. The current failure is the 24-byte serialization offset mismatch, not simply an absent source mask.

## Compatibility serialization being built

The new PA-220 Linux handoff copy keeps ABI version 1.4 and serializes:

- bytes 0..151: generic bootinfo prefix, including legacy core_mask
- bytes 152..175: PAN extension (profile 0, cpuid=physical core number, UART 0, board name PA-220)
- bytes 176..183: FDT physical address
- bytes 184..311: 128-byte extended coremask, rebuilt from the validated legacy core_mask

The extended mask is rebuilt from core_mask so unused high bits from the newer SDK cannot leak into the PAN 3.10.87 kernel. The generic in-memory U-Boot structure is not changed; only the kernel-visible copy uses the PAN layout.

The profile byte is set to zero because the currently observed kernel path already proceeds with zero in that field. The extension's exact historical field names are inferred from the kernel's compiled reads; the physical boot test remains the final validation.

## OEM SPI comparison status

The original SPI image contains a flat Stage-3 U-Boot payload at SPI offset 0x100000, size 1,310,920 bytes, and the OEM PA-220 FDT at 0x1c9460. The payload is not an ELF with symbols. A raw MIPS64 scan has been archived, but it has not yet provided symbol-level confirmation of the OEM bootinfo writer. The compatibility layout above is grounded in the exact original kernel's compiled field accesses.

## Next physical test

Load the new image to RAM only. Do not write SPI flash.

    bootoctlinux 0x22000000 numcores=4 endbootargs console=ttyS0,115200 root=/dev/mmcblk0p3 rootfstype=ext3 rootdelay=10

Do not include nousb in this test; USB must be allowed to initialize with the passed PA-220 FDT.

Expected first checks:

1. U-Boot prints PA220-PAN-ABI with size 312, FDT offset 176, ext mask offset 184, FDT address 0x0fff2da0, and both core masks equal to 0xf.
2. Linux no longer prints Using internal Device Tree.
3. Linux enumerates four CPUs.
4. The PA-220 USB controller initializes without the previous legacy-controller Oops.
5. eMMC exposes /dev/mmcblk0 and can mount partition 3.

If the PA-220 MMC host initializes but sustained reads still stall, evaluate the separate known DDR52/eMMC issue and the cavium,no-ddr workaround. Do not mix that second fix into this ABI test.

The next round is RAM-only. No SPI flash writes are part of this phase.


## Cloud build and allocation correction (2026-11-28)

- Corrected build completed: Actions #46134874478 SUCCESS, Artifact 15258983126.
- Final RAM-test image: `artifacts/pa220/builds/pa220-pan-kernel-abi-compat.bin`, 1,402,792 bytes, SHA-256 `318c319b824603ee3080ab12687e940ec317c9216566b1986ea8a9bde9048d1d`.
- The patch reserves 312 bytes for both hplug and non-hplug paths. The earlier build #46134530956 is not valid for testing because the non-hplug path retained a 288-byte reservation; do not use that artifact.
- This is a kernel-ABI-derived compatibility layer, not yet a symbol-level reproduction of the OEM U-Boot writer. The original OEM Stage-3 is a flat binary without ELF symbols.
- Physical RAM test remains pending. Use the corrected image only; do not write SPI.
