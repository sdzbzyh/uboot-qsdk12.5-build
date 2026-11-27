#!/usr/bin/env python3
"""Read-only structural inventory of the PA-220 SPI NOR backup.

This tool never modifies the input image. It reports cryptographically
validated OCTEON bootloader headers and bounded embedded uImage/FDT markers.
"""
import argparse
import hashlib
import json
import re
import struct
import zlib
from pathlib import Path

BOOT_MAGIC = b"BOOT"
UIMAGE_MAGIC = b"\x27\x05\x19\x56"
FDT_MAGIC = b"\xd0\x0d\xfe\xed"
SECTOR = 0x1000
ERASE_BLOCK = 0x10000
MAX_OCTEON_IMAGE = 0x200000

def crc32(data):
    return zlib.crc32(data) & 0xffffffff

def octeon_headers(buf):
    out = []
    pos = 0
    while True:
        hit = buf.find(BOOT_MAGIC, pos)
        if hit < 0:
            break
        off = hit - 8
        pos = hit + 1
        if off < 0 or off + 192 > len(buf):
            continue
        try:
            nop, = struct.unpack_from(">I", buf, off + 4)
            magic, hcrc = struct.unpack_from(">II", buf, off + 8)
            hlen, major, minor, board = struct.unpack_from(">HHHH", buf, off + 16)
            dlen, dcrc = struct.unpack_from(">II", buf, off + 24)
            address, = struct.unpack_from(">Q", buf, off + 32)
            flags, image_type = struct.unpack_from(">IH", buf, off + 40)
        except struct.error:
            continue
        rec = {
            "offset": off, "magic": f"0x{magic:08x}", "nop": nop,
            "header_length": hlen, "header_version": f"{major}.{minor}",
            "board_type": board, "data_length": dlen,
            "declared_total": hlen + dlen, "link_address": f"0x{address:016x}",
            "flags": flags, "image_type": image_type,
            "stored_header_crc": f"0x{hcrc:08x}", "stored_data_crc": f"0x{dcrc:08x}"
        }
        if (nop == 0 and 192 <= hlen <= 0x200 and major == 1 and minor >= 1
                and dlen <= MAX_OCTEON_IMAGE and hlen + dlen <= MAX_OCTEON_IMAGE
                and off + hlen + dlen <= len(buf)):
            calc_hcrc = crc32(buf[off:off+12] + buf[off+16:off+hlen])
            payload = buf[off+hlen:off+hlen+dlen]
            calc_dcrc = crc32(payload)
            rec["calculated_header_crc"] = f"0x{calc_hcrc:08x}"
            rec["calculated_data_crc"] = f"0x{calc_dcrc:08x}"
            rec["header_crc_pass"] = calc_hcrc == hcrc
            rec["data_crc_pass"] = calc_dcrc == dcrc
            rec["bounds_pass"] = True
            rec["fully_valid"] = (rec["header_crc_pass"] and rec["data_crc_pass"]
                                  and 1 <= image_type <= 7)
        else:
            rec["bounds_pass"] = False
            rec["fully_valid"] = False
        out.append(rec)
    return out

def embedded_uimages(buf):
    out = []
    pos = 0
    while True:
        off = buf.find(UIMAGE_MAGIC, pos)
        if off < 0:
            break
        pos = off + 1
        if off + 64 > len(buf):
            continue
        magic, hcrc, timestamp, size, load, entry, dcrc = struct.unpack_from(">7I", buf, off)
        if size > len(buf) - off - 64:
            continue
        hdr = bytearray(buf[off:off+64])
        hdr[4:8] = b"\0" * 4
        calc_h = crc32(hdr)
        calc_d = crc32(buf[off+64:off+64+size])
        if calc_h == hcrc and calc_d == dcrc:
            out.append({"offset":off,"size":64+size,"payload_size":size,
                        "load":f"0x{load:08x}","entry":f"0x{entry:08x}",
                        "header_crc_pass":True,"data_crc_pass":True})
    return out

def embedded_fdts(buf):
    out = []
    pos = 0
    while True:
        off = buf.find(FDT_MAGIC, pos)
        if off < 0:
            break
        pos = off + 1
        if off + 40 > len(buf):
            continue
        total, struct_off, strings_off, rsv_off, version, last_comp = struct.unpack_from(">6I", buf, off+4)
        strings_size, struct_size = struct.unpack_from(">2I", buf, off+32)
        if (40 <= total <= len(buf)-off and 16 <= version <= 17
                and 16 <= last_comp <= version and rsv_off % 8 == 0
                and 40 <= rsv_off <= total-16 and 40 <= struct_off <= total
                and 40 <= strings_off <= total
                and struct_off + struct_size <= total
                and strings_off + strings_size <= total):
            out.append({"offset":off,"totalsize":total,"version":version,
                        "struct_offset":struct_off,"struct_size":struct_size,
                        "strings_offset":strings_off,"strings_size":strings_size,
                        "reserve_map_offset":rsv_off})
    return out[:128]

def erased_runs(buf, unit, fill):
    blocks = []
    count = len(buf) // unit
    for i in range(count):
        chunk = buf[i*unit:(i+1)*unit]
        if chunk and all(b == fill for b in chunk):
            blocks.append(i)
    runs = []
    for idx in blocks:
        if not runs or idx != runs[-1][1] + 1:
            runs.append([idx, idx])
        else:
            runs[-1][1] = idx
    return [{"start":a*unit,"end_exclusive":(b+1)*unit,"length":(b-a+1)*unit}
            for a,b in runs]

def interesting_strings(buf):
    pattern = re.compile(rb"[ -~]{6,}")
    terms = re.compile(rb"(PA.?220|PA.?200|OCTEON|CAVIUM|BOOT|STAGE|U-BOOT|UBOOT|SPI|NOR|NAND|EMMC|MMC|ENV|CPLD|PALO ALTO|OPENWRT)", re.I)
    found = []
    for m in pattern.finditer(buf):
        if terms.search(m.group(0)):
            found.append({"offset":m.start(),"text":m.group(0)[:180].decode("ascii","replace")})
    return found[:500]

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--image", required=True)
    ap.add_argument("--json", required=True)
    ap.add_argument("--markdown", required=True)
    args = ap.parse_args()
    path = Path(args.image)
    data = path.read_bytes()
    headers = octeon_headers(data)
    valid = [x for x in headers if x["fully_valid"]]
    uimgs = embedded_uimages(data)
    fdts = embedded_fdts(data)
    report = {
        "input": path.name, "size_bytes":len(data),
        "sha256":hashlib.sha256(data).hexdigest(),
        "whole_image_crc32":f"0x{crc32(data):08x}",
        "size_matches_16MiB":len(data)==16*1024*1024,
        "sector_size_assumed_for_inventory":SECTOR,
        "erase_block_size_assumed_for_inventory":ERASE_BLOCK,
        "octeon_header_candidates":headers,
        "fully_valid_octeon_headers":valid,
        "validated_embedded_uimages":uimgs,
        "bounded_embedded_fdts":fdts,
        "all_ff_4k_runs":erased_runs(data,SECTOR,0xff),
        "all_zero_4k_runs":erased_runs(data,SECTOR,0x00),
        "all_ff_64k_runs":erased_runs(data,ERASE_BLOCK,0xff),
        "interesting_ascii_strings":interesting_strings(data),
        "limitations":[
            "This is a byte-level SPI NOR inventory, not proof of the complete silicon/OEM boot sequence.",
            "No eMMC/GPT conclusion is derived from this SPI image.",
            "A CRC-valid OCTEON header proves format integrity only, not authenticity or install authorization.",
            "No flash write, erase, updater, or device operation is performed."
        ]
    }
    Path(args.json).write_text(json.dumps(report,indent=2)+"\n",encoding="utf-8")
    lines = [
        "# PA-220 SPI NOR image: read-only structural inventory",
        "",
        f"- Input: `{path.name}`",
        f"- Size: {len(data)} bytes (0x{len(data):x})",
        f"- SHA-256: `{report['sha256']}`",
        f"- Whole-image CRC32: `{report['whole_image_crc32']}`",
        f"- OCTEON header candidates: {len(headers)}; fully valid: {len(valid)}",
        f"- CRC-valid embedded legacy uImages: {len(uimgs)}",
        f"- Bounded embedded FDTs: {len(fdts)}",
        "",
        "## OCTEON headers",
        "",
        "| SPI offset | Type | Board | Header | Data | Link address | Header CRC | Data CRC |",
        "|---:|---:|---:|---:|---:|---:|---|---|"
    ]
    for h in headers:
        lines.append(f"| 0x{h['offset']:06x} | {h['image_type']} | {h['board_type']} | 0x{h['header_length']:x} | 0x{h['data_length']:x} | {h['link_address']} | {h.get('header_crc_pass',False)} | {h.get('data_crc_pass',False)} |")
    lines += ["","## CRC-valid embedded legacy uImages","","| SPI offset | Total size | Payload | Load | Entry |","|---:|---:|---:|---:|---:|"]
    for x in uimgs:
        lines.append(f"| 0x{x['offset']:06x} | 0x{x['size']:x} | 0x{x['payload_size']:x} | {x['load']} | {x['entry']} |")
    lines += ["","## Erased 64 KiB runs (all 0xff)",""]
    for x in report["all_ff_64k_runs"]:
        lines.append(f"- 0x{x['start']:06x}..0x{x['end_exclusive']:06x} ({x['length']} bytes)")
    lines += ["","## Relevant printable strings (first 100)",""]
    for x in report["interesting_ascii_strings"][:100]:
        lines.append(f"- 0x{x['offset']:06x}: `{x['text']}`")
    lines += ["","## Limits",""] + [f"- {x}" for x in report["limitations"]]
    Path(args.markdown).write_text("\n".join(lines)+"\n",encoding="utf-8")
    print("\n".join(lines))

if __name__ == "__main__":
    main()
