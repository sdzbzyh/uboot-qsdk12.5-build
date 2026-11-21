# P5-1C second cloud build failure analysis

- Workflow run: [44862992236](https://github.com/sdzbzyh/uboot-qsdk12.5-build/actions/runs/44862992236)
- Integration commit: 23417f7607656ebe8a86f045c0c6858877d96b74
- Fixed PA-220 source: 801717ba6a8a59ed2cfd53bf3e1b19841fecd2e8
- Cavium SDK download/checksum: PASS
- Patch application and command object compilation: PASS
- Final link: FAIL

## Root cause

The final link reported a duplicate `_u_boot_list_2_cmd_2_pa220` command-list symbol between `common/libcommon.o` and `board/octeon/pa220/libpa220.o`.

The existing `pa220` command is introduced by patch 0001 and extended by patch 0002. The first P5-1C implementation incorrectly registered a second top-level `pa220` command from `cmd_pa220_web.c`.

## Correction

P5-1C now extends the existing `do_pa220()` dispatcher with the `web` subcommand and keeps the original single `U_BOOT_CMD(pa220,...)` registration in the PA-220 board file. The new common command object exports only `do_pa220_web()`; it no longer registers a second command.

The P5-1C board-file hunk is generated against the actual post-0001/post-0002 source state, preserving the existing read-only `status`, `env`, `memory`, `board` and `clock` subcommands.

No P4 source or report is modified. The next Actions run will reapply the complete ordered patch series once and repeat one full target build to verify the corrected final link.
