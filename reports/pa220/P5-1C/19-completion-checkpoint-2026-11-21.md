# PA-220 U-BootKit — P5-1C completion checkpoint
Date: 2026-11-21

## Checkpoint status

**Foreground service lifecycle, existing-command integration, TCP RX handler registration and TCP-core input wiring are implemented and verified by a real PA-220 OCTEON build and final link. End-to-end TCP connection management is NOT yet accepted because the current network stack has no reviewed TCP IPv4/Ethernet/ARP output path.**

This checkpoint closes the implementation/build slice of P5-1C within the output limitation explicitly identified in the project request. It does not claim a functioning Web server or a complete TCP session.

## Repository state

- Repository: `sdzbzyh/uboot-qsdk12.5-build`
- Branch: `feature/pa220-ubootkit-web`
- Branch HEAD before this checkpoint commit: `54cd5b33ccc54f1ca899e896e00d460fc5a936f4`
- P5-1C code commit: `70c6c48d4f84a5a81089ec68dcd97590ac5960bd`
- Fixed source: `sdzbzyh/pa220-openwrt@801717ba6a8a59ed2cfd53bf3e1b19841fecd2e8`
- Build run: [44863474734](https://github.com/sdzbzyh/uboot-qsdk12.5-build/actions/runs/44863474734) — SUCCESS
- Build evidence: `reports/pa220/P5-1C/10-octeon-integrated-build.log`
- Final verification: `reports/pa220/P5-1C/18-final-verification-summary.md`

## Implemented

1. Existing board command `pa220` dispatches the new `web` subcommand. Existing `status`, `env`, `memory`, `board` and `clock` subcommands remain registered through the original single `U_BOOT_CMD(pa220,...)`.
2. `common/cmd_pa220_web.c` implements startup checks, Ethernet initialization, foreground receive polling, TCP listener context and cleanup.
3. The TCP handler is registered only after startup succeeds.
4. Received TCP segments are passed to `tcp_core_input()`.
5. `tcp_core_poll()` is called from the foreground loop.
6. On Ctrl-C, the handler is unregistered before the connection context is cleared; Ethernet is then halted.
7. No P4 network driver, NetLoop, DHCP, Ping or TFTP source is changed.
8. No HTTP parser, URI handler or remote write capability is introduced.

## Build and final link

- Cavium GCC 4.7.0 / SDK Build 52.
- Exact pinned PA-220 source.
- Patches 0001–0004 applied by the sole patch-application owner, `build-pa220.sh`.
- Full target build passed.
- `cmd_pa220_web.o` is a big-endian MIPS N32 relocatable object.
- Final `u-boot-octeon_pa220`, linker map and System.map exist and passed verification.
- `do_pa220_web`, `net_set_tcp_handler`, `tcp_core_poll` and `tcp_core_input` are present in the final ELF, map and System.map.
- Full ELF/map/System.map/binary retained in Actions artifact `14691016829`.

## Acceptance boundary

| Requirement | Status |
|---|---|
| `pa220 web` front entry | PASS |
| Foreground startup / polling / exit lifecycle | PASS |
| TCP handler register / unregister ordering | PASS |
| Single `tcp_core_connection` context | PASS |
| Segment dispatch to `tcp_core_input()` | PASS |
| TCP core polling integration | PASS |
| Existing NetLoop and network commands preserved | PASS by source scope and final build |
| Read-only security boundary | PASS by implementation scope |
| Real Cavium OCTEON target build | PASS |
| Final ELF / map / System.map / symbols | PASS |
| TCP SYN/SYN-ACK handshake over Ethernet | BLOCKED — no TCP output emitter |
| Established-session payload delivery and peer close over wire | BLOCKED — depends on TCP output |
| RAM boot and physical TCP test | NOT RUN — separate hardware task |

The emit callback intentionally returns failure. This prevents the TCP core from recording an unsent segment as transmitted. The listener therefore cannot complete a passive open. Existing core FIN/RST/timeout logic is integrated by API, but those live-session paths cannot be validated until a safe output path is implemented.

## Explicit follow-up dependency

Before advertising or testing a usable TCP/Web service, separately design and review a minimal single-connection TCP IPv4/Ethernet output path, including ARP neighbour resolution, packet-buffer ownership, retry/timeout behavior and cleanup. Do not reuse the global ARP pending buffer as if it were a reentrant queue. After that work, run targeted connection-state tests and real PA-220 RAM-boot / Ethernet validation.

No work on that output dependency is started by this checkpoint.

## P4 parallel work

P4 remains IN PROGRESS according to `doc/pa220/P4-PLAN.md`. P5-1C touched no P4 files. DHCP/Ping hardware tests remain NOT RUN; TFTP receive/upload remains BLOCKED by its security gate.
