# P3 历史审计持久检查点（2026-11-19）

<badge>IN PROGRESS</badge>

## 当前状态

- 任务：P3 全阶段历史证据追溯、仓库查缺补漏与最终归档。
- 当前批次：Batch 1 — 初始远端基线与已知证据索引；Batch 1b — board/clock 原始日志路径与 RAM/TLV 证据补充。
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
- 当前 HEAD 中 0001 blob：`8cbca887ee554b94e1ea51d73a8adf36d3f49c36`；0002 blob：`f7427063c29cfa2fe39c17b9257d998ddc68b2f5`。
- 原始 board/clock 串口日志已在仓库根目录 `reports/P3-BOARD-CLOCK-2026-11-19-serial.log` 找到并回读，blob `1db8f7cbcfedcda5dfbd302308728504b6c76be1`；此前 `doc/pa220/reports/` 的 404 是错误路径探测，不是文件缺失。
- 本地工作区状态：NOT INSPECTED；远端 GitHub API 无法证明本地是否存在未提交修改。
- 远端 `TEST-PLAN.md` 当前已标 P3 COMPLETE、P4 IN PROGRESS；这是待审计的历史声明，不作为本次审计完成状态。

## 本批证据统计（临时）

- 已登记账本条目：23（其中若干仍是历史检索摘要或助手声明，证据等级未升级）。
- 已发现需要继续核验的事项：构建原始日志/ELF-MAP-BIN 归档、TLV print 原始输出、SPI sf probe 原始输出、RAM 地址覆盖与 alias 证据、0002 补丁修复全过程、阶段状态一致性、本地工作区状态未知。board/clock 日志路径已解决。
- 已完成修复：审计账本与检查点已创建；board/clock 日志路径误判已纠正。尚未补录 session-transcribed 日志。
- 尚未修复：全部疑点。
- 尚未核验的助手声明：P3 COMPLETE 的证据闭环、0002 最终构建细节、SPI probe 实测原始输出、SCLK 初始化调用链、P4 首批审计声明及其文件对应关系。

## 下一批具体目标

1. 继续按时间顺序恢复 2026-11-19 P3 初始 CLI/RAM/bootmem 会话的逐字用户输出和中断点。
2. 追踪 0001 的应用、构建、ELF/MAP/BIN 校验、原厂 Stage2/TFTP 启动原始记录。
3. 追踪 `0x81000000` 加载地址、镜像覆盖/CRC 变化、`bdinfo`、named block、cached/uncached/physical alias 讨论。
4. 继续核对 `reports/P3-BOARD-CLOCK-2026-11-19-serial.log` 的提交历史与引用关系；检查根目录 `reports/` 和 `doc/pa220/reports/` 报告是否存在重复/矛盾版本。
5. 追踪 0002 丢失/恢复、hunk 损坏/修复、patch-check、构建和实机验证全过程。
6. 追溯 0x81000000 TFTP/bootloadervalidate 用户记录、旧镜像启动前后 CRC/覆盖区间（含 0x810000c0–0x810200bf 与首 16 KiB 细分）及相关链接符号。
7. 追溯 TLV 五个 tuple 的完整原始 print/display、EEPROM 末端读取输出、SPI probe 的证据级别；必要时只创建明确标记 session-transcribed 的文本日志。
8. 每完成一个小批次，追加账本并更新本检查点；若无法确认原始输出，明确保留 EVIDENCE MISSING。

## 上下文与继续规则

- 不声称有精确 token 计数；当前可用上下文无法可靠量化。
- 已在继续历史检索前先写入本检查点，后续以账本为主。
- 如果下一批检索与安全提交所需上下文不足，先更新本检查点，再交接新会话；不得省略历史范围。
- 禁止重做已通过硬件测试来补文件。
