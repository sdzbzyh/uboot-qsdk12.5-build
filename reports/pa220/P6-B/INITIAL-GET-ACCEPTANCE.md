# P6-B Initial GET Acceptance — PA-220 RAM-only TFTP success path

- Date: 2026-11-27
- Repository/branch: sdzbzyh/uboot-qsdk12.5-build / feature/pa220-ubootkit-web
- Fixed source: sdzbzyh/pa220-openwrt@801717ba6a8a59ed2cfd53bf3e1b19841fecd2e8
- Evidence: user-provided PA-220 serial and Windows PowerShell output.

**PARTIAL ACCEPTANCE — first 4 KiB success path PASS; P6-B overall remains open.**

## Live device state

U-Boot 2013.07, build time Nov 25 2026 - 11:04:13. Network: ipaddr=192.168.2.2, serverip=192.168.2.10, ethact=octrgmii0.

| Item | Value |
|---|---|
| bdinfo.memsize | 0x200000000 (8 GiB) |
| loadaddr | 0x20000000 |
| __tmp_load base | 0x20000000 |
| __tmp_load size | 0x10000000 (256 MiB) |
| __tmp_load interval | [0x20000000, 0x30000000) |

After transfer, pa220 memory still reported the same loadaddr and descriptor.

## Deterministic fixture

4096 bytes: 64-byte legacy uImage header + 4032-byte inert ASCII payload (repeated `PA220-P6-TFTP-GET-TEST\n`, truncated).

| Header field | Value |
|---|---|
| magic | 0x27051956 |
| timestamp / ih_size | 0 / 4032 |
| ih_load / ih_ep | 0 / 0 |
| ih_dcrc | 705c16e0 |
| ih_os / ih_arch | 5 (Linux) / 5 (MIPS) |
| ih_type / ih_comp | 3 (RAMDisk) / 0 (none) |
| name | PA220-P6-GET-TEST |
| ih_hcrc | b1aa12e3 |

Whole-file CRC32: 9e4df6c8  
Whole-file SHA-256: 83b1bb6b9c006c9d38c6915a985895bfeffd41f24d969aae1b74365041028082

Generator: tools/pa220/make-p6-get-fixture.py. It asserts length, header CRC, payload CRC, whole-file CRC and SHA-256.

## Device transfer

Command: `tftpboot ${loadaddr} pa220-p6-get-4096.bin`

Relevant supplied output:

```text
TFTP from server 192.168.2.10; our IP address is 192.168.2.2
Filename 'pa220-p6-get-4096.bin'.
Load address: 0x20000000
Loading: octrgmii0: Up 1000 Mbps Full duplex (port 24) (AGL)
Receive error (code 11: rgmii rx), dropping
T #####################################################
TFTP GET verified: size=4096 format=legacy-uImage CRC32=9e4df6c8 SHA256=83b1bb6b9c006c9d38c6915a985895bfeffd41f24d969aae1b74365041028082
         0 Bytes/s
done
Bytes transferred = 4096 (1000 hex)
```

The rgmii rx warning is retained. Transfer nevertheless completed and the verifier accepted the expected length, format and digests. This proves this transfer succeeded, not that the link is free of intermittent receive errors.

## Published metadata and host cross-check

Device output:

```text
filesize=1000
fileaddr=20000000
loadaddr  : 0x20000000
load base : (not set)
load size : (not set)
__tmp_load: base=0x20000000 size=0x10000000
```

Device filesize=0x1000 (4096 bytes); fileaddr equals loadaddr. Windows PowerShell reported file length 4096 and SHA-256:

`83B1BB6B9C006C9D38C6915A985895BFEFFD41F24D969AAE1B74365041028082`

This exactly matches the device digest. No pcap was supplied, so no claim is made about negotiated block size, DATA block count or terminal zero-length DATA.

## Acceptance matrix

| Gate | Result |
|---|---|
| Live descriptor precondition | PASS (P6-A) |
| 4 KiB RAM-only TFTP GET | PASS |
| Format recognition | PASS — legacy-uImage |
| Device CRC32/SHA-256 | PASS |
| filesize/fileaddr publication | PASS |
| Host length/SHA cross-check | PASS |
| Descriptor unchanged | PASS |
| Exact reservation boundary | NOT RUN |
| Over-reservation rejection | NOT RUN |
| Interrupted/timeout failure cleanup | NOT RUN |

**Conclusion:** P6-B initial success path accepted; P6-B overall remains incomplete pending boundary/overrun rejection and abort/timeout cleanup. No bootm, bootelf, booti, bootcmd, saveenv or persistent-storage write was performed.

## Next work

Review the production reservation and failure-cleanup paths. Design minimal deterministic exact-boundary and overrun tests; confirm abort/timeout cleanup and metadata invalidation in Host/target tests before the next device round. Keep the observed RX warning visible until repeat testing clarifies whether it is reproducible.
