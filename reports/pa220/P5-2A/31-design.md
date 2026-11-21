# PA-220 P5-2A parser design
Standalone C parser, one fixed RX buffer, one HTTP/1.x request head, no dynamic allocation, I/O, TCP calls, URI dispatch, or body support. Each TCP-read chunk is appended once; incomplete bytes remain parser-owned because tcp_core_read consumes its RX queue. A completed request remains stable until explicit reset.
| Resource | Limit |
|---|---:|
| RX buffer | 4096 bytes |
| TX planning limit (not allocated here) | 1460 bytes |
| Request line excluding CRLF | 512 bytes |
| URI | 384 bytes |
| Header count | 16 |
| Header line including CRLF | 256 bytes |
| Aggregate header section including CRLF | 2048 bytes |
| Request body | 0 bytes |
Exactly one SP separates METHOD, request-target and VERSION. Only HTTP/1.0 and HTTP/1.1. METHOD and header names use token characters. Request-target is bounded visible ASCII. CRLF is mandatory. Header names are case-insensitive; obs-fold and malformed/control-bearing values are rejected. Content-Length uses checked decimal parsing; one zero value is accepted, duplicates are rejected, nonzero is rejected. Transfer-Encoding is unsupported and CL/TE conflicts are rejected. Bytes following CRLFCRLF are rejected. NEED_MORE is a viable incomplete prefix; COMPLETE occurs only at CRLFCRLF; ERROR is sticky until reset.
P5-2A does not alter TCP, foreground web lifecycle, patch series, or URI routing. The target workflow compiles the parser object against the exact pinned PA-220 source.
