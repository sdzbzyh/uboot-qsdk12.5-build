# PA-220 P5-2C static memory and stack audit

Evidence source: Actions Run [45137814338](https://github.com/sdzbzyh/uboot-qsdk12.5-build/actions/runs/45137814338), artifact `14800038825`.

## Static storage

The linked six-object N32 relocatable object reports:

| Section | Bytes |
|---|---:|
| Text | 18,216 |
| Data | 0 |
| BSS | 25,715 |
| Total | 43,931 |

BSS breakdown from the object size reports:

- `common/cmd_pa220_web.o`: 24,051 bytes
- `net/tcp_ipv4_tx.o`: 1,664 bytes
- Other compiled modules: no BSS reported

Key command-module static symbols:

| Symbol | Bytes |
|---|---:|
| `pa220_web_connection` | 8,604 |
| `pa220_web_parser` | 13,224 |
| `pa220_web_response` | 1,460 |
| `pa220_web_rx` | 536 |
| `pa220_web_info` | 184 |
| `pa220_web_peer` | 11 |

These are fixed-size static objects; the parser and TCP connection are not allocated on the foreground stack.

## Compiler stack-usage evidence

The GCC `.su` reports were generated and included in the OCTEON artifact. Relevant per-function frame sizes:

| Function | Frame bytes |
|---|---:|
| `http_handler_build` | 1,424 |
| `build_response` | 288 |
| `header_u64` | 96 |
| `writer_json_string` | 96 |
| `pa220_web_tcp_rx` | 128 |
| `pa220_web_service` | 48 |
| `do_pa220_web` | 64 |
| `tcp_core_input` | 128 |
| `tcp_emit` | 112 |
| `tcp_ipv4_tx_emit` | 80 |

The HTTP handler's largest listed frame is 1,424 bytes; the nested response builder contributes a further 288-byte frame on the path that calls it. The per-function reports are compiler measurements, not a complete whole-program worst-case stack proof.

## Remaining audit work

A full call-graph stack bound, including all nested paths and any platform interrupt/watchdog context, has not yet been formally established. The static-memory and per-function stack measurements are available, but this section is not a final stack-safety sign-off.
