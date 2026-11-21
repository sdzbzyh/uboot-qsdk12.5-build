# P5-1C third cloud build failure analysis

- Workflow run: [44863250278](https://github.com/sdzbzyh/uboot-qsdk12.5-build/actions/runs/44863250278)
- Integration commit: 65a5926da33ebe202518fbfe1e9c277721061528
- Fixed PA-220 source: 801717ba6a8a59ed2cfd53bf3e1b19841fecd2e8
- SDK download/checksum and source checkout: PASS
- P5-1C patch application: FAIL before compilation

## Root cause

The P5-1C board-dispatch hunk was generated from a line-array representation that retained the trailing empty split token after the final newline. That token was emitted as a real blank context line at end-of-file. The actual post-0001/post-0002 board file has a final newline, but no additional blank source line, so `git apply` correctly rejected the hunk at `pa220_board.c:415`.

This was a patch-generation boundary error, not a source-baseline or patch-order problem.

## Correction

The board-file diff generator now excludes the trailing empty split token when the source ends with a newline. The corrected hunk is independently matched against the reconstructed fixed-source state after patches 0001 and 0002, and applying the generated hunk reconstructs the intended board file exactly.

The implementation continues to extend the existing `do_pa220()` command rather than adding a second `U_BOOT_CMD(pa220,...)` entry. No P4 source or report is modified.
