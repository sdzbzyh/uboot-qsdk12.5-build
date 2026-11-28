# PA-220 original PAN-OS kernel artifact

This directory is reserved for the original PA-220 PAN-OS kernel exported from the device's eMMC.

## Upload

Place the already-exported, unmodified kernel ELF here using its original filename:

`vmlinux-3.10.87-oct3-mp`

Expected file size from the live eMMC listing: **10,387,920 bytes** (`0x9e81d0`).

Do not upload the 23-byte `vmlinux.oct3-mp` symlink entry in place of the kernel. The actual ELF was read from:

- eMMC partition: `mmc 0:3`
- path: `/boot/vmlinux-3.10.87-oct3-mp`
- format: ELF64, big-endian, MIPS

After upload, record its SHA-256 in the accompanying analysis report. Keep the binary unchanged; perform all inspection on a copy or read-only.

## Intended analysis

- Verify the uploaded artifact's exact size and SHA-256.
- Inspect ELF header, program headers, section headers, entry point and load segments.
- Search for embedded kernel command-line/default root selection strings and relevant OCTEON boot conventions.
- Determine the correct RAM-only `bootoctlinux` invocation for the existing `sysroot1` root filesystem.

This artifact is for offline analysis only. It does not authorize booting, modifying, or writing the eMMC or SPI flash.
