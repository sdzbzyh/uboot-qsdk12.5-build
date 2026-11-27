#!/usr/bin/env python3
import argparse
import hashlib
import pathlib
import struct
import zlib

def make_fixture():
    payload = bytes(((i * 17 + 3) & 0xff) for i in range(256))
    fields = (0x27051956, 0, 0, len(payload), 0x81000000, 0x81000000,
              zlib.crc32(payload) & 0xffffffff, 5, 6, 2, 0,
              b"PA220 P7 READ-ONLY PREVIEW".ljust(32, b"\0"))
    header = bytearray(struct.pack(">7I4B32s", *fields))
    hcrc = zlib.crc32(header) & 0xffffffff
    struct.pack_into(">I", header, 4, hcrc)
    return bytes(header) + payload, hcrc, zlib.crc32(payload) & 0xffffffff

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--output", required=True)
    args = ap.parse_args()
    data, hcrc, dcrc = make_fixture()
    path = pathlib.Path(args.output)
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_bytes(data)
    print("FORMAT=legacy-uImage")
    print("SIZE=%d" % len(data))
    print("HEADER_CRC32=%08x" % hcrc)
    print("DATA_CRC32=%08x" % dcrc)
    print("SHA256=%s" % hashlib.sha256(data).hexdigest())
    print("LOAD/ENTRY=0x81000000 (metadata only; never executed)")
if __name__ == "__main__":
    main()
