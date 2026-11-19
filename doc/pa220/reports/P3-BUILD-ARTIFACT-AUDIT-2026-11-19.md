# P3 构建产物归档核验（2026-11-19）

> 状态：**EVIDENCE MISSING — P3 原始构建日志及二进制产物未归档到当前 GitHub 分支**。本文件只记录远端文件树核验结果和历史用户证据，不重建、不补造二进制或 build log。

## 核验范围与方法

- 仓库：`sdzbzyh/uboot-qsdk12.5-build`
- 分支：`feature/pa220-ubootkit-web`
- 核验 HEAD：本文件创建前的分支 HEAD 见本次审计检查点。
- 通过 GitHub Git Trees API 对当前分支执行 recursive tree 检索（tree 共 42,404 项，truncated=false）。
- 重点检查 `artifacts/pa220/`、P3 报告/日志、ELF/BIN/MAP、build.log 和相关产物名。
- 远端树检查不等于本地 Debian 工作区检查；本地工作区仍 NOT INSPECTED。

## P3 0001 构建：历史用户证据

来自 `doc/pa220/reports/P3-START-2026-11-19.md` 的记录：

| 产物 | 历史大小 | 历史 SHA-256 | 当前 GitHub 是否有对应 P3 二进制文件 |
|---|---:|---|---|
| `u-boot-octeon_pa220` ELF | 7,739,990 bytes | `96bc39ca233c8374b0dbbfbe50428a9d2b4701a49f330ab8ddcec6bd05b3c00f` | 否 |
| `u-boot-octeon_pa220.map` 主 MAP | 576,978 bytes | `d2b48f73f67562bf80fdb4d3cbe5bb6d06377e5686f507620c46743d2f9c0944` | 否 |
| `u-boot-octeon_pa220.bin` | 1,358,312 bytes | `f8738a4c92e748e37462ab560bff3f33360f4b03269b372b06f6563a18037f73` | 否 |

- Image data CRC：`0x1c54a9b4`；header CRC：`0xea85a539`。
- ELF entry / LOAD VAddr / PAddr：`0xc0000000`。
- 辅助 `System-octeon_pa220.map` 曾为 0 bytes，empty SHA-256：`e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855`。
- 历史用户消息说明 ELF/MAP 位于固定 integration checkout 的 U-Boot 子目录，只有 BIN 被复制到 output 目录；输出目录缺少 ELF/MAP 不代表它们未生成。
- 当前 GitHub 只有报告中的哈希和大小，没有对应 P3 ELF/MAP/BIN 原件。

## P3 0002 构建：历史用户证据

来自 `doc/pa220/test-logs/2026-11-19_P3_0002-build-artifacts.session-transcribed.txt`：

| 产物/指标 | 历史用户结果 | 当前 GitHub 原件 |
|---|---|---|
| BIN | 1,362,280 bytes | 未找到 |
| ELF SHA-256 | `e3f2d4799f9de0d71d0d25f411953ea62ef67993cdc0a2378c0fbddd5f68ef08` | 未找到 |
| BIN SHA-256 | `643b1bf60d31550efee2efe3a95c6cc9d5318b5ee75b32b526f00a966042bf28` | 未找到 |
| 主 MAP SHA-256 | `5764da78b816a2ef47e6796f1e307cdf8dccaeb5de74d2c5054052f06c10814c` | 未找到 |
| System MAP | 曾 0 bytes；另一次 host nm 生成 3,426 lines / 100,700 bytes，SHA-256 `e1c6ef977b712160c515892de421a820245d3a1b08b5de550da8a182c9cd8751`；后续又有空文件记录 | 未找到；状态冲突按时间点保留 |

- Build exit code：0（用户历史消息）。
- Text/data/BSS：1,332,156 / 30,108 / 1,671,340 bytes。
- Image data CRC：`0x0e351bbc`；header CRC：`0xdcc5dc0c`。
- ELF 为 ELF32 big-endian MIPS EXEC，entry `0xc0000000`，2 program headers、21 sections。
- 完整原始 build log、ELF、BIN、主 MAP 和各次 System MAP 仍未在 GitHub 找到。

## 当前 GitHub 文件树核验结果

- `artifacts/pa220/p2-3/20261118/` 存在完整 P2-3 归档：build.log 1,960,797 bytes、ELF 7,709,260 bytes、BIN 1,357,960 bytes、主 MAP 576,978 bytes、配置快照及 artifact audit。
- 该目录日期为 2026-11-18，属于 P2-3；其 BIN 大小 1,357,960 与 P3 0001 的 1,358,312、P3 0002 的 1,362,280 均不同，不能冒充 P3 产物。
- 根目录 `pa220-build.log` 当前为 0 bytes。
- 根目录 `pa220-maintenance-build.log` 当前为 0 bytes。
- 当前树未发现 P3 专属 `artifacts/pa220/p3/20261119/` 或等效目录。
- 当前树未发现 P3 0001/0002 对应的 ELF、BIN、主 MAP、完整 build log。
- `reports/P3-BOARD-CLOCK-2026-11-19-serial.log` 存在，1,026 bytes，blob `1db8f7cbcfedcda5dfbd302308728504b6c76be1`。
- `doc/pa220/test-logs/2026-11-19_P3_tlv_reader_001.txt` 存在，41,276 bytes，但内容是源码摘录，不是设备串口日志。
- 其他 P3 CLI/bootmem、TLV、Stage2/TFTP 记录是明确标注的 session-transcribed 文本，不是原始捕获。

## 处理结论

1. P3 0001 和 0002 构建结果的历史用户证据与哈希/CRC 已保存在 P3 报告和 session-transcribed 账本中。
2. P3 二进制原件、完整 build log、ELF/MAP 原件目前无法从 GitHub 当前分支恢复，状态为 **EVIDENCE MISSING**。
3. 不从 P2-3 目录复制或改名旧产物冒充 P3；不根据 SHA-256 生成虚假文件；不为补档而重建 0001/0002。
4. 如本地 Debian 构建目录仍保留原始产物，需在后续由用户授权后以原文件、原哈希补归档；当前审计不触碰用户本地工作区。
5. P3 构建“历史执行 PASS”与“原始产物归档完整”是两个不同状态：前者有用户证据，后者未满足。
