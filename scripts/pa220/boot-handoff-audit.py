#!/usr/bin/env python3
"""Read-only OEM Stage-3 versus rebuilt PA-220 U-Boot FDT audit."""
import argparse
import json
import re
import struct
from pathlib import Path


def parse_fdt(blob, offset):
    if offset + 40 > len(blob):
        raise ValueError("header outside input")
    h = struct.unpack_from(">10I", blob, offset)
    magic, total, os, ostr, ores, version, last, bootid, ss, st = h
    if magic != 0xD00DFEED or total < 40 or offset + total > len(blob):
        raise ValueError("invalid FDT header")
    if os + st > total or ostr + ss > total:
        raise ValueError("invalid FDT block bounds")
    strings = blob[offset + ostr:offset + ostr + ss]
    data = blob[offset + os:offset + os + st]
    nodes, stack, p, ended = {}, [], 0, False
    while p + 4 <= len(data):
        token = struct.unpack_from(">I", data, p)[0]
        p += 4
        if token == 1:
            end = data.find(b"\\0", p)
            if end < 0:
                raise ValueError("unterminated node name")
            name = data[p:end].decode("utf-8", "replace")
            p = (end + 4) & ~3
            stack.append(name)
            path = "/" + "/".join(x for x in stack if x)
            nodes[path] = {}
        elif token == 2:
            if not stack:
                raise ValueError("unbalanced end node")
            stack.pop()
        elif token == 3:
            if p + 8 > len(data) or not stack:
                raise ValueError("truncated property")
            length, nameoff = struct.unpack_from(">II", data, p)
            p += 8
            if nameoff >= len(strings) or p + length > len(data):
                raise ValueError("property out of bounds")
            end = strings.find(b"\\0", nameoff)
            if end < 0:
                raise ValueError("unterminated property name")
            name = strings[nameoff:end].decode("utf-8", "replace")
            path = "/" + "/".join(x for x in stack if x)
            nodes[path][name] = data[p:p + length]
            p = (p + length + 3) & ~3
        elif token == 4:
            pass
        elif token == 9:
            ended = True
            break
        else:
            raise ValueError("unknown FDT token")
    if not ended or "/" not in nodes:
        raise ValueError("incomplete FDT")
    return {"offset": offset, "totalsize": total, "version": version, "nodes": nodes}


def scan_fdts(blob):
    found, pos, magic = [], 0, b"\\xd0\\x0d\\xfe\\xed"
    while True:
        pos = blob.find(magic, pos)
        if pos < 0:
            break
        try:
            found.append(parse_fdt(blob, pos))
        except ValueError:
            pass
        pos += 4
    return found


def strings(raw):
    if raw is None:
        return []
    return [x.decode("utf-8", "replace") for x in raw.split(b"\\0") if x]


def display(raw):
    if raw is None:
        return "ABSENT"
    vals = strings(raw)
    if vals:
        return " | ".join(vals)
    if len(raw) % 4 == 0 and len(raw) <= 64:
        return "cells:" + " ".join("%08x" % x for x in struct.unpack(">" + "I" * (len(raw) // 4), raw))
    return "0x" + raw.hex()


def choose(fdts, marker):
    for fdt in fdts:
        root = fdt["nodes"]["/"]
        values = strings(root.get("compatible")) + strings(root.get("model"))
        if any(marker in x for x in values):
            return fdt
    raise RuntimeError("could not find FDT marker " + marker)


def matching_nodes(tree, marker):
    result = []
    for path, props in tree["nodes"].items():
        comp = " | ".join(strings(props.get("compatible")))
        if marker in comp:
            result.append({"path": path, "compatible": comp,
                           "reg": display(props.get("reg")),
                           "status": display(props.get("status"))})
    return result


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--spi", required=True)
    ap.add_argument("--uboot", required=True)
    ap.add_argument("--toolchain", required=True)
    ap.add_argument("--out", required=True)
    a = ap.parse_args()
    spi, uboot = Path(a.spi).read_bytes(), Path(a.uboot).read_bytes()
    oem = choose(scan_fdts(spi), "tb7100")
    current = choose(scan_fdts(uboot), "tb7100")
    old, new = oem["nodes"], current["nodes"]
    added, removed = sorted(set(new) - set(old)), sorted(set(old) - set(new))
    critical = ("compatible", "reg", "status", "bus-width", "max-frequency",
                "non-removable", "no-sdio", "no-sd", "cavium,no-ddr",
                "power-gpios", "power", "refclk-type-ss", "refclk-type-hs")
    diffs = []
    for path in sorted(set(old) & set(new)):
        for key in critical:
            if old[path].get(key) != new[path].get(key):
                diffs.append({"path": path, "property": key,
                              "oem": display(old[path].get(key)),
                              "rebuilt": display(new[path].get(key))})
    versions = []
    for header in Path(a.toolchain).rglob("cvmx-bootinfo.h"):
        try:
            source = header.read_text(errors="replace")
        except OSError:
            continue
        match = re.search(r"^\\s*#\\s*define\\s+CVMX_BOOTINFO_MIN_VER\\s+(\\d+)", source, re.M)
        if match:
            versions.append({"header": str(header), "minor_version": int(match.group(1))})
    report = {
        "spi_size": len(spi), "uboot_size": len(uboot),
        "oem_fdt": {"offset": oem["offset"], "size": oem["totalsize"],
                    "nodes": len(old), "model": display(old["/"].get("model")),
                    "compatible": display(old["/"].get("compatible"))},
        "rebuilt_fdt": {"offset": current["offset"], "size": current["totalsize"],
                        "nodes": len(new), "model": display(new["/"].get("model")),
                        "compatible": display(new["/"].get("compatible"))},
        "nodes_only_in_rebuilt": added, "nodes_only_in_oem": removed,
        "critical_property_differences": diffs,
        "oem_mmc": matching_nodes(oem, "octeon-6130-mmc"),
        "rebuilt_mmc": matching_nodes(current, "octeon-6130-mmc"),
        "oem_usb": matching_nodes(oem, "octeon-7130"),
        "rebuilt_usb": matching_nodes(current, "octeon-7130"),
        "toolchain_bootinfo_versions": versions
    }
    out = Path(a.out)
    out.mkdir(parents=True, exist_ok=True)
    (out / "boot-handoff-audit.json").write_text(json.dumps(report, indent=2) + "\\n")
    lines = [
        "# PA-220 OEM Stage-3 and rebuilt U-Boot FDT comparison", "",
        "Read-only comparison of the original SPI Stage-3 FDT and the FDT embedded in the rebuilt PA-220 U-Boot image.", "",
        "| Field | OEM SPI Stage 3 | Rebuilt U-Boot |",
        "|---|---:|---:|",
        "| FDT offset | 0x%x | 0x%x |" % (oem["offset"], current["offset"]),
        "| FDT size | %d | %d |" % (oem["totalsize"], current["totalsize"]),
        "| Node count | %d | %d |" % (len(old), len(new)),
        "| Model | %s | %s |" % (display(old["/"].get("model")), display(new["/"].get("model"))),
        "| Compatible | %s | %s |" % (display(old["/"].get("compatible")), display(new["/"].get("compatible"))),
        "", "## MMC controller", "",
        "OEM: " + json.dumps(report["oem_mmc"]),
        "Rebuilt: " + json.dumps(report["rebuilt_mmc"]),
        "", "## CN7130 USB controller", "",
        "OEM: " + json.dumps(report["oem_usb"]),
        "Rebuilt: " + json.dumps(report["rebuilt_usb"]),
        "", "## Structural differences", "",
        "Nodes only in rebuilt: " + (", ".join(added) or "none"),
        "Nodes only in OEM: " + (", ".join(removed) or "none"),
        "Critical property differences: %d" % len(diffs)
    ]
    for item in diffs:
        lines.append("- %s %s: OEM=%s; rebuilt=%s" %
                     (item["path"], item["property"], item["oem"], item["rebuilt"]))
    lines += ["", "## Toolchain bootinfo ABI", ""]
    if versions:
        for item in versions:
            lines.append("- %s: CVMX_BOOTINFO_MIN_VER=%d" %
                         (item["header"], item["minor_version"]))
    else:
        lines.append("- cvmx-bootinfo.h not found in extracted toolchain")
    lines += ["", "## Limits", "",
              "- FDT similarity alone does not prove Linux accepted the passed FDT.",
              "- The live kernel message 'Using internal Device Tree.' is separate evidence of fallback selection.",
              "- No device access or SPI/eMMC writes are performed.", "",
              "Full machine-readable inventory: boot-handoff-audit.json", ""]
    (out / "BOOT-HANDOFF-AUDIT.md").write_text("\\n".join(lines))
    print((out / "BOOT-HANDOFF-AUDIT.md").read_text())
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
