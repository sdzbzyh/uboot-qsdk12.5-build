# P5-2A Host regression matrix
| # | Case | Expected | Actual | Result |
|---:|---|---|---|---|
| 01 | Complete GET | COMPLETE, GET /, 1 header | COMPLETE, GET /, 1 header | PASS |
| 02 | Complete HEAD | COMPLETE, HEAD /status | COMPLETE, HEAD /status | PASS |
| 03 | Request-line split | NEED_MORE until final chunk | NEED_MORE then COMPLETE | PASS |
| 04 | URI split | NEED_MORE until final chunk | NEED_MORE then COMPLETE | PASS |
| 05 | Header split | NEED_MORE then 1 parsed header | NEED_MORE then COMPLETE, header_count=1 | PASS |
| 06 | CRLF split | NEED_MORE then COMPLETE | NEED_MORE then COMPLETE | PASS |
| 07 | Multiple headers | COMPLETE, header_count=3 | COMPLETE, header_count=3 | PASS |
| 08 | Header name case-insensitive | COMPLETE, CL zero accepted | COMPLETE, CL zero accepted | PASS |
| 09 | Request line overlong | ERROR / request-line-limit | ERROR / request-line-limit | PASS |
| 10 | URI overlong | ERROR / bad-uri | ERROR / bad-uri | PASS |
| 11 | Header count over limit | ERROR / header-count-limit | ERROR / header-count-limit | PASS |
| 12 | Header line over limit | ERROR / header-line-limit | ERROR / header-line-limit | PASS |
| 13 | Header total over limit | ERROR / header-total-limit | ERROR / header-total-limit | PASS |
| 14 | RX buffer overflow | ERROR / rx-limit | ERROR / rx-limit | PASS |
| 15 | NUL byte | ERROR / bad-header | ERROR / bad-header | PASS |
| 16 | Bare LF | ERROR / bad-crlf | ERROR / bad-crlf | PASS |
| 17 | Invalid request-line spacing | ERROR / bad-request-line | ERROR / bad-request-line | PASS |
| 18 | Header missing colon | ERROR / bad-header | ERROR / bad-header | PASS |
| 19 | Invalid Content-Length | ERROR / bad-content-length | ERROR / bad-content-length | PASS |
| 20 | Duplicate Content-Length | ERROR / duplicate-content-length | ERROR / duplicate-content-length | PASS |
| 21 | Conflicting duplicate Content-Length | ERROR / duplicate-content-length | ERROR / duplicate-content-length | PASS |
| 22 | Content-Length + Transfer-Encoding | ERROR / content-length-transfer-encoding | ERROR / content-length-transfer-encoding | PASS |
| 23 | Transfer-Encoding before Content-Length | ERROR / content-length-transfer-encoding | ERROR / content-length-transfer-encoding | PASS |
| 24 | Unsupported Transfer-Encoding | ERROR / transfer-encoding-not-supported | ERROR / transfer-encoding-not-supported | PASS |
| 25 | Content-Length decimal overflow | ERROR / bad-content-length | ERROR / bad-content-length | PASS |
| 26 | Control byte in header value | ERROR / bad-header-value | ERROR / bad-header-value | PASS |
| 27 | Nonzero request body | ERROR / body-not-supported | ERROR / body-not-supported | PASS |
| 28 | Incomplete request | NEED_MORE | NEED_MORE | PASS |
| 29 | Trailing body bytes | ERROR / body-too-large | ERROR / body-too-large | PASS |
| 30 | Reset then independent request | COMPLETE, HEAD | COMPLETE, HEAD | PASS |
| 31 | Repeated COMPLETE | COMPLETE, stable | COMPLETE, stable | PASS |
| 32 | Header terminator split | NEED_MORE then COMPLETE, count=1 | NEED_MORE then COMPLETE, count=1 | PASS |
| 33 | CR followed by non-LF | ERROR / bad-crlf | ERROR / bad-crlf | PASS |
| 34 | Malformed percent escape in URI | ERROR / bad-uri | ERROR / bad-uri | PASS |
| 35 | Backslash in URI | ERROR / bad-uri | ERROR / bad-uri | PASS |
