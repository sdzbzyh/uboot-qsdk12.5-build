# PA-220 original PAN-OS kernel: offline ELF inspection

## Scope and provenance

Read-only analysis of the user-uploaded file `pa220-original-vmlinux`. No device commands were issued and no eMMC/SPI content was modified.

- File size: **10,387,920 bytes** (`0x9e81d0`)
- SHA-256: `eff6e9d16cf11144642404ddd010bb9d1f62616df185460491a9cd1fd7639e6d`
- File identification: ELF 64-bit MSB executable, MIPS, MIPS64 rel2, statically linked, not stripped
- ELF type: `ET_EXEC`
- ELF flags: `noreorder, octeon2, mips64r2`
- Build ID: `b305d1bd17f2baaf2776ace2712707b47a73a3ae`
- Kernel release string: `3.10.87-oct3-mp SMP mod_unload modversions OCTEON 64BIT`

## ELF layout

- Entry point: `0xffffffff80623fa0` (symbol `kernel_entry`)
- `_text`: `0xffffffff80100000`
- `_stext`: `0xffffffff80100400`
- `_end`: `0xffffffff80952640`
- One PT_LOAD segment:
  - File offset: `0x001000`
  - Virtual address: `0xffffffff80100000`
  - Physical address: `0xffffffff80100000`
  - File size: `0x7b0a80`
  - Memory size: `0x852640`
  - Flags: RWE
  - Alignment: `0x1000`
- The PT_LOAD memory image ends at `0xffffffff80952640`; the extra `0xa1bc0` bytes beyond file payload are zero-fill (BSS).
- ELF contains 27 section headers. Main alloc sections include .text, .rodata, .data, .init.text, .init.data, .data..percpu and .bss.

**Loading implication:** this is a linked MIPS64 Octeon kernel ELF, not a flat binary or uImage. The ELF entry and segment addresses are kernel virtual/KSEG addresses. Do not substitute the ELF entry as a TFTP/load address or assume that a generic raw `bootm` path is appropriate. The PA-220 U-Boot `bootoctlinux` implementation and its exact argument/ELF-loading semantics must be checked before issuing the boot command.

## Kernel capability evidence

Strings and symbols confirm:
- `kernel_entry`, `prom_init`, `setup_arch`, `start_kernel`, `arcs_cmdline`, `boot_command_line`, and `saved_command_line` are present.
- Generic kernel parameters `root=`, `rootfstype=`, `rootflags=`, `rootwait`, `init=`, `rdinit=`, and `console=` are present.
- `mmcblk` driver strings are present.
- ext2 and ext3 filesystem implementation strings are present.
- The kernel contains the diagnostic string `No init found. Try passing init= option to kernel`.

This supports that the kernel has code paths for MMC block devices, ext2/ext3 and the proposed generic boot parameters. It does **not** prove that the live eMMC controller/partition is enumerated successfully during this kernel boot, or that a particular init path is present on p3.

## Important command-line caveat

The image contains these strings in its Octeon setup-related read-only data:
- `console=pci`
- `console=`
- ` console=ttyS0,115200`
- ` rw root=1f00 slram=root,0x40000000,+1073741824`

The last string resembles an older/default Octeon command-line fragment. A printable string alone does not establish whether it is an active built-in command line, a fallback, or an unused setup string. The kernel also has the standard `arcs_cmdline` and `boot_command_line` buffers. Therefore, the final command line must be verified from the bootloader handoff/source behavior or the kernel's early boot output; do not assume that the supplied arguments replace or append to any default.

No explicit `numcores=4`, `root=/dev/mmcblk0p3`, `root=LABEL=`, or `endbootargs` literal was found as a complete embedded boot-argument string. This is not evidence that those parameters are unsupported: these are ordinarily passed by the bootloader.

## Diagnostic path status

Previously established device observations (P7-3B):
- eMMC `mmc 0:3` is labelled `sysroot1`; its `/etc/fstab` names `LABEL=sysroot1` as root.
- The kernel file is `/boot/vmlinux-3.10.87-oct3-mp`.
- `/boot/vmlinux.oct3-mp` is a 23-byte symlink, not the kernel ELF.
- The old bootcmd's p1 lookup failed; p3 contains the actual PAN-OS root filesystem.

Proposed controlled parameters remain:
`root=/dev/mmcblk0p3 rootfstype=ext3 rootflags=ro,noload ro rootwait init=/bin/sh`

The use of `/dev/mmcblk0p3` avoids dependence on root-label resolution. `ro,noload` is intended to prevent ext3 journal replay. This is still a proposed diagnostic configuration, not yet proven on this kernel/device combination.

## Next minimal device check

Before loading or starting the kernel, perform only this read-only directory listing at the U-Boot prompt:

```
ext2ls mmc 0:3 /bin
```

Confirm whether `sh` exists (and whether it is a symlink; if needed inspect its target with the available read-only ext filesystem commands). Do not execute `tftpput`, repeat the kernel export, mount the filesystem read-write, run repair tools, or invoke the factory installer.

After confirming the shell path, inspect the exact PA-220 `bootoctlinux` implementation/usage and handoff argument order, then formulate a RAM-only boot attempt. Keep `numcores=4` if required by the established OEM handoff, but verify its placement relative to `endbootargs` rather than guessing.

## Stop conditions

Stop if the kernel fails to enumerate eMMC/p3, read-only ext3 mount fails, filesystem errors appear, or `/bin/sh` cannot be executed. No filesystem repair, read-write remount, partition changes, SPI/eMMC writes, `saveenv`, or factory installation is part of this diagnostic.

