# P5-2A Host matrix
| # | Case | Expected |
|---:|---|---|
| 01 | Complete GET | COMPLETE and exact fields |
| 02 | Complete HEAD | COMPLETE and exact fields |
| 03 | Request-line split | NEED_MORE then COMPLETE |
| 04 | URI split | NEED_MORE then COMPLETE |
| 05 | Header split | NEED_MORE then COMPLETE |
| 06 | CRLF split | NEED_MORE then COMPLETE |
| 07 | Multiple headers | COMPLETE, count 3 |
| 08 | Case-insensitive header names | COMPLETE, CL zero accepted |
| 09 | Request line overlong | ERROR / REQUEST_LINE_LIMIT |
| 10 | URI overlong | ERROR / BAD_URI |
| 11 | Header count limit | ERROR / HEADER_COUNT_LIMIT |
| 12 | Header line limit | ERROR / HEADER_LINE_LIMIT |
| 13 | Header total limit | ERROR / HEADER_TOTAL_LIMIT |
| 14 | RX overflow | ERROR / RX_LIMIT |
| 15 | NUL byte | ERROR |
| 16 | Bare LF | ERROR / BAD_CRLF |
| 17 | Invalid request-line spacing | ERROR / BAD_REQUEST_LINE |
| 18 | Header missing colon | ERROR / BAD_HEADER |
| 19 | Invalid Content-Length | ERROR / BAD_CONTENT_LENGTH |
| 20 | Duplicate Content-Length | ERROR / DUPLICATE_CONTENT_LENGTH |
| 21 | Conflicting duplicate Content-Length | ERROR / DUPLICATE_CONTENT_LENGTH |
| 22 | Content-Length + Transfer-Encoding | ERROR / conflict |
| 23 | Transfer-Encoding before Content-Length | ERROR / conflict |
| 24 | Unsupported Transfer-Encoding | ERROR / unsupported |
| 25 | Content-Length decimal overflow | ERROR / BAD_CONTENT_LENGTH |
| 26 | Control byte in header value | ERROR / BAD_HEADER_VALUE |
| 27 | Nonzero body length | ERROR / BODY_NOT_SUPPORTED |
| 28 | Incomplete request | NEED_MORE |
| 29 | Trailing body bytes | ERROR / BODY_TOO_LARGE |
| 30 | Reset then independent request | COMPLETE |
| 31 | Repeated COMPLETE | COMPLETE |
| 32 | Header terminator split | NEED_MORE then COMPLETE |
