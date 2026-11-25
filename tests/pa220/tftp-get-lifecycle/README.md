# P6-1B TFTP GET lifecycle Host Harness

The harness injects the exact `GET_STATE_HELPERS` production helper text exported by
`scripts/generate-pa220-tftp-receive-patch.py` into a small C translation unit.
It does not provide a parallel implementation of the receive state machine.

Run with:

```sh
bash tests/pa220/tftp-get-lifecycle/run.sh
```

The runner compiles and executes the production helpers with GCC, then repeats
under AddressSanitizer and UndefinedBehaviorSanitizer.

Coverage includes:

- Full 32 MiB maximum-range reservation through the P4-2 validator, including
  reservation failure.
- Sequential block offsets, out-of-order blocks, range-end crossing, maximum
  plus one, zero-length terminal DATA, and checked byte-count commit.
- Exact completion, partial-length mismatch, and server tsize mismatch.
- Bounded decimal tsize parsing, including valid, over-limit, and malformed
  values.
- Timeout/failure cleanup and the command-return abort hook used for Ctrl-C.
- Clearing partial transfer length and invalidating both volatile result
  metadata markers on failure.

Failure diagnostics use ordinary newline-delimited output and do not alter the transfer state machine.

The Host Harness does not emulate the complete UDP/TFTP packet engine. The
OCTEON build separately compiles the integrated production `tftp.c` and
`cmd_net.c` call sites and links the complete PA-220 U-Boot image.
