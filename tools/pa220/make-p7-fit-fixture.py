#!/usr/bin/env python3
"""Build deterministic, inert FIT blobs for PA-220 read-only preview testing."""
import argparse
import hashlib
import pathlib
import struct
import zlib

def u32(v):
    return struct.pack(">I", v & 0xffffffff)

class Fdt:
    def __init__(self):
        self.struct = bytearray()
        self.strings = bytearray()
        self.nameoffs = {}

    def nameoff(self, name):
        if name not in self.nameoffs:
            self.nameoffs[name] = len(self.strings)
            self.strings.extend(name.encode("ascii") + b"\0")
        return self.nameoffs[name]

    def begin(self, name):
        self.struct.extend(u32(1))
        self.struct.extend(name.encode("ascii") + b"\0")
        self.struct.extend(b"\0" * ((-len(self.struct)) & 3))

    def end(self):
        self.struct.extend(u32(2))

    def prop(self, name, value):
        self.struct.extend(u32(3))
        self.struct.extend(u32(len(value)))
        self.struct.extend(u32(self.nameoff(name)))
        self.struct.extend(value)
        self.struct.extend(b"\0" * ((-len(self.struct)) & 3))

    def string(self, name, value):
        self.prop(name, value.encode("ascii") + b"\0")

    def finish(self):
        self.struct.extend(u32(9))
        reserve = b"\0" * 16
        off_rsv = 40
        off_struct = off_rsv + len(reserve)
        off_strings = off_struct + len(self.struct)
        total = off_strings + len(self.strings)
        header = struct.pack(">10I", 0xd00dfeed, total, off_struct,
                             off_strings, off_rsv, 17, 16, 0,
                             len(self.strings), len(self.struct))
        return header + reserve + self.struct + self.strings

def add_hash_node(f, name, data, algo, value):
    f.begin(name)
    f.string("algo", algo)
    f.prop("value", value)
    f.end()

def add_image(f, name, data, image_type, include_hashes=False,
              hash_fail=False, external=False):
    f.begin(name)
    f.string("description", "PA-220 inert FIT preview fixture")
    if external:
        f.prop("data-position", u32(0x1000))
    else:
        payload = bytearray(data)
        if hash_fail:
            payload[0] ^= 0x80
        f.prop("data", bytes(payload))
    f.string("type", image_type)
    f.string("arch", "mips")
    f.string("os", "linux")
    f.string("compression", "none")
    f.prop("load", u32(0x81000000))
    f.prop("entry", u32(0x81000000))
    if include_hashes and not external:
        add_hash_node(f, "hash-crc32", data, "crc32",
                      struct.pack(">I", zlib.crc32(data) & 0xffffffff))
        add_hash_node(f, "hash-sha1", data, "sha1",
                      hashlib.sha1(data).digest())
        add_hash_node(f, "hash-md5", data, "md5",
                      hashlib.md5(data).digest())
        f.begin("hash-ignored")
        f.string("algo", "sha1")
        f.prop("value", hashlib.sha1(data).digest())
        f.prop("uboot-ignore", u32(1))
        f.end()
        add_hash_node(f, "hash-sha256-unsupported", data, "sha256",
                      hashlib.sha256(data).digest())
        f.begin("signature-preview-only")
        f.string("algo", "sha256,rsa2048")
        f.string("key-name-hint", "fixture-key")
        f.prop("value", b"\x5a" * 32)
        f.end()
    f.end()

def build(hash_fail=False):
    f = Fdt()
    f.begin("")
    f.string("description", "PA-220 P7-2B FIT read-only fixture")
    f.prop("timestamp", u32(1795766400))
    f.begin("images")
    kernel = bytes((i * 17 + 3) & 0xff for i in range(256))
    ramdisk = b"PA220-INERT-RAMDISK-FIXTURE\n"
    fdt_blob = b"PA220-INERT-FDT-PAYLOAD\n"
    add_image(f, "kernel-1", kernel, "kernel", True, hash_fail)
    add_image(f, "ramdisk-1", ramdisk, "ramdisk", True)
    add_image(f, "fdt-1", fdt_blob, "flat_dt", False)
    add_image(f, "external-1", b"", "firmware", False, external=True)
    f.end()
    f.begin("configurations")
    f.string("default", "conf-1")
    f.begin("conf-1")
    f.string("kernel", "kernel-1")
    f.string("ramdisk", "ramdisk-1")
    f.string("fdt", "fdt-1")
    f.end()
    f.end()
    f.end()
    return f.finish()

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--output-dir", required=True)
    args = ap.parse_args()
    out = pathlib.Path(args.output_dir)
    out.mkdir(parents=True, exist_ok=True)
    for name, bad in (("pa220-p7-fit-preview.itb", False),
                      ("pa220-p7-fit-hash-fail.itb", True)):
        data = build(bad)
        path = out / name
        path.write_bytes(data)
        print("%s SIZE=%d SHA256=%s" %
              (name, len(data), hashlib.sha256(data).hexdigest()))
    print("FIXTURE_EXPECTATIONS=crc32/sha1/md5 PASS; ignored hash IGNORED; sha256 UNSUPPORTED; signature NOT_VERIFIED; external data UNSUPPORTED")
    print("HASH_FAIL_FIXTURE=kernel sha1/crc32/md5 FAIL after payload mutation")
    print("EXECUTION=NONE; payloads are inert metadata/test bytes")
if __name__ == "__main__":
    main()
