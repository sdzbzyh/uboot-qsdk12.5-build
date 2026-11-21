# PA-220 P5-1B checkpoint — final-link GC root and single-build correction

- Last build run: `44861187740`.
- First invocation of `build-pa220.sh` successfully applied 0001/0002/0003 and completed the PA-220 build and final link.
- The workflow then erroneously invoked the build a second time; the duplicate failed because the patches were already applied. This caused the workflow's final status to fail despite a successful first build.
- Artifact `14690295303` contains the first successful build's `tcp.o`, `libnet.o`, ELF, map, System.map and BIN.
- Inspection showed TCP APIs present in `tcp.o` and `libnet.o`, but discarded from final ELF by `--gc-sections`; the previous anchor was only reachable from an unreferenced setter.
- New patch version roots a typed registration/core-anchor table from the live `NetReceive()` TCP branch using a volatile pointer.
- Workflow removes the duplicate second build and will run symbol checks on the single successful build.
- P5-1B remains incomplete until a fresh Actions run passes final ELF/map/System.map symbol verification.
- RAM boot and physical TCP traffic remain NOT VERIFIED.
