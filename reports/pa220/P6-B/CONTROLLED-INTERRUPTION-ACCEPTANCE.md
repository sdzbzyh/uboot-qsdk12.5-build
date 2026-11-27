# P6-B Controlled Interruption / Timeout Acceptance

- Date: 2026-11-27
- Repository: `sdzbzyh/uboot-qsdk12.5-build`
- Branch: `feature/pa220-ubootkit-web`
- Scope: controlled server-side interruption during a RAM-only TFTP GET of a 32 MiB fixture
- Result: **PASS for the observed controlled-interruption scenario**

## Device result

The device initiated:

```
tftpboot ${loadaddr} pa220-p6-get-33554432.bin
```

The transfer entered the DATA phase and then failed after the Tftpd64 server process was stopped:

```
Loading: ##############T
TFTP GET failed: network interruption
```

Post-failure read-only checks:

- `filesize` is undefined.
- `fileaddr` is undefined.
- `pa220 memory` remains unchanged:
  - `loadaddr = 0x20000000`
  - load base and load size are not set
  - `__tmp_load = [0x20000000, 0x30000000)` (256 MiB)

This confirms failed-transfer metadata cleanup and preservation of the live memory descriptor for this test.

## Packet capture

- Capture: `C:\\pa220-p6-b\\pa220-p6-b-interrupt.pcapng`
- Size: 10,998,760 bytes
- SHA-256: `83B96A88C0AEC9983A7B429369396B715B9613CAA2312D0A42984508AB6840CA`
- RRQ and OACK were observed, followed by DATA blocks through block 6706.
- Frame 13415: device-to-server UDP payload `00 04 1a 32` = TFTP ACK opcode 4, block 6706.
- Frame 13416: identical payload `00 04 1a 32` = retransmitted ACK for block 6706.
- The two ACKs are separated by approximately 4.999 seconds, consistent with the negotiated 5-second timeout.
- No ICMP packets were present in the reported capture tail.

The final ACK retransmission is directly confirmed by the packet hex dump, not inferred solely from UDP packet length.

## Acceptance interpretation

PASS for the controlled server-side interruption case:

1. TFTP GET progressed into DATA transfer.
2. The device emitted ACK 6706 and retransmitted the same ACK after approximately five seconds.
3. U-Boot terminated the GET with `network interruption`.
4. Failed-transfer metadata was cleared (`filesize` and `fileaddr` absent).
5. The live `__tmp_load` descriptor remained unchanged.

This validates the observed timeout/retry and cleanup behavior in this controlled scenario. It does not claim exhaustive coverage of every network failure mode or every possible timing/race condition.

No loader/boot command, `saveenv`, or persistent-storage write was performed. The test was RAM-only.

## P6-B conclusion

Together with the previously accepted 4 KiB success, 32 MiB exact-limit success, and 32 MiB+1 OACK tsize rejection, this closes the planned P6-B physical test matrix. P6-B is now **ACCEPTED for the tested TFTP GET cases**.
