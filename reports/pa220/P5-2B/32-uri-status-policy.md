# P5-2B URI and HTTP status policy
| Input | Result |
|---|---|
| exact allowlist URI | route selected |
| query string | 404; no query semantics |
| valid percent encoding | never decoded; exact-match miss => 404 |
| prefix, suffix, extra path segment | 404 |
| malformed URI syntax, backslash, fragment, malformed percent escape | 400 |
| malformed method/version/request structure | 400 |
| valid method other than GET/HEAD on an allowed URI | 405 with Allow: GET, HEAD |
| missing required device/memory snapshot | 500 |
| output capacity too small | function error, output length zero, caller must not transmit |
| HEAD | same representation headers and Content-Length as GET, zero body bytes |
All responses use Connection: close; no keep-alive, request body, file path, command or mutation route exists.
