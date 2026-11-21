# P5-1B workflow correction — single patch application owner

The corrected 0003 patch passed the dedicated workflow's initial sequential `git apply --check` and `git apply` in run `44860942367`. The subsequent build failed because the workflow had pre-applied the patches, then invoked `build-pa220.sh`, which correctly owns patch application itself and attempted to apply 0001 again.

Correction: remove the workflow's duplicate pre-application. The workflow now invokes the established `build-pa220.sh` directly on a fresh pinned checkout. That script performs its own per-patch reverse-check, `git apply --check`, and apply. The workflow captures its output, verifies all three patch application confirmations, and independently verifies the fixed source commit.

This preserves the proven build path used in Actions run `44860047122` and avoids patching the same checkout twice. The next run is the authoritative target build and final ELF/map/symbol check.
