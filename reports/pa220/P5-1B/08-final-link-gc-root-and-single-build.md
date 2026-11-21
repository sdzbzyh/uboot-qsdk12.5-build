# P5-1B final-link garbage-collection finding and correction

## Actual first-build evidence from Actions run 44861187740

The build script applied 0001/0002/0003, compiled `net/tcp.o`, built `net/libnet.o`, and completed the final PA-220 link. The first build generated:
- `net/tcp.o`: 53,016 bytes
- `net/libnet.o`: 401,813 bytes
- final ELF: 7,787,497 bytes
- final map: 579,599 bytes
- System.map: 100,730 bytes
- PA-220 BIN: 1,362,344 bytes

The ELF is a statically linked ELF32 big-endian MIPS N32 executable, entry point `0xc0000000`.

## Why the first link-retention attempt was insufficient

Artifact `14690295303` was downloaded and inspected. `nm` confirmed all TCP APIs were in both `tcp.o` and `libnet.o`, but no TCP API symbol survived in the final ELF. The linker map placed `.text.tcp_*`, `.text.tcp_core_link_anchor` and `.rodata.tcp_core_api_refs` in discarded input sections.

The previous anchor was referenced only from `net_set_tcp_handler()`; that setter itself was unreachable to the linker and was garbage-collected. This is a real `--gc-sections` reachability issue, not a compile failure.

## Corrected root reference

- Add a typed `tcp_net_api_refs` table in `net.c` referencing both `net_set_tcp_handler` and `tcp_core_link_anchor`.
- In the live `NetReceive()` TCP branch, after explicit-handler gating, store the table address through a volatile pointer. This creates a live relocation from the retained receive path to the table.
- The table keeps the setter and core anchor; the core anchor keeps every public TCP API function.
- This adds no packet behavior and does not activate TCP without explicit registration.
- Remove the ineffective anchor call from the setter.

## Workflow correction

Run `44861187740` completed the full build successfully once, then failed because the workflow redundantly invoked `build-pa220.sh` a second time. The second invocation encountered already-applied patches. The workflow now retains only the first invocation and proceeds directly to object/ELF/map checks.

The next run is required to prove the TCP symbols survive final link and appear in the ELF/map/System.map.
