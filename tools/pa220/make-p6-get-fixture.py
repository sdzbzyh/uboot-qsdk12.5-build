#!/usr/bin/env python3
"""Generate deterministic PA-220 P6 TFTP GET fixtures.

Examples:
  python3 tools/pa220/make-p6-get-fixture.py
  python3 tools/pa220/make-p6-get-fixture.py --size 33554432
  python3 tools/pa220/make-p6-get-fixture.py --size 33554433

The size includes the 64-byte legacy uImage header. Payload is inert ASCII.
These files are RAM-only transfer tests; never boot or execute them.
"""
import argparse
import hashlib
import struct
import zlib
from pathlib import Path

MAX_FILE_SIZE = 32 * 1024 * 1024
LINE = b"PA220-P6-TFTP-GET-TEST\n"
EXPECTED_4K = {
    "header_crc32": 0xB1AA12E3,
    "payload_crc32": 0x705C16E0,
    "file_crc32": 0x9E4DF6C8,
    "sha256": "83b1bb6b9c006c9d38c6915a985895bfeffd41f24d969aae1b74365041028082",
}

def crc32(data):
    return zlib.crc32(data) & 0xFFFFFFFF

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument("--size", type=int, default=4096,
                    help="total file size including 64-byte uImage header (default: 4096)")
parser.add_argument("--output", type=Path)
args = parser.parse_args()

if args.size < 65 or args.size > MAX_FILE_SIZE + 1:
    parser.error(f"--size must be between 65 and {MAX_FILE_SIZE + 1}")
payload_size = args.size - 64
output = args.output or Path(f"pa220-p6-get-{args.size}.bin")
payload = (LINE * ((payload_size + len(LINE) - 1) // len(LINE)))[:payload_size]

header0 = struct.pack(
    ">7I4B32s",
    0x27051956, 0, 0, payload_size, 0, 0, crc32(payload),
    5, 5, 3, 0, b"PA220-P6-GET-TEST".ljust(32, b"\0"),
)
header_crc = crc32(header0)
header = bytearray(header0)
struct.pack_into(">I", header, 4, header_crc)
image = bytes(header) + payload
digest = hashlib.sha256(image).hexdigest()
file_crc = crc32(image)

assert len(header) == 64 and len(image) == args.size
if args.size == 4096:
    assert header_crc == EXPECTED_4K["header_crc32"], hex(header_crc)
    assert crc32(payload) == EXPECTED_4K["payload_crc32"], hex(crc32(payload))
    assert file_crc == EXPECTED_4K["file_crc32"], hex(file_crc)
    assert digest == EXPECTED_4K["sha256"], digest

output.write_bytes(image)
print(f"output={output}")
print(f"size={len(image)}")
print(f"payload_size={payload_size}")
print(f"header_crc32={header_crc:08x}")
print(f"payload_crc32={crc32(payload):08x}")
print(f"file_crc32={file_crc:08x}")
print(f"sha256={digest}")
