# P5-2B fixed memory accounting
- Maximum caller-owned HTTP TX buffer: 1460 bytes.
- Handler body workspace: 1268 bytes, fixed-size stack array.
- Response header workspace: 192 bytes, fixed-size stack array in the response builder.
- Target compiler stack-usage report: http_handler_build 1424 bytes plus nested build_response 288 bytes, for a 1712-byte maximum nested handler call path; the snapshot provider uses 32 bytes.
- U-Boot snapshot structure: 184 bytes on the target ABI, measured by a target ELF symbol.
- No writable global response cache and no heap allocator dependency; all request/response storage is caller-owned or fixed-size stack storage.
- Compiler stack-usage reports are archived as 33-http_handler-stack-usage.su and 34-http_handler_uboot-stack-usage.su.
