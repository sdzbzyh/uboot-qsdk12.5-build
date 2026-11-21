# PA-220 P5-2A implementation checkpoint

- Base integration commit: 3fc1ec2e6b0cff3bb0950343c63c6f1bd1bc5414
- Fixed PA-220 source: IronDemiGod/pa220-openwrt@801717ba6a8a59ed2cfd53bf3e1b19841fecd2e8
- Scope: independent bounded HTTP/1.x request-head parser only
- TCP core and pa220 web lifecycle: unchanged
- P5-2B URI handler: not implemented
- Host and OCTEON validation: pending GitHub Actions execution

Parser limits are defined in u-boot/include/net/http_request.h. The parser owns a 4096-byte RX array and a fixed structured request object; the TX limit is a compile-time planning constant and is not allocated by this parser. Host tests print the exact parser-structure size. The target workflow compiles one OCTEON object against the pinned source checkout and records its ABI, undefined symbols, size, and SHA-256.
