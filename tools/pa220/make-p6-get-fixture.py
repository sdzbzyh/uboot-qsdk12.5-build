#!/usr/bin/env python3
"""Generate the deterministic 4 KiB PA-220 P6 TFTP GET fixture.
Usage: python3 tools/pa220/make-p6-get-fixture.py [output-path]
RAM-only transfer/integrity test data; never boot or execute this file.
"""
import hashlib
import struct
import sys
import zlib
from pathlib import Path

OUTPUT = Path(sys.argv[1]) if len(sys.argv) > 1 else Path("pa220-p6-get-4096.bin")
SIZE = 4032
LINE = b"PA220-P6-TFTP-GET-TEST\n"
EXPECTED_HEADER_CRC = 0xB1AA12E3
EXPECTED_PAYLOAD_CRC = 0x705C16E0
EXPECTED_FILE_CRC = 0x9E4DF6C8
EXPECTED_SHA256 = "83b1bb6b9c006c9d38c6915a985895bfeffd41f24d969aae1b74365041028082"

def crc32(data):
    return zlib.crc32(data) & 0xFFFFFFFF

payload = (LINE * ((SIZE + len(LINE) - 1) // len(LINE)))[:SIZE]
header0 = struct.pack(
    ">7I4B32s",
    0x27051956, 0, 0, SIZE, 0, 0, crc32(payload),
    5, 5, 3, 0, b"PA220-P6-GET-TEST".ljust(32, b"\0"),
)
header_crc = crc32(header0)
header = bytearray(header0)
struct.pack_into(">I", header, 4, header_crc)
image = bytes(header) + payload
digest = hashlib.sha256(image).hexdigest()

assert len(header) == 64 and len(image) == 4096
assert header_crc == EXPECTED_HEADER_CRC, hex(header_crc)
assert crc32(payload) == EXPECTED_PAYLOAD_CRC, hex(crc32(payload))
assert crc32(image) == EXPECTED_FILE_CRC, hex(crc32(image))
assert digest == EXPECTED_SHA256, digest

OUTPUT.write_bytes(image)
print(f"output={OUTPUT}")
print(f"size={len(image)}")
print(f"header_crc32={header_crc:08x}")
print(f"payload_crc32={crc32(payload):08x}")
print(f"file_crc32={crc32(image):08x}")
print(f"sha256={digest}")
