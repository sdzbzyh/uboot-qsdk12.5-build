# PA-220 P5-2C Host Harness

Run from the repository root:

```sh
bash tests/host/http_lifecycle/run.sh
```

The runner executes the following production-code checks:

1. Host syntax compilation of the production foreground command (`u-boot/common/cmd_pa220_web.c`) and U-Boot device-information adapter (`u-boot/net/http_handler_uboot.c`) against narrow U-Boot API shims.
2. Native compilation and execution of the production TCP core, HTTP request parser, and read-only URI response builder. The suite reports 29 named scenarios and 161 assertions.
3. An end-to-end host simulation that links and executes the real `do_pa220_web()` foreground command with the production TCP core, parser, URI handler, and U-Boot information adapter. A deterministic fake Ethernet RX/TX boundary drives SYN/SYN-ACK/ACK, a complete HTTP request carried with FIN, the HTTP response, response ACKs, server FIN, final ACK, and command cleanup. It asserts that FIN is not sent until all response bytes have been ACKed.

The foreground simulation stubs the low-level `tcp_ipv4_tx` Ethernet transmitter; it does not replace the TCP/HTTP production code. The native Ethernet transmitter has separate P5-1D regression and target-build coverage. Physical PA-220 wire-level validation remains a separate hardware acceptance step.
