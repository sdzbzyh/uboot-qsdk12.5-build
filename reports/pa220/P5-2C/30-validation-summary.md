# PA-220 P5-2C validation summary — interim (superseded)

> **SUPERSEDED BY FINAL ACCEPTANCE (2026-11-23):** This report records an earlier intermediate workflow state and is retained for history only. Its `NOT COMPLETE` conclusion and `NOT RUN` entries do not describe the final P5-2C state. The final Host, full target build/link, and physical HTTP/TCP results are recorded in [`ACCEPTANCE.md`](ACCEPTANCE.md). Do not use this interim report as the current status.

Date: 2026-11-23  
Branch: `feature/pa220-ubootkit-web`  
Integration commit tested: `3526880c4987fe4e7ee03109f06faf10e163a3a2`

## Results

| Area | Result | Evidence |
|---|---|---|
| Dedicated P5-2C push workflow | Triggered | Run 45137814338 |
| OCTEON N32 object compilation | PASS | Job 137568193204 |
| N32 relocatable link | PASS | Artifact 14800038825 |
| ELF32 / ABI2 / OCTEON flags | PASS | `24-n32-linked-elf-header.txt` |
| Dynamic allocation dependency check | PASS | `25-n32-undefined-symbols.txt` |
| Per-function stack usage | Collected; review incomplete | `.su` files in artifact |
| Static memory report | Collected | `22-object-sizes.txt`, `27-linked-symbol-sizes.txt` |
| Independent P5-2C Host lifecycle harness | NOT RUN | Harness path does not yet exist |
| Full U-Boot final executable/image link | NOT RUN | Current workflow links the six relevant objects relocatably |
| Physical PA-220 validation | NOT RUN | Must be performed separately |

## Actions artifacts

- OCTEON artifact: [14800038825](https://github.com/sdzbzyh/uboot-qsdk12.5-build/actions/runs/45137814338/artifacts/14800038825)
  - SHA256 digest: `5f1d57c24bfb6c42f555d9b8c0a408c94fb56c8757a7cb5975fcdac8c15bf69e`
  - Includes the N32 relocatable object, object ELF headers, undefined symbols, object sizes, symbol sizes, stack-usage reports and complete OCTEON build log.
- Host evidence artifact: [14801176146](https://github.com/sdzbzyh/uboot-qsdk12.5-build/actions/runs/45137814338/artifacts/14801176146)
  - SHA256 digest: `5cf118e3156bd835ea58f0c1a17f7e9b7b80dd2bc6a76de1eef1082fc7851b55`
  - Records the missing independent lifecycle harness and failed Host job; it is not a passing test report.

## P5-1D workflow concurrency correction

Commit `13a27333528f950ede2641b86327e3b853ecc141` removed shared-branch report/patch commits from the P5-1D workflow. Its subsequent Run 45137699302 did not encounter the previous Rebase conflict. The build still failed independently because the P5-1D full-image workflow did not stage the HTTP request header required by the P5-2C command source (`net/http_request.h`); this is an integration/build-workflow issue, not evidence of a P5-2C runtime defect.

## Overall acceptance

P5-2C is **NOT COMPLETE**. The OCTEON N32 object-level build/link is verified, but the independent Host lifecycle suite, complete stack call-graph audit, full firmware link and physical-device validation remain outstanding. No P5-2C runtime or physical-device success is claimed.
