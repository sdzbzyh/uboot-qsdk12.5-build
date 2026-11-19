# P3 历史审计持久检查点（2026-11-19）

<badge>IN PROGRESS</badge>

## 当前状态

- 任务：P3 全阶段历史证据追溯、仓库查缺补漏与最终归档。
- 当前批次：Batch 1 — 远端基线；Batch 1b — 原始日志路径与 RAM/TLV 补充；Batch 2 — P3 Git 历史时间线及补丁/日志证据对照；Batch 3 — 0002 构建、TLV/时钟和继承 SPI 证据补录；Batch 4 — bootmem 区间算术复核与补丁错误追溯；Batch 5 — P3 阶段决策及 CLI/bootmem 历史补充；Batch 6 — EEPROM 容量边界与 System.map 证据补充；Batch 7 — 固定源码 clock 调用链与 DDR 测量语义复核；Batch 8 — P3 构建产物 GitHub tree 归档核验。
- 审计是否完成：**否**。
- P3 COMPLETE 是否可判定：**否（审计未完成）**。
- P4 实际开发：**禁止开始**。当前远端已有 P4 计划/源码审计文件，不代表本次获准开展 P4 开发。
- 安全：不执行设备命令，不重建、不 RAM 启动、不写 EEPROM/CPLD/Flash/eMMC/NAND/USB，不分配 named block，不执行 mtest。

## 本批审计范围

- 历史上下文检索覆盖：检索结果包含 2026-11-18 项目 P0/P1/P2 会话索引，以及 2026-11-19 上午 P3/0002/TLV/SCLK/阶段决策的若干摘要。
- 本批没有取得 P3 从首次开始到本次之前的完整会话清单，也没有完整逐字原始输出。
- 因此本批只能建立首批证据账本与 GitHub 基线，不可声称历史范围已遍历。
- 账本：`doc/pa220/reports/P3-HISTORICAL-EVIDENCE-AUDIT-2026-11-19.md`。

## 本批已核验的 GitHub 事实

- 仓库：`sdzbzyh/uboot-qsdk12.5-build`。
- 分支：`feature/pa220-ubootkit-web`（分支存在）。
- 审计开始时分支 HEAD：`0a6f080a58c9a93e3d4950f8898fc46528d55618`，commit message：`Reconcile final P3 TLV and SPI NOR test statuses`，时间：2026-11-19 09:41:36Z。首批审计账本与检查点已提交；当前 HEAD 以本检查点最后回读值为准。
- 已回读 `doc/pa220/TEST-PLAN.md`、`P3-START-2026-11-19.md`、`P3-READONLY-AUDIT-2026-11-19.md`、`P3-COMPLETION-2026-11-19.md`、`P4-PLAN.md`、0001/0002 patch 正文。
- 审计期间新增提交：账本首版 `37cd1f5`、检查点首版 `137dc6f`、账本补充 `a182e1c`、检查点更新 `e1aa5b7`、EEPROM transcript `b8fe267`、RAM validation transcript `7944920`、test-log README 规则 `41e0bc0`；均位于目标分支，未强推。
- board/clock 原始日志的创建提交 `8873182` 已核验，原始文件 24 行；文件确实在仓库根目录 `reports/`。
- `doc/pa220/test-logs/2026-11-19_P3_tlv_reader_001.txt` 的实际正文是源码摘录（TLV reader/print/dispatch/config），不是设备串口输出；保留原文件并在账本中纠正其证据属性。
- `doc/pa220/test-logs/2026-11-19_P3_tlv-eeprom-end-read.session-transcribed.txt` 保存可逐字恢复的用户命令与输出；`...stage2-tftp-validation.session-transcribed.txt` 只保存可恢复摘要，明确非逐字原始日志。
- 当前 HEAD 中 0001 blob：`8cbca887ee554b94e1ea51d73a8adf36d3f49c36`；0002 blob：`f7427063c29cfa2fe39c17b9257d998ddc68b2f5`。
- 原始 board/clock 串口日志已在仓库根目录 `reports/P3-BOARD-CLOCK-2026-11-19-serial.log` 找到并回读，blob `1db8f7cbcfedcda5dfbd302308728504b6c76be1`；此前 `doc/pa220/reports/` 的 404 是错误路径探测，不是文件缺失。
- 本地工作区状态：NOT INSPECTED；远端 GitHub API 无法证明本地是否存在未提交修改。
- 远端 `TEST-PLAN.md` 已新增第 9 节，明确 P3 历史证据审计 IN PROGRESS，并规定在审计完成前不得开始 P4 实际开发。
- `P4-PLAN.md` 已改为 PAUSED（仅暂停执行，不删除历史计划/源码审计）。
- `P3-COMPLETION-2026-11-19.md` 已追加历史审计覆盖说明，保留当日历史 COMPLETE 决策但明确本次审计尚未完成。

## 本批证据统计（临时）

- 已登记账本条目：53（其中 52 条为 P3 证据/审计事项，1 条为 P3 起点前的 P2 继承风险引用；若干仍是历史检索摘要或助手声明，证据等级未升级）。
- 已发现需要继续核验的事项：构建原始日志/ELF-MAP-BIN 归档、TLV print 原始输出、SPI sf probe 原始输出、RAM 地址覆盖与 alias 证据、0002 补丁修复全过程、阶段状态一致性、本地工作区状态未知。board/clock 日志路径已解决。
- 已完成修复：审计账本与检查点已创建并回读；board/clock 日志路径误判已纠正；已创建 EEPROM 末端读取与 Stage2/TFTP 校验两份 session-transcribed 记录；test-logs README 已说明该类文件不是原始设备捕获。
- 尚未修复：全部疑点。
- 尚未核验的助手声明：P3 COMPLETE 的完整证据闭环、0002 构建细节、SPI probe 实测原始输出、SCLK 初始化调用链、P4 首批审计声明及其文件对应关系。

## 下一批具体目标

1. 继续按时间顺序恢复 2026-11-19 P3 初始 CLI/RAM/bootmem 会话的逐字用户输出和中断点。
2. 追踪 0001 的应用、构建、ELF/MAP/BIN 校验、原厂 Stage2/TFTP 启动原始记录。
3. 追踪 `0x81000000` 加载地址、镜像覆盖/CRC 变化、`bdinfo`、named block、cached/uncached/physical alias 讨论。
4. 已核实 `reports/P3-BOARD-CLOCK-2026-11-19-serial.log` 原始日志和提交 `8873182`；继续核对报告交叉引用与 P3 完成状态的证据支撑。
5. 已通过 Git commit 历史追踪 0001 hunk 修复链（`794b6d8` 至 `679a926`）和 0002 patch 修复链（`32e4dbe`、`aa718c6`、`5be99f3`、`cf5a927`）；继续从会话恢复实际 patch-check、构建与文件恢复操作的用户原始输出。
6. 追溯 0x81000000 TFTP/bootloadervalidate 用户记录、旧镜像启动前后 CRC/覆盖区间（含 0x810000c0–0x810200bf 与首 16 KiB 细分）及相关链接符号。
7. 追溯 TLV 五个 tuple 的完整原始 print/display、EEPROM 末端读取输出、SPI probe 的证据级别；必要时只创建明确标记 session-transcribed 的文本日志。
8. 检查 P3-START 的构建产物、ELF/MAP/BIN 哈希、原始 build log 是否实际归档；确认 P3-START 中各项 PASS 的原始证据级别。
9. 核对 `TEST-PLAN.md`、P3-READONLY-AUDIT、P3-COMPLETION、ROADMAP 的状态和交叉引用；添加历史审计期间的状态覆盖说明，避免误入 P4 开发。
10. 每完成一个小批次，追加账本并更新本检查点；若无法确认原始输出，明确保留 EVIDENCE MISSING。

## 最近维护提交

- `70641513a869fc9f033382ac708aa75837101b80`：扩充 P3 历史证据账本至 30 条。
- `bf47ff66a15650e217672096dba016b00ac40532`：记录 P3 历史时间线第二批检查点。
- `b8fe2676ee4ff04b9964a8192304a16189255d4d`：EEPROM 末端读取 session-transcribed 记录。
- `79449201ce2eaf754832ad5b609ee47097fbcec5`：Stage2/TFTP 校验 session-transcribed 记录。
- `41e0bc08695194181cca12706c0b913eb67af985`：test-logs README 增加 session-transcribed 规则。
- `ea8f035415eca179d4e9b4d2d2c2b23673a5c340`：TEST-PLAN 第 9 节审计覆盖说明。
- `4edfb54835be763bd9eb71c7c4d6bd7da23be13b`：P4-PLAN 暂停。
- `4923a37e70c3f0bd2e746b0d969f237451250ecf`：P3 completion 历史审计附录。
- `8848f8bf823f83ee3da82dd640469fd39631143c`：CLI/bootmem session-transcribed。
- `e31d482c58d971d6b3f01ee7ec00af09c54b84e0`：0002 构建产物 session-transcribed。
- `1cb71bace99ac5335832d890528c51bbb5a286a2`：TLV tuple 结构 session-transcribed。
- `f281e8077990aa86f0484fe9ba00454794e1d32c`：继承 SPI probe session-transcribed。
- `7d3a43d7ff3c83f774ac4a1453e2171ab56dcdfa`：更正 SPI 证据来源。
- `8b1010865e9e936aff18fb1319984ac7321b3f85`：TEST-PLAN 更正 SPI 来源。
- `f1813a61b168b4134fa0b91b44ddb0182ca67cde`：P3 completion 补充 SPI 来源。
- `e76d4a51c5af3a3e801b471fbd596edb5920d107`：P3 source audit 补充 SPI 来源。
- `0d5073ed766837e2939180f19081cb4eff1b3c3f`：ROADMAP 标记 P3 历史审计 hold。
- `5aa0024a42d84b72c57762872da396d9be26eea3`：追加 bootmem 纠错、SCLK 与 no-repeat 证据。
- `bd2ac814a00e07b6bb8a7016c7141ad2b7f89c2c`：修正 P3-START 的 __boot_vector1__ 区间终点。
- `65e77d1ddbbb4d0d0a4d71444a6bcc5860d860ae`：记录该算术勘误。
- `ac5036174e6b283d524f5efe43c16fa289bae6e8`：补录 0002 line 88/line 65 hunk 损坏与用户纠正。
- `2323b6fe612455fecd7efd2ca9326c2b418a60f7`：TEST-PLAN 引用 bootmem 算术勘误。
- `ef4eab9c4015fdc099ab7888c8c2637cbd2d05fc`：补充 CLI/bootmem session-transcribed 细节。
- `42f3f05218f69e8d84e712b2f07bfe8bd961cc99`：补充 TLV 地址/容量限制说明。
- `ea628bfa854a15ac3bc649ab51d825b365048a0d`：登记 0001 System.map 空文件及 EEPROM 容量证据。
- `b9aec5d43882225dd826a8f30448bde49ee435bc`：P3-START 区分主 MAP 与空 System.map。
- `ceb58e77519091ebbe0e32c3eb3430a6a0fc6e9c`：TEST-PLAN 补充 MAP/EEPROM 容量限制。
- `d98682ac27d162657f0b1d8a4454f3c89e8777c9`：board/clock 汇总报告增加历史审计覆盖说明。
- `c6164d3f7587873ffc20492628c2479b4fe95496`：P3 源码审计报告增加历史审计覆盖说明。
- `5b2731a41e599dba2ba32698ab1fb3b8e94c10ed`：补录固定源码 clock 语义证据。
- `2ba63999bbce8aa6b1d9162d0a5e5334693a2a8f`：更正 DDR clock source 说明。
- `40bcb51986c8ffe4a05fc8d991041eed2a9a1f49`：P3 完成报告记录 DDR 注释限制。
- `e7c8ca9fbd09d443de13cf948e6fbafe41c5ff4f`：TEST-PLAN 登记 DDR clock note FAIL。
- `0e5c81256bf479b1a1c51ba5d3cc85eeeda55868`：账本记录 EEPROM board/MAC 来源交叉验证。
- `cb618b4c0fcd15d775ef8ff45ac0a104af91ac5a`：P3 完成报告更正 board/MAC 来源。
- `06ad3773f6daee988734e553f2ee029249f223b2`：P3 源码审计报告补充 board/MAC 来源。
- `2a9ea9319beec6a0c74a8e812701f102e64e52c6`：TEST-PLAN 更新 board/MAC 来源。
- `1d0bfb8566fd68e345d55d9ffa10217359f3cc8a`：记录 P3 起点前 P2 RAM CRC 风险引用。
- `959c78511930e2bb9395d7adac9bfb617a90fc27`：P3 build artifact tree audit 报告。
- `727e45a4f99b5ceca2cfca4542c343afba4ffe7b`：登记 P3 构建原件缺失。
- `a40fe1794cfb68ad2e23f2db827070d31402bbb8`：TEST-PLAN 记录 P3 artifact archive status。
- `1c7c87479b04b10471a3e76ac74f8749879fcc19`：P3-START 链接构建产物审计。
- `f7dfde53ca4a5fbd7022112f7cf9e61d3ff7b2fb`：登记 P3 commit span、阶段决策、0001 apply 和 artifact 路径。

## Git 历史范围核对

- P2 收尾提交 `47805be639ad39cb5f03f9e527b2727e74ee5ea9` 到原 P3 完成 HEAD `0a6f080a58c9a93e3d4950f8898fc46528d55618`，GitHub compare 返回 37 commits，时间范围 2026-11-19 06:58:00Z–09:41:36Z，改动 13 个文件。
- 该 Git 历史范围已完成仓库层面的初步核对；并不代表所有历史会话已逐条审完。

## 新发现的仓库内容错误

- P3-START 原先将 __boot_vector1__ base `0x0fff5c00` + size `0x8000` 的终点写成 `0x10003c00`；算术应为 `0x0fffdc00`。该处已按用户历史 namedprint 值更正，不重新运行设备命令。
- P3 的 System-octeon_pa220.map 在 0001 构建中也曾为空；不得与有内容的主 MAP 混为一谈。
- EEPROM 的 256-byte tuple 长度上限不是物理容量或全设备软件访问窗口；物理容量/寻址映射仍 UNKNOWN。

## 本批新增源码核验结论

- 固定源码 commit `801717ba6a8a59ed2cfd53bf3e1b19841fecd2e8` 已回读 `board_octeon.c`、`cvmx-clock.c`、`octeon_dram.c`、`lib_octeon_shared.c`、`octeon_board_common.c`、`pa220_board.c` 和 `octeon_eeprom_types.h`。
- `octeon_setup_clocks` 在 init_sequence index 1，早于 PA220 `early_board_init` index 12；SCLK 由 `CVMX_RST_BOOT.pnr_mul × 50 MHz` 写入 `gd->bus_clk`，不是 Stage2 handoff 直接传值。
- DDR 在 `octeon_init_dram` 后可能由 CN70XX 的 IPD/DCLK counter ratio 动态测量后写回 `gd->mem_clk`。0002 的统一 NOTE/DDR source 说明不准确，列为 P3-H050 FAIL。该问题尚未修复；不擅自改已验证补丁或重建。

## 本批新增构建归档核验结论

- GitHub recursive tree：42,404 entries，truncated=false。
- P3 专属 artifacts 目录不存在；P3 0001/0002 ELF、BIN、主 MAP、完整 build log 均未找到。
- P2-3 artifacts/pa220/p2-3/20261118 存在，但属于 11/18 P2-3，BIN 大小与 P3 不同，不可冒充。
- 根目录 pa220-build.log 和 pa220-maintenance-build.log 均为 0 bytes。
- board/clock 原始串口日志存在；其余 CLI/TLV/Stage2 记录多为 session-transcribed。

## 本批最新远端状态

- 检查点更新前读取 HEAD：`c6164d3f7587873ffc20492628c2479b4fe95496`；本检查点提交后 HEAD 将再次推进。
- 已确认所有新建/修改文件均在 `feature/pa220-ubootkit-web`；本批未修改 main、固定源码仓库或执行强推。
- 本地 Debian 工作区仍未能通过 GitHub 远端接口检查；不能声称其 clean，也没有触碰用户本地未提交内容。

## 上下文与继续规则

- 不声称有精确 token 计数；当前可用上下文无法可靠量化。
- 已在继续历史检索前先写入本检查点，后续以账本为主。
- 如果下一批检索与安全提交所需上下文不足，先更新本检查点，再交接新会话；不得省略历史范围。
- 禁止重做已通过硬件测试来补文件。
