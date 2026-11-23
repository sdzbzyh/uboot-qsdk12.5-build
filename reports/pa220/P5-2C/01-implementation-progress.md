# PA-220 P5-2C implementation progress checkpoint

UTC: 2026-11-23
Branch: feature/pa220-ubootkit-web
Base checkpoint: 7282a487c5a5b6b6efa25eff49c5961a826f5914
Implementation commits:
- d3b0898db4cbe59ee7135be779c8a24d9aabbb75 — integrate bounded HTTP foreground lifecycle
- 8b30dea871b2be80a4eceacce23a316350b18779 — bound HTTP FIN retry interval

## Source changes

Only u-boot/common/cmd_pa220_web.c was modified. P5-2A Parser and P5-2B Handler source/header files were not modified.

The foreground entry now:
- owns the HTTP parser, request scratch, handler snapshot and response buffer in static storage;
- drains tcp_core_read() after tcp_core_input(), including payload accepted from the FIN-bearing segment;
- handles NEED_MORE, COMPLETE and ERROR, and applies a bounded partial-request timeout;
- delegates exact URI and GET/HEAD response construction to the existing handler;
- retains one response offset and caps each write by peer_mss, peer_window, TCP_CORE_MAX_MSS and TCP_CORE_TX_CAPACITY;
- advances the offset only by tcp_core_write()'s positive accepted length;
- waits for the complete pending ACK before sending another segment;
- leaves the offset unchanged on a zero-window result;
- sends local FIN only after every response byte has been acknowledged;
- relies on the TCP core's LAST-ACK completion and LISTEN reset path;
- retains a single-peer, single-request, Connection: close service model.

## Validation state — NOT PASS

No P5-2C Host test suite has been run. No P5-2C test scenario or assertion count is claimed.

No P5-2C OCTEON N32 compile, relocatable link, ELF/ABI audit, stack report, static-memory report, or dynamic-allocation dependency audit has been run.

GitHub Actions status for 8b30dea871b2be80a4eceacce23a316350b18779 was queried:
- Combined status checks: none reported.
- Commit-associated workflow runs returned: none.
- Therefore no P5-2C Actions Run ID exists in this checkpoint.

The available GitHub integration actions in this execution environment expose repository/file/commit and workflow-result retrieval, but do not expose a shell/runner or workflow-dispatch action. The changes were committed through the GitHub contents API; this does not execute host tests or cross-compilation.

## Required remaining work

1. Review and correct the foreground implementation through compile/test feedback.
2. Add an independent P5-2C Host harness covering parser streaming, request timeout/error, all routes and methods, MSS/partial/full ACK, zero window, pending suppression, accepted-byte accounting, early FIN, CLOSE-WAIT, LAST-ACK, listener recovery, timeout and abort.
3. Run the Host suite and preserve its complete log and exact scenario/assertion counts.
4. Run OCTEON N32 compile and relocatable link against pinned source 801717ba6a8a59ed2cfd53bf3e1b19841fecd2e8; preserve ELF/ABI/stack/static-memory/dynamic-dependency evidence.
5. Record Actions Run ID and artifact SHA-256 values.
6. Prepare, but do not claim, PA-220 physical-device validation.

No physical-device validation is claimed. P5-2C is not complete.


## 2026-11-23 cloud validation update

Dedicated workflow added:
- `.github/workflows/pa220-p5-2c.yml`
- Workflow commit: `3526880c4987fe4e7ee03109f06faf10e163a3a2`
- Push-triggered Run: `45137814338`

OCTEON job `137568193204` completed successfully:
- Private `octeon-sdk-5.1.0` downloaded and SHA256 sidecar verified.
- GCC: Cavium GCC 4.7.0, SDK Build 52; target `mips64-octeon-linux-gnu`.
- Fixed PA-220 source `801717ba6a8a59ed2cfd53bf3e1b19841fecd2e8` checked out exactly.
- Patch series 0001 through 0005 applied; P5-2A/P5-2B/P5-2C sources and headers staged.
- Six target objects compiled and N32 relocatable linked.
- Linked ELF is ELF32 big-endian MIPS, flags include `abi2, octeon, mips64r2`.
- No malloc/calloc/realloc/free undefined dependency detected.
- Linked object SHA256: `e6b67dabfaeba0f75a1743b7c4e899345669cc942fe59ea30b927d46c0bebd95`.
- Combined object: text 18,216 bytes; data 0; BSS 25,715; total 43,931.
- Full logs, object ELF reports, undefined symbols, sizes and .su stack reports are in Actions Artifact `14800038825` (digest `5f1d57c24bfb6c42f555d9b8c0a408c94fb56c8757a7cb5975fcdac8c15bf69e`).

Host job `137568193540` failed before executing any lifecycle scenario because `tests/host/http_lifecycle/run.sh` has not yet been added. Artifact `14801176146` (digest `5cf118e3156bd835ea58f0c1a17f7e9b7b80dd2bc6a76de1eef1082fc7851b55`) preserves this failure. No Host scenario or assertion count is claimed.

P5-1D workflow concurrency correction:
- Commit `13a27333528f950ede2641b86327e3b853ecc141` removed shared-branch auto-commits and downgraded permissions to read-only.
- Subsequent Run `45137699302` did not encounter the prior Rebase conflict.
- That Run still failed its OCTEON full-image build because the P5-1D workflow did not stage `net/http_request.h` required by the current P5-2C command source. This is a workflow source-staging/build issue, not a Host or P5-2C runtime test result.

P5-2C remains NOT COMPLETE. Outstanding: implement and run the independent production-lifecycle Host harness; complete call-graph stack analysis; improve full firmware integration/link validation; perform physical PA-220 validation. No physical-device result is claimed.
