# PA-220 P5-1B checkpoint — linker GC handling and dedicated validation

- Date: 2026-11-21
- Previous implementation commit: `1bd76cd4be8c1c59330954d31d56311386aedc7d`
- Parallel Actions evidence commit: `4826e196ddef2510e66421666ecc365b72215c6c`
- Fixed PA-220 source: `801717ba6a8a59ed2cfd53bf3e1b19841fecd2e8`

## Verified so far

Actions run `44860047122` completed SUCCESS. Its log confirms:
- patch 0001/0002/0003 applied;
- Cavium GCC 4.7.0 compiled `net/tcp.o`;
- `net/libnet.o` was built with `tcp.o` as an input;
- final link command included `net/libnet.o`;
- PA-220 BIN generated, 1,362,344 bytes.

The run did not preserve the final ELF/map/symbol table in the report artifact. The final linker uses `--gc-sections`, so symbol retention must be checked separately.

## Changes in this checkpoint

- Add `tcp_core_link_anchor()` with typed references to all public TCP core APIs.
- Call the anchor only when a non-NULL TCP receive handler is explicitly registered.
- Dedicated workflow checks TCP API symbols in `libnet.o`, final ELF, map and System.map.
- Legacy baseline/Host validation workflow becomes manual-only to prevent duplicate builds; P5-1B workflow owns the next full build.
- No P4 files changed; the only intervening commit was the previous cloud-validation evidence commit.

## Next

Wait for the dedicated Actions run. P5-1B remains incomplete until all object, final ELF, map and symbol checks pass and evidence is committed under `reports/pa220/P5-1B/`. RAM boot and physical TCP traffic remain NOT VERIFIED.
