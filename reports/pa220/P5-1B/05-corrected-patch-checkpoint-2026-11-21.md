# PA-220 P5-1B checkpoint — corrected new-file patch

- Previous branch commit: `82951cdb` (Actions evidence for failed apply-check).
- Fixed PA-220 source: `801717ba6a8a59ed2cfd53bf3e1b19841fecd2e8`.
- Failed workflow: `44860655890`.
- Failure: patch 0003 treated `u-boot/net/tcp.c` and `u-boot/include/net/tcp.h` as modifications; neither file exists in the pinned tree.
- Correction: regenerate 0003 with both files as proper new-file additions. Keep paths consistent with existing 0001/0002.
- Link-retention anchor remains included to preserve TCP public API sections under `--gc-sections`.
- Dedicated workflow remains configured; rerun is triggered by the corrected 0003 patch.
- P5-1B is not complete until the new run passes all target/ELF/map/symbol checks.
- RAM boot and physical TCP receive remain NOT VERIFIED.
