# P6-B 32 MiB Exact Reservation Boundary — Device Acceptance

- Date: 2026-11-27
- Repository/branch: sdzbzyh/uboot-qsdk12.5-build / feature/pa220-ubootkit-web
- Fixed source: sdzbzyh/pa220-openwrt@801717ba6a8a59ed2cfd53bf3e1b19841fecd2e8
- Device: PA-220, U-Boot 2013.07, build time Nov 25 2026 - 11:04:13
- Evidence: user-provided serial output and Windows PowerShell output.

**PASS — exact configured 32 MiB maximum accepted by the device and independently verified on the host.**

## Pre-transfer live descriptor

```text
filesize=1000
fileaddr=20000000
loadaddr  : 0x20000000
load base : (not set)
load size : (not set)
__tmp_load: base=0x20000000 size=0x10000000
```

The target is `0x20000000`; the live temporary-load descriptor remains `[0x20000000, 0x30000000)` (256 MiB). The GET command was:

```text
tftpboot ${loadaddr} pa220-p6-get-33554432.bin
```

## Device result

```text
TFTP from server 192.168.2.10; our IP address is 192.168.2.2
Filename 'pa220-p6-get-33554432.bin'.
Load address: 0x20000000
Loading: ##################################################
TFTP GET verified: size=33554432 format=legacy-uImage CRC32=97257c28 SHA256=fe773a13c9bfa1759c32826302e0b9a0e2ba6c88de9841219a2337d17861f526
         1.8 MiB/s
done
Bytes transferred = 33554432 (2000000 hex)
```

After transfer, device reported:

```text
filesize=2000000
fileaddr=20000000
loadaddr  : 0x20000000
load base : (not set)
load size : (not set)
__tmp_load: base=0x20000000 size=0x10000000
```

## Independent host verification

Windows PowerShell commands were run with the file variable assigned first:

```powershell
$f = ".\\pa220-p6-get-33554432.bin"
(Get-FileHash -Path $f -Algorithm SHA256).Hash
(Get-Item $f).Length
```

Observed output:

```text
FE773A13C9BFA1759C32826302E0B9A0E2BA6C88DE9841219A2337D17861F526
33554432
```

The host length and SHA-256 exactly match the target's reported length and digest.

## Acceptance

| Check | Result |
|---|---|
| Full 32 MiB transfer | PASS |
| Target filesize = 0x02000000 | PASS |
| Target fileaddr = 0x20000000 | PASS |
| Format recognition = legacy-uImage | PASS |
| Target CRC32 = 97257c28 | PASS |
| Target SHA-256 matches host | PASS |
| Host file length = 33,554,432 | PASS |
| Live descriptor unchanged | PASS |
| Pcap captured | NOT PROVIDED |

This proves successful reception at the configured 32 MiB exact boundary. It does not prove the 32 MiB+1 rejection stage or failure cleanup. No boot/loader command, `saveenv`, or persistent-storage write was performed.

## Next test

Proceed to the 33,554,433-byte fixture from Actions run [#45938976540](https://github.com/sdzbzyh/uboot-qsdk12.5-build/actions/runs/45938976540), with packet capture active on the Windows Ethernet interface before issuing the GET. Preserve the pcap and complete serial output so the rejection can be attributed to OACK `tsize` validation or DATA reservation enforcement. After failure, inspect `printenv filesize fileaddr` and `pa220 memory`; do not run any loader/boot command.
