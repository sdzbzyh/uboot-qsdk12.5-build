#!/usr/bin/env python3
"""Read-only scan and raw disassembly of the OEM PA-220 SPI Stage-3 image."""
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

# SPI layout audit previously identified the OEM Stage-3 payload at 0x100000,
# length 1,310,920 bytes. Verify the embedded PA-220 FDT is inside that range.
stage3_off, stage3_size = 0x100000, 1310920
if stage3_off + stage3_size <= len(blob):
    stage3 = blob[stage3_off:stage3_off + stage3_size]
    stage3_path = Path("/tmp/pa220-oem-stage3.bin")
    stage3_path.write_bytes(stage3)
    lines += [
        "",
        "## OEM Stage-3 raw image",
        "",
        f"- SPI offset: 0x{stage3_off:x}",
        f"- Size: {stage3_size} bytes (0x{stage3_size:x})",
        f"- First 64 bytes: {stage3[:64].hex(' ')}",
        f"- PA-220 FDT relative offset: 0x{0x1c9460 - stage3_off:x}",
        f"- FDT signature inside extracted image: {stage3.find(bytes.fromhex('d00dfeed')) >= 0}",
        "",
    ]
    dis = subprocess.run(
        ["objdump", "-D", "-b", "binary", "-m", "mips:isa64r2", "-EB",
         "--adjust-vma=0xc0000000", str(stage3_path)],
        text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, check=False
    )
    if dis.returncode == 0:
        dlines = dis.stdout.splitlines()
        patterns = re.compile(r"\b(?:ld|sd|lw|sw|lwu|lbu|sb|sh)\s+.*(?:0x(?:98|a0|b0|b8)|(?:152|160|176|184))\(")
        hits = [i for i, line in enumerate(dlines) if patterns.search(line)]
        lines += [
            "### Raw MIPS64 disassembly: bootinfo-related field offsets",
            "",
            f"- Full disassembly instruction lines: {len(dlines)}",
            f"- Direct load/store candidates at offsets 152/160/176/184: {len(hits)}",
            "",
            "~~~asm",
        ]
        emitted = set()
        for idx in hits[:160]:
            for j in range(max(0, idx - 2), min(len(dlines), idx + 3)):
                if j not in emitted:
                    lines.append(dlines[j])
                    emitted.add(j)
            lines.append("")
        if not hits:
            lines.append("(No direct load/store instructions matched these offsets.)")
        lines += ["~~~", ""]
    else:
        lines += ["### Raw MIPS64 disassembly", "", "~~~text", dis.stdout.rstrip(), "~~~", ""]

lines += [
    "## Bootloader strings",
    "",
    "~~~text",
]
for m in re.finditer(rb"(?<![ -~])(?:U-Boot|OCTEON|bootinfo|coremask|fdt_addr|bootoctlinux|PA-220)[ -~]{0,120}", blob):
    raw = m.group(0).decode("ascii", "replace").strip()
    if raw:
        lines.append(f"0x{m.start():x}: {raw}")
lines += [
    "~~~",
    "",
    "## Interpretation limits",
    "",
    "- The OEM Stage-3 is a flat binary in this SPI image, not an ELF with symbols.",
    "- Raw disassembly uses the known Stage-3 image range and a provisional U-Boot text base of 0xc0000000; instruction offsets remain useful even if the linked virtual base differs.",
    "- Direct load/store matches are candidates and require code-context validation before attributing them to bootinfo fields.",
    "- No device access or persistent writes are performed.",
    "",
]
out_path.parent.mkdir(parents=True, exist_ok=True)
out_path.write_text("\n".join(lines))
print(out_path)
