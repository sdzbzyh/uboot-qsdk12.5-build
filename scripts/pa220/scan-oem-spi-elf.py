#!/usr/bin/env python3
"""Read-only scan of the OEM PA-220 SPI backup for executable ELF payloads."""
import re
import struct
import subprocess
import sys
from pathlib import Path

spi_path = Path(sys.argv[1])
out_path = Path(sys.argv[2])
blob = spi_path.read_bytes()
lines = [
    "# PA-220 OEM SPI executable scan",
    "",
    "Read-only analysis of the original SPI backup. No device access or writes.",
    "",
    f"- SPI file: {spi_path}",
    f"- SPI size: {len(blob)} bytes (0x{len(blob):x})",
    "",
    "## OEM FDT signatures",
    "",
]
pos = 0
fdts = []
while True:
    pos = blob.find(b"\xd0\x0d\xfe\xed", pos)
    if pos < 0:
        break
    if pos + 40 <= len(blob):
        total = struct.unpack_from(">I", blob, pos + 4)[0]
        if 40 <= total <= len(blob) - pos:
            sample = blob[pos:pos + total]
            if b"pan,pa-220" in sample or b"tb7100" in sample:
                fdts.append((pos, total))
    pos += 4
if fdts:
    for off, size in fdts:
        lines.append(f"- FDT at SPI offset 0x{off:x}, size {size} (0x{size:x})")
else:
    lines.append("- No PA-220 FDT signature found")

lines += ["", "## Embedded MIPS64 big-endian ELF candidates", ""]
candidates = []
pos = 0
while True:
    pos = blob.find(b"\x7fELF", pos)
    if pos < 0:
        break
    if pos + 64 <= len(blob):
        ident = blob[pos:pos + 16]
        if ident[4] == 2 and ident[5] == 2 and ident[6] == 1:
            try:
                e_type, e_machine, e_version = struct.unpack_from(">HHI", blob, pos + 16)
                e_phoff, e_shoff = struct.unpack_from(">QQ", blob, pos + 32)
                e_ehsize, e_phentsize, e_phnum, e_shentsize, e_shnum, e_shstrndx = struct.unpack_from(">HHHHHH", blob, pos + 52)
                if e_machine == 8 and e_version == 1 and e_ehsize == 64 and e_phentsize in (0, 56) and e_phnum < 128:
                    end = pos + 64
                    for i in range(e_phnum):
                        ph = pos + e_phoff + i * e_phentsize
                        if ph + 56 > len(blob):
                            raise ValueError("program header outside SPI")
                        p_type, p_flags, p_offset, p_vaddr, p_paddr, p_filesz, p_memsz, p_align = struct.unpack_from(">IIQQQQQQ", blob, ph)
                        if p_type == 1:
                            end = max(end, pos + p_offset + p_filesz)
                    if e_shoff and e_shentsize and e_shnum:
                        end = max(end, pos + e_shoff + e_shentsize * e_shnum)
                    if pos < end <= len(blob):
                        candidates.append((pos, end - pos, e_type, e_machine))
            except (ValueError, struct.error):
                pass
    pos += 4

if not candidates:
    lines.append("- No structurally valid ELF64 big-endian MIPS payload found by signature scan.")
else:
    for idx, (off, size, e_type, e_machine) in enumerate(candidates):
        path = Path(f"/tmp/pa220-oem-elf-{idx}.bin")
        path.write_bytes(blob[off:off + size])
        lines.append(f"- Candidate {idx}: SPI offset 0x{off:x}, size {size} (0x{size:x}), ELF type {e_type}, machine {e_machine}")
        lines += ["", f"### Candidate {idx}: ELF metadata", "", "~~~text"]
        for cmd in (["readelf", "-hW", str(path)], ["readelf", "-lW", str(path)], ["readelf", "-SW", str(path)]):
            result = subprocess.run(cmd, text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, check=False)
            lines.append("$ " + " ".join(cmd))
            lines.append(result.stdout.rstrip())
        nm = subprocess.run(["nm", "-n", str(path)], text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, check=False)
        symbols = [x for x in nm.stdout.splitlines() if re.search(r"(octeon_setup_boot_desc_block|start_linux|cvmx_bootinfo_array|cvmx_desc_vaddr|do_bootoctlinux)", x)]
        lines += ["", "Relevant symbols:", *(symbols or ["(none found; likely stripped)"]), "~~~", ""]
        for sym in ("octeon_setup_boot_desc_block", "start_linux", "do_bootoctlinux"):
            if any(re.search(r"\b" + re.escape(sym) + r"$", x) for x in symbols):
                dis = subprocess.run(["objdump", "-d", f"--disassemble={sym}", str(path)], text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, check=False)
                lines += [f"### Candidate {idx}: {sym}", "", "~~~asm", dis.stdout.rstrip(), "~~~", ""]

lines += [
    "## Bootloader strings",
    "",
    "~~~text",
]
for m in re.finditer(rb"(?<![ -~])(?:U-Boot|OCTEON|bootinfo|coremask|fdt_addr|PA-220)[ -~]{0,120}", blob):
    raw = m.group(0).decode("ascii", "replace").strip()
    if raw:
        lines.append(f"0x{m.start():x}: {raw}")
lines += [
    "~~~",
    "",
    "## Interpretation limits",
    "",
    "- An ELF signature scan can miss a flat or compressed U-Boot image.",
    "- A stripped ELF may not retain function names; absence of symbols is not proof that code is absent.",
    "- The scan does not assume that the FDT's location identifies the start or full extent of Stage-3.",
    "",
]
out_path.parent.mkdir(parents=True, exist_ok=True)
out_path.write_text("\n".join(lines))
print(out_path)
