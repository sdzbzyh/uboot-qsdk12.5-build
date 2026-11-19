# P3 历史审计持久检查点（2026-11-19）

<badge>IN PROGRESS</badge>

## 当前状态

- 任务：P3 全阶段历史证据追溯、仓库查缺补漏与最终归档。
- 当前批次：Batch 1 — 初始远端基线与已知证据索引。
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
- 当前分支 HEAD：`0a6f080a58c9a93e3d4950f8898fc46528d55618`，commit message：`Reconcile final P3 TLV and SPI NOR test statuses`，时间：2026-11-19 09:41:36Z。
- 已回读 `doc/pa220/TEST-PLAN.md`、`P3-START-2026-11-19.md`、`P3-READONLY-AUDIT-2026-11-19.md`、`P3-COMPLETION-2026-11-19.md`、`P4-PLAN.md`、0001/0002 patch 正文。
- 当前 HEAD 中 0001 blob：`8cbca887ee554b94e1ea51d73a8adf36d3f49c36`；0002 blob：`f7427063c29cfa2fe39c17b9257d998ddc68b2f5`。
- `doc/pa220/reports/P3-BOARD-CLOCK-2026-11-19-serial.log` 不存在（404）。报告引用的是仓库根目录 `reports/P3-BOARD-CLOCK-2026-11-19-serial.log`，该路径仍待单独核验。
- 本地工作区状态：NOT INSPECTED；远端 GitHub API 无法证明本地是否存在未提交修改。
- 远端 `TEST-PLAN.md` 当前已标 P3 COMPLETE、P4 IN PROGRESS；这是待审计的历史声明，不作为本次审计完成状态。

## 本批证据统计（临时）

- 已登记账本条目：17。
- 已发现需要继续核验的仓库维护疑点：至少 5 类（日志路径/原始串口证据、构建原始日志与产物、TLV/SPI 原始输出、阶段状态提前标记、工作区状态未知）。
- 已完成修复：0（本批先建立审计账本与检查点；不覆盖任何既有文件）。
- 尚未修复：全部疑点。
- 尚未核验的助手声明：P3 COMPLETE、0002 最终构建/实机测试细节、TLV/SPI 实测原始证据、P4 首批审计声明及其文件对应关系。

## 下一批具体目标

1. 继续按时间顺序恢复 2026-11-19 P3 初始 CLI/RAM/bootmem 会话的逐字用户输出和中断点。
2. 追踪 0001 的应用、构建、ELF/MAP/BIN 校验、原厂 Stage2/TFTP 启动原始记录。
3. 追踪 `0x81000000` 加载地址、镜像覆盖/CRC 变化、`bdinfo`、named block、cached/uncached/physical alias 讨论。
4. 从 GitHub 回读 `reports/P3-BOARD-CLOCK-2026-11-19-serial.log` 根目录候选路径及相关提交；检查提交历史和文件引用关系。
5. 追踪 0002 丢失/恢复、hunk 损坏/修复、patch-check、构建和实机验证全过程。
6. 每完成一个小批次，追加账本并更新本检查点；若无法确认原始输出，明确保留 EVIDENCE MISSING。

## 上下文与继续规则

- 不声称有精确 token 计数；当前可用上下文无法可靠量化。
- 已在继续历史检索前先写入本检查点，后续以账本为主。
- 如果下一批检索与安全提交所需上下文不足，先更新本检查点，再交接新会话；不得省略历史范围。
- 禁止重做已通过硬件测试来补文件。
