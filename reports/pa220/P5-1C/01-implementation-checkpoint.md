# PA-220 P5-1C implementation checkpoint

Status: IMPLEMENTATION COMMITTED; OCTEON target build pending.

## Scope

This first implementation unit adds the explicit `pa220 web` foreground command and connects the P5-1B TCP RX registration API to the existing `tcp_core_connection` / `tcp_core_input()` / `tcp_core_poll()` interfaces.

- The command is compiled only when `CONFIG_CMD_NET` is enabled.
- It follows the PA-220 `NetLoop()` Ethernet initialization order but owns a separate foreground `eth_rx()` loop.
- TCP handler registration occurs only after interface, MAC, IP and listener initialization succeed.
- Ctrl-C exits the foreground service.
- Cleanup unregisters the TCP handler before clearing the static connection context, then halts Ethernet.
- The existing DHCP, Ping, TFTP and NetLoop source files are not modified.
- No HTTP parser, URI handler, request body, upload, command execution or remote write operation is introduced.

## Explicit TCP output boundary

The existing TCP core requires a synchronous emit callback. The PA-220 network layer has low-level Ethernet/IP send primitives, but no reviewed generic TCP IPv4 emitter with safe neighbour-resolution ownership. Its existing ARP pending transmit state is global and coupled to the single `NetTxPacket` buffer.

This implementation therefore uses a fail-closed emit callback that returns an error. It does not pretend that discarded SYN-ACK/ACK frames were transmitted. The TCP core is invoked for validated received segments, but a passive open cannot complete and no real TCP/Web session is advertised. A dedicated, reviewed TCP output/ARP integration remains required before physical TCP communication.

## Lifecycle distinctions

- User Ctrl-C: service exit; unregister callback, clear connection state and halt Ethernet.
- Peer RST / TCP close: handled by the existing TCP core state machine when a connection exists.
- TCP idle/retransmission/TIME-WAIT expiry: driven by `tcp_core_poll()`; service exit is not used as a substitute for TCP timeout.
- No NetLoop timeout handler is borrowed or overwritten by the foreground loop.

## P4 isolation

P4 remains in progress as a separate workstream. Its latest checkpoint records P4-1/P4-2 static audit evidence only; DHCP/Ping hardware tests remain NOT RUN, and TFTP receive remains BLOCKED. This patch changes only `common/Makefile` and adds `common/cmd_pa220_web.c` to the fixed PA-220 source through patch 0004. It does not modify `net.c`, `net.h`, `net/Makefile`, P4 documents or P4 reports.

## Validation plan

The dedicated P5-1C Actions workflow applies the patch series only through `build-pa220.sh`, runs one complete pinned PA-220 OCTEON build, and checks the command object, final ELF, linker map, System.map and required symbols. RAM boot and physical network validation remain separate and are not claimed by a successful cross-build.
