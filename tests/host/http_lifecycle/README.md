# PA-220 P5-2C Host Harness

Run from the repository root:

```sh
bash tests/host/http_lifecycle/run.sh
```

The runner performs two checks:

1. Host syntax compilation of the production foreground command (`u-boot/common/cmd_pa220_web.c`) and the production U-Boot device-information adapter (`u-boot/net/http_handler_uboot.c`) against narrow U-Boot API shims.
2. Native compilation and execution of the production TCP core, HTTP request parser, and read-only URI response builder.

The executable reports 28 named scenarios. They cover passive open/handshake, checksum rejection, RX ordering and buffering, partial/full ACK handling, zero-window behavior, retransmission and timeout paths, FIN/CLOSE-WAIT/LAST-ACK/TIME-WAIT transitions, fragmented HTTP parsing, malformed framing, unsupported bodies, and handler status/method behavior.

The host suite does **not** emulate the Ethernet driver or execute the full `do_pa220_web()` foreground loop end-to-end. That remains a separate integration/runtime validation item; a successful host run must not be represented as hardware validation.
