# P5-2B fixed memory accounting
- Maximum caller-owned HTTP TX buffer: 1460 bytes.
- Handler body workspace: 1268 bytes, fixed-size stack array.
- Response header workspace: 192 bytes, fixed-size stack array in the response builder.
- Combined nested body/header scratch: 1460 bytes, excluding small scalar/local metadata.
- U-Boot snapshot structure: 184 bytes on the target ABI, measured by a target ELF symbol.
- No writable global response cache and no heap allocator dependency; all request/response storage is caller-owned or fixed-size stack storage.
- Compiler stack-usage reports are archived as 33-http_handler-stack-usage.su and 34-http_handler_uboot-stack-usage.su.
