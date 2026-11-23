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
