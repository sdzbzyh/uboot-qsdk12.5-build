#!/usr/bin/env python3
import argparse
import hashlib
import pathlib
import struct
import zlib

def make_wrapper(payload):
    fields = (0x27051956, 0, 0, len(payload), 0x81000000, 0x81000000,
              zlib.crc32(payload) & 0xffffffff, 5, 6, 2, 0,
              b"PA220 STAGE3 RAM WRAPPER".ljust(32, b"\0"))
    header = bytearray(struct.pack(">7I4B32s", *fields))
    hcrc = zlib.crc32(header) & 0xffffffff
    struct.pack_into(">I", header, 4, hcrc)
    return bytes(header) + payload, hcrc, zlib.crc32(payload) & 0xffffffff

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--input", required=True)
    ap.add_argument("--output", required=True)
    args = ap.parse_args()
    payload = pathlib.Path(args.input).read_bytes()
    wrapped, hcrc, dcrc = make_wrapper(payload)
    out = pathlib.Path(args.output)
    out.parent.mkdir(parents=True, exist_ok=True)
    out.write_bytes(wrapped)
    print("FORMAT=legacy-uImage")
    print("WRAPPED_SIZE=%d (0x%x)" % (len(wrapped), len(wrapped)))
    print("PAYLOAD_SIZE=%d (0x%x)" % (len(payload), len(payload)))
    print("HEADER_CRC32=%08x" % hcrc)
    print("PAYLOAD_CRC32=%08x" % dcrc)
    print("WRAPPER_SHA256=%s" % hashlib.sha256(wrapped).hexdigest())
    print("PAYLOAD_SHA256=%s" % hashlib.sha256(payload).hexdigest())
    print("RAM_COPY_SOURCE_OFFSET=0x40")
    print("RAM_COPY_DESTINATION=0x81000000")
    print("NOTE=copy payload only; do not boot/execute the wrapper itself")
if __name__ == "__main__":
    main()
