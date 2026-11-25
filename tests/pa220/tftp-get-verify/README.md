# P6-1C TFTP GET verification Host Harness

This harness injects the exact P6-1B receive-state helpers and P6-1C verification
helpers exported by the production patch generators. It does not reimplement the
production format recognizer or completion gate.

Run with:

    bash tests/pa220/tftp-get-verify/run.sh

The suite checks legacy uImage, FIT/FDT, ELF32 and ELF64 headers; truncated
headers, declared-length mismatches, FDT bounds, ELF program-header table bounds,
unknown formats, CRC32's IEEE reference vector, cache synchronization, SHA-256
invocation, successful completion, and failure-state metadata invalidation.

SHA-256 is stubbed at the Host boundary to assert the exact production call
contract; the OCTEON build links the pinned U-Boot SHA-256 implementation enabled
by CONFIG_SHA256. CRC32's Host boundary uses the standard IEEE polynomial and
checks the 123456789 reference vector. This harness validates format and lifecycle
logic, not cryptographic implementation internals.


The valid-format completion cases use bounded synthetic headers; the 4 KiB plain-text fixture from the earlier P6-0 proposal is intentionally not a success case because P6-1C rejects unsupported formats. The P6-B first-transfer fixture must be changed to a supported, inert test image before any real receive is considered.
