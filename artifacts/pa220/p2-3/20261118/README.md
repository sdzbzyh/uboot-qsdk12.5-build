# PA-220 P2-3 Build Evidence — 2026-11-18

This directory stores the clean-build evidence and audited output artifacts for PA-220 U-Boot P2-3.

Expected files include:

- `build.log` — complete build log
- `build-exit-code.txt` — build process exit code
- `u-boot-octeon_pa220.bin` — generated boot image
- `u-boot-octeon_pa220` — ELF image
- `u-boot-octeon_pa220.map` — linker map
- `System-octeon_pa220.map` — symbol map
- `config.h`, `config.mk`, and `autoconf.mk` (if generated) — build configuration snapshots
- `SHA256SUMS` — checksums for uploaded artifacts

Upload the files from the local P2-3 evidence and build directories. Keep original filenames where possible; if configuration snapshots share a filename, rename the board-level snapshot to `board-config.mk`.

Build source commit: `801717ba6a8a59ed2cfd53bf3e1b19841fecd2e8`
Integration branch: `feature/pa220-ubootkit-web`
