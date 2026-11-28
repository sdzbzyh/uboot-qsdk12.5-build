# PA-220 original kernel internal FDT comparison

## Scope and evidence

This report compares the two flattened device trees embedded in the original PA-220 kernel ELF against the original SPI Stage-3 FDT and the FDT embedded in the rebuilt PA-220 U-Boot.

Kernel ELF:
- File: `artifacts/pa220/original-kernel/pa220-original-vmlinux`
- SHA-256: `eff6e9d16cf11144642404ddd010bb9d1f62616df185460491a9cd1fd7639e6d`
- Build ID: `b305d1bd17f2baaf2776ace2712707b47a73a3ae`
- Release: `3.10.87-oct3-mp`

The internal FDTs are located through the ELF symbols `__dtb_octeon_3xxx_begin/end` and `__dtb_octeon_68xx_begin/end` in `.init.data`.

## Kernel-embedded FDTs

| Kernel internal FDT | ELF file offset | Size | Nodes | Root compatible |
|---|---:|---:|---:|---|
| OCTEON 3xxx | `0x7a57e0` | `0x2d51` (11,601) | 68 | `cavium,octeon-3860` |
| OCTEON 68xx | `0x7a8540` | `0x2fad` (12,205) | 73 | `cavium,octeon-6880` |

Neither internal FDT contains an MMC controller node. Both describe older USB hardware:

| Node | Compatible | Register address |
|---|---|---:|
| `/soc@0/usbn@1180068000000` | `cavium,octeon-5750-usbn` | `0x1180068000000` |
| `/soc@0/usbn@1180068000000/usbc@16f0010000000` | `cavium,octeon-5750-usbc` | `0x16f0010000000` |
| `/soc@0/uctl@118006f000000` | `cavium,octeon-6335-uctl` | `0x118006f000000` |
| `/soc@0/uctl@118006f000000/ehci@16f0000000000` | `cavium,octeon-6335-ehci` | `0x16f0000000000` |
| `/soc@0/uctl@118006f000000/ohci@16f0000000400` | `cavium,octeon-6335-ohci` | `0x16f0000000400` |

The 3xxx and 68xx internal trees have the same legacy USB controller layout relevant to the observed failure.

## OEM and rebuilt PA-220 FDTs

The full comparison is recorded in [BOOT-HANDOFF-AUDIT.md](boot-handoff-audit/BOOT-HANDOFF-AUDIT.md).

| Critical device | OEM SPI Stage 3 | Rebuilt U-Boot |
|---|---|---|
| eMMC controller | `cavium,octeon-6130-mmc` at `0x1180000002000` | Same |
| USB UCTL 0 | `cavium,octeon-7130-usb-uctl` at `0x1180068000000` | Same |
| USB xHCI/DWC3 0 | `cavium,octeon-7130-xhci`, `synopsys,dwc3` at `0x1680000000000` | Same |
| USB UCTL 1 | `cavium,octeon-7130-usb-uctl` at `0x1180069000000` | Same |
| USB xHCI/DWC3 1 | `cavium,octeon-7130-xhci`, `synopsys,dwc3` at `0x1690000000000` | Same |

The rebuilt tree adds `pan,pa-220` to root `compatible` while retaining `cavium,tb7100`. The power-GPIO phandle numbers differ because phandles are local references within each tree; the controller path and GPIO line numbers remain the same (MMC line 8, USB lines 2 and 3).

The remaining node-set differences concern board peripherals such as the fan controller, EEPROM, temperature/voltage monitors, and LEDs. They do not explain the missing eMMC or the observed USB controller access.

The original kernel ELF also contains the OF match strings `cavium,octeon-6130-mmc`, `cavium,octeon-7130-usb-uctl`, and `synopsys,dwc3`. This supports that the original kernel has relevant driver/match code; it is not by itself proof that a live probe will succeed.

## Correlation with the physical boot logs

The user's boot logs show:

1. Without `nousb`, the kernel reaches `cvmx_usb_initialize()` and raises a Data Bus Error. The reported access address includes `0x16f0010000000`, matching the legacy USB controller child present in the kernel's internal 3xxx/68xx FDT.
2. With `nousb`, the USB Oops disappears, confirming that disabling USB suppresses that failing probe.
3. The kernel then waits for `/dev/mmcblk0p3`, but neither internal FDT describes the PA-220 eMMC controller. Therefore the MMC driver has no matching platform device to probe.

The two failures are consequently consistent with one common cause: the kernel is selecting its generic internal FDT instead of the PA-220 FDT supplied by U-Boot. This is not a firewall or network restriction.

## Bootinfo/FDT handoff status

Static source and binary inspection shows:
- Current U-Boot advertises bootinfo ABI major/minor `1.4`.
- The original kernel's device-tree selection logic accepts bootinfo minor version `>= 3` when `fdt_addr` is nonzero.
- The current U-Boot binary contains writes for major version, minor version, and the `fdt_addr` field (offset 152 / `0x98` in the bootinfo structure).
- The U-Boot FDT allocation is constrained below `0x20000000`; U-Boot's `cvmx_phys_to_ptr()` uses an identity pointer for physical addresses below `0x80000000`. Thus the source's `new_fdt` pointer is expected to equal the physical address in this allocation range.

Despite this static handoff path, the live kernel prints `Using internal Device Tree.`. The unresolved issue is therefore runtime handoff/bootinfo visibility, not absence of the PA-220 MMC/USB nodes in the rebuilt FDT.

## Next action

A diagnostic-only build has been added behind `PA220_FDT_HANDOFF_DIAGNOSTICS=1`. It prints, immediately after the bootinfo copy:
- compiled ABI version and structure field offsets;
- the in-memory and copied bootinfo major/minor versions and `fdt_addr`;
- the boot descriptor's `cvmx_desc_vaddr`;
- the FDT source/copy addresses, header validation result, and totalsize.

Use those serial values to determine why the kernel does not consume the passed FDT. Keep the first follow-up boot RAM-only and use `nousb` to isolate eMMC enumeration. If the kernel reports the passed PA-220 FDT and eMMC enumerates, repeat without `nousb` to check whether the USB Oops is also resolved.

No SPI/eMMC writes, erases, environment saves, or installer operations are part of this investigation.
