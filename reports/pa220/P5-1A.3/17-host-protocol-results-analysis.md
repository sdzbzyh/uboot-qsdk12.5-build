# P5-1A.3 Host TCP protocol verification — analysis

Date: 2026-11-20
Branch: `feature/pa220-ubootkit-web`

## Retrieved execution evidence

Source log: `reports/pa220/P5-1A.3/16-host-protocol-verification.txt`

- Host compile: `BUILD_RC=0`
- Host test process: `TEST_RC=1`
- Assertions: 152 passed, 20 failed.
- TCP source SHA1 before and after execution: `2be12d49f0e5d80e6980b649d6148b90a474d287`.
- TCP header SHA1 before and after execution: `db4752ce1c9589c9579069e40476279c0eabda75`.
- Harness SHA256: `b191b27a972a1c4b9f1a5ca66a53e15b642c71ecb77dcfbbfc2ec15a5d5d080a`.

## Interpretation of the 20 failed assertions

The harness incorrectly assumes that every accepted ordinary segment makes `tcp_core_input()` return 1.

Inspection of the committed implementation shows:
- For in-order payload and FIN, the input path sends an ACK and returns `tcp_send_ack()`.
- `tcp_send_ack()` returns `tcp_emit()`, which returns the emit callback's return value.
- The test callback `emit_cb()` returns 0 on successful emission.
- For ordinary ACK-only segments the input path returns 0.
- The SYN/listen and SYN_RECEIVED handshake paths explicitly return 1.

Therefore, many harness checks of `input(...) == 1` for established-state data/FIN/ACK traffic are invalid test expectations, not evidence of TCP input rejection. The 20 failures must not be classified as 20 TCP defects. The harness must distinguish protocol acceptance/state/output from the callback return convention.

## Protocol observations that remain significant

### CLOSE-WAIT post-FIN payload

The log reports:
- state = 6 (`TCP_CORE_CLOSE_WAIT`)
- rx_len = 1
- rcv_nxt = 5003
- payload byte `X` was accepted after peer FIN.
- Harness printed `CONFIRMED: data accepted after peer FIN`.

Source inspection confirms the generic payload receive path runs without a guard rejecting payload when `c->state == TCP_CORE_CLOSE_WAIT`. The FIN path sets `fin_received` and transitions ESTABLISHED to CLOSE_WAIT, but later input does not reject post-FIN payload. This is a reproducible protocol correctness defect candidate, strongly supported by both runtime observation and source path. It requires a corrected assertion-based retest to remove dependence on the harness's mistaken return-value expectation.

### TIME-WAIT duplicate FIN

The log reports:
- original deadline = 60012
- after duplicate FIN = 60012
- ACK output assertions passed in the harness.
- The duplicate-FIN input return-value assertion failed because it expected 1, whereas successful ACK emission returns 0 through the callback.

Source inspection shows the duplicate FIN has sequence 5001 while `rcv_nxt` is 5002. It is handled by the sequence-mismatch branch, which sends an ACK and returns the callback result before reaching the FIN processing block that sets `time_wait_deadline`. Thus the observed unchanged deadline is consistent with the implementation. The current behavior is not a confirmed defect from this test; the test should assert the emitted ACK and unchanged deadline while expecting the callback return value 0.

## Next bounded action

Create a separate corrected Host harness copy (do not edit the original harness or TCP source), correct only the return-value expectations according to the implementation's documented callback contract, and rerun. Preserve the original source identities. Keep the focused CLOSE-WAIT and TIME-WAIT checks as explicit state/output assertions.

No production TCP source changes are authorized by this analysis.
