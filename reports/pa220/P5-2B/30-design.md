# PA-220 P5-2B bounded read-only URI handler
- The P5-2A parser is consumed through struct http_request; parser sources and behavior are unchanged.
- Handler is a separate module; P5-2C lifecycle integration is out of scope.
- Response storage is caller-owned and capped at 1460 bytes. The handler uses a fixed 1268-byte stack body workspace and a fixed 192-byte stack header workspace. It copies the complete response to caller storage only after checking the total wire length; an undersized output buffer leaves output_length zero.
- No heap allocation, filesystem access, shell command, environment lookup, storage write, reset, firmware update or persistent operation exists in this module.
- URI matching is exact byte-for-byte against /, /api/status, /api/board, /api/memory, /api/network. Query strings are valid request-target syntax but do not match the allowlist (404). Valid percent escapes are never decoded and do not match a route (404). Prefixes and extra segments are 404. Invalid URI syntax is 400.
- GET sends the body. HEAD has the same status, Content-Type and body-derived Content-Length as GET but sends zero body bytes. Other token-valid methods receive 405 and Allow: GET, HEAD.
- HTTP/1.0 and HTTP/1.1 are accepted; response version follows the request. All responses use Connection: close.
- Status mapping: malformed request representation 400; unmatched exact URI 404; unsupported method 405; unavailable required board/memory snapshot 500.
- JSON strings escape quotes, backslashes and control bytes. Home-page board text is HTML escaped. String scans and writes are bounded.
