# PA-220 P5-1A.3 Validation Records

This directory stores the reproducible evidence for P5-1A.3 validation of the standalone PA-220 native TCP core.

Planned records:

- `01-remote-source.txt` — validated branch HEAD and source-file hashes.
- `02-preprocess.log` / `03-preprocess.err` — OCTEON-target preprocessing output and diagnostics.
- `04-compile.log` / `05-compile.err` — OCTEON-target compilation output and diagnostics.
- `06-elf-header.txt` — ELF header and ABI inspection.
- `07-object-info.txt` — object-file metadata and size.
- `08-host-tests.log` — independent host-side protocol tests.
- `09-test-summary.md` — evidence-based result summary and remaining limitations.

Temporary source copies, object files, and test executables belong under `/tmp/pa220-p5-validation/`, not in this report directory. Only useful, reproducible logs and reports should be committed here.