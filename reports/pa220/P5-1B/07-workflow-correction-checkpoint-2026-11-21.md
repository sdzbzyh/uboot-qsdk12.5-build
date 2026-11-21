# PA-220 P5-1B checkpoint — delegate patching to build-pa220.sh

- Corrected patch commit: `effd2d9dd6117866c7a0548eca072830c9025d68`.
- Failed workflow run: `44860942367`.
- The corrected patch's sequential apply-check/apply passed.
- Failure occurred because the workflow pre-applied 0001/0002/0003 and then called `build-pa220.sh`, which is already responsible for patch application; it attempted to re-apply 0001.
- Workflow now delegates checkout and patch application exclusively to `build-pa220.sh`, captures its output, verifies all three patch confirmations and checks the exact source revision.
- No source code change is required for this workflow-order correction.
- P5-1B remains incomplete pending a clean full build and final ELF/map/symbol verification.
- RAM boot and physical TCP receive remain NOT VERIFIED.
