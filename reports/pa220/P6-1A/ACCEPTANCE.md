# P6-1A — TFTP GET stale metadata invalidation

- Date: 2026-11-25
- Integration repository: `sdzbzyh/uboot-qsdk12.5-build`
- Branch: `feature/pa220-ubootkit-web`
- Parent checkpoint: `c2e9253d0ed6ae926e1dfa5120d794a401047146`
- Fixed PA-220 source: `sdzbzyh/pa220-openwrt@801717ba6a8a59ed2cfd53bf3e1b19841fecd2e8`

## Scope

Invalidate stale volatile `filesize` and `fileaddr` before a TFTP GET/RRQ can start. This work does not enable or test a real transfer.

## Source API finding

In the pinned `u-boot/common/cmd_nvedit.c`, `_do_env_set()` returns `!hdelete_r(...)` for deletion. `setenv(name, NULL)` therefore returns zero for successful deletion and one when the variable is absent or deletion fails. The helper must call `getenv()` before deletion.

## Production behavior

- Attempt `filesize` and `fileaddr` deletion independently.
- Continue the second attempt even if the first fails.
- Re-read both names and fail closed unless both are absent.
- Call only in the GET branch before RRQ state/request setup.
- On failure, set `NETLOOP_FAIL` and return without sending RRQ.
- No `saveenv`; PUT and Server/WRQ paths are untouched.

## Validation evidence

This report is a checkpoint template. Fill actual run URL/ID, commit, test counts, toolchain identity, object/link checks and artifact hashes from the dedicated P6-1A Actions run. Until those records exist, all build/test results remain **PENDING**, not PASS.

## Hardware status

No TFTP transfer is authorized by this implementation. Before any real GET, complete P6-A live descriptor/reservation checks and all subsequent P6 software gates, then obtain explicit confirmation for the transfer.
