# PA-220 U-BootKit — P5-1D physical TCP/Ethernet validation

Date: 2026-11-21
Branch: `feature/pa220-ubootkit-web`
Stage: P5-1D — real-board TCP/Ethernet verification
Status: **PASS — TCP handshake and 10-byte client payload were observed on the physical Ethernet link**

## Provenance

- P5-1D implementation commit: `e0a528acfcd4d023d87afc5096e939c132bebece`
- GitHub Actions run: `44867249332` — SUCCESS
- Artifact: `pa220-p5-1d-native-tcp-ipv4-ethernet` (artifact ID `14692751342`)
- Final BIN SHA-256: `63ec085a77e5a415dad4f52affb5a19889110484aaf7fd15c0a57336bc97c69d`
- Final ELF SHA-256: `d72b416add1c43bb5e82e1bf42c5e82712666d6c6e8478674f3d302148c657f0`
- Fixed PA-220 source baseline: `sdzbzyh/pa220-openwrt@801717ba6a8a59ed2cfd53bf3e1b19841fecd2e8`

## Physical test setup

- PA-220 IPv4: `192.168.2.2/24`
- Windows test host IPv4: `192.168.2.10/24`
- TCP destination: `192.168.2.2:80`
- Active PA-220 interface reported by U-Boot: `octrgmii0`
- Link: 1000 Mbps, full duplex, AGL port 24
- U-Boot foreground command: `pa220 web`
- Console reported: foreground lifecycle started, TCP RX hook on port 80, native TCP IPv4/Ethernet output enabled for one on-link peer.
- Windows PowerShell client reported both `TCP CONNECTED` and `TCP DATA SENT`.

## Packet evidence

The source evidence is `PA220-Test/p5-1d-peer.txt` (blob SHA `4206e7d9d1c930848ece0bbe6e09a3cd2c02815a`).

At 13:07:51 on 2026-11-21:

1. Windows host `3C-97-0E-D4-E1-0E` sent an ARP request for `192.168.2.2`.
2. PA-220 replied that `192.168.2.2` is at `08-30-6B-E1-BE-08`.
3. Windows sent TCP SYN from `192.168.2.10:5464` to `192.168.2.2:80`, sequence `824783136`.
4. PA-220 sent SYN-ACK from `192.168.2.2:80` to `192.168.2.10:5464`, sequence `3232238604`, acknowledging `824783137`.
5. Windows sent the final ACK, acknowledging `3232238605`.

This directly demonstrates that the PA-220 emitted the SYN-ACK on Ethernet and completed the TCP three-way handshake.

At 13:07:51.842699, Windows sent a 10-byte PSH+ACK payload with sequence range `824783137:824783147`. The test program's payload was ASCII `P5-1D-TEST`. At 13:07:51.842940, PA-220 returned an ACK acknowledging `824783147`. This confirms that the 10-byte segment reached the PA-220 TCP receive path and was acknowledged. The packet decoder's `HTTP` label is port-based protocol dissection, not evidence that the payload was an HTTP request.

## MAC address observation

The temporary U-Boot environment had previously displayed:

```
ethaddr=08:30:6b:e1:be:00
```

However, the physical capture shows the PA-220 ARP reply and TCP Ethernet source MAC as:

```
08-30-6B-E1-BE-08
```

The Windows host learned and used the latter address, and the TCP exchange succeeded. The discrepancy remains unexplained. Do not change the MAC based only on this observation; investigate the driver/hardware MAC source read-only in a later step if it is relevant.

## Results and limits

| Check | Result |
|---|---|
| ARP request/reply | PASS |
| PA-220 native Ethernet SYN-ACK | PASS |
| TCP three-way handshake | PASS |
| 10-byte client payload transmitted | PASS |
| PA-220 ACK of all 10 payload bytes | PASS |
| Payload application/HTTP parsing | Not in scope; not tested |
| FIN orderly close behavior | Not yet verified |
| RST handling / abnormal close cleanup | Not yet verified |
| Timeout and retry cleanup | Not yet verified |
| MAC `BE:00` vs observed `BE:08` explanation | Open |

The capture also contained a console message `Receive error (code 11: rgmii rx), dropping`. The packet evidence above independently confirms successful ARP and TCP exchange for the tested peer; the isolated message is not sufficient to classify this tested flow as failed.

## Correction to the earlier safety-preparation status

The earlier `20-ram-boot-safety-audit.md` recorded that execution was blocked before the user had reported launching the image. Subsequent user-observed evidence now establishes that the P5-1D image was loaded into RAM and launched, and that the foreground TCP test ran. This report records the later observed state; it does not retroactively claim that the earlier audit's safety concern was incorrect.

The startup path was previously audited as containing automatic CPLD writes. The device status subsequently observed by the user included CPLD register values, but this packet-validation report does not claim that no CPLD side effects occurred. Continue to avoid all intentional CPLD writes and do not repeat the RAM launch.

## Safety constraints for follow-up

- Do not reboot, reset, power-cycle, or run `go` again.
- Do not issue CPLD writes or commands that may write CPLD registers.
- Do not run `saveenv`, SPI erase/write/update, eMMC/NAND writes, firmware flashing, or persistent environment changes.
- Do not restart `pa220 web` unless the user confirms that the foreground service has exited and explicitly agrees.
- Before any close/cleanup test, first establish whether the foreground service is still running without interrupting it.
- Do not repeat the successful SYN/SYN-ACK/ACK or payload test. The next useful hardware test is orderly TCP FIN close, followed by targeted RST/timeout cleanup only if safe and necessary.
- Do not begin HTTP parser work based on this result alone.

## Scope

This report records packet evidence and observed behavior only. It does not claim application-layer HTTP functionality, successful orderly close, or full validation of all TCP error paths.
