# P3 历史审计持久检查点（2026-11-19）

<badge>IN PROGRESS</badge>

## 当前状态

- 任务：P3 全阶段历史证据追溯、仓库查缺补漏与最终归档。
- 当前批次：Batch 1 — 远端基线；Batch 1b — 原始日志路径与 RAM/TLV 补充；Batch 2 — P3 Git 历史时间线及补丁/日志证据对照。
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
- 远端 `TEST-PLAN.md` 当前已标 P3 COMPLETE、P4 IN PROGRESS；这是待审计的历史声明，不作为本次审计完成状态。

## 本批证据统计（临时）

- 已登记账本条目：30（其中若干仍是历史检索摘要或助手声明，证据等级未升级）。
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

## 上下文与继续规则

- 不声称有精确 token 计数；当前可用上下文无法可靠量化。
- 已在继续历史检索前先写入本检查点，后续以账本为主。
- 如果下一批检索与安全提交所需上下文不足，先更新本检查点，再交接新会话；不得省略历史范围。
- 禁止重做已通过硬件测试来补文件。
