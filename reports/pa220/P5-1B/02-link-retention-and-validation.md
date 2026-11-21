# P5-1B link-retention correction and cloud build design

- Previous full build: Actions run `44860047122`, SUCCESS, commit `1bd76cd4be8c1c59330954d31d56311386aedc7d`.
- That run applied patch 0003, compiled `net/tcp.o`, combined it into `net/libnet.o`, and linked `net/libnet.o` in the final U-Boot link command.
- The final link uses `--gc-sections`; therefore merely including `net/libnet.o` does not prove every otherwise-unreferenced TCP API function survives section garbage collection.

## Link-retention fix

- Add `tcp_core_link_anchor()` and a typed function-pointer reference table for every public TCP core API.
- `net_set_tcp_handler(non-NULL)` references the anchor. This has no packet-processing side effect; it only keeps the core API reachable in final link configurations using section GC.
- The registration callback remains disabled by default; `net_clear_handlers()` still resets it.
- The TCP input dispatch stays after IPv4 validation, destination filtering and defragmentation; the TCP segment length is independently bounded.

## Updated validation workflow

`.github/workflows/pa220-p5-1b-integration.yml`:
- checks out exact PA-220 commit `801717ba6a8a59ed2cfd53bf3e1b19841fecd2e8`;
- sequentially apply-checks/applies patches 0001, 0002, 0003;
- uses private SDK Release `octeon-sdk-5.1.0` and `OCTEON_SDK_READ_TOKEN`;
- runs full target build without repeating P5-1A Host tests;
- checks all TCP API symbols in `net/libnet.o`, final ELF, linker map and System.map;
- uploads `tcp.o`, `libnet.o`, final ELF, map, BIN and reports;
- commits logs/evidence under `reports/pa220/P5-1B/`.

The legacy P5-1A cloud-validation workflow is changed to manual-only so patch updates do not launch duplicate full builds.

## Status

The previous full build is a real PASS for patch application, target compilation, network aggregate object inclusion and the final link command. Final retained-symbol verification is pending the dedicated workflow. RAM boot and physical TCP receive remain separate NOT VERIFIED items.
