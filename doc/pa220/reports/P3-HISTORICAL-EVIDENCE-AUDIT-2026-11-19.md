# P3 历史证据审计账本（2026-11-19）

> 状态：**IN PROGRESS / INCOMPLETE**。本账本为首批落盘版本，不代表 P3 全历史会话已遍历。仅将可明确追溯的用户输出、已回读文件和助手声明分级记录；不得把报告中的转述升级为原始串口证据。

## 审计基线

- 集成仓库：`sdzbzyh/uboot-qsdk12.5-build`
- 分支：`feature/pa220-ubootkit-web`
- 本批核验 HEAD：`0a6f080a58c9a93e3d4950f8898fc46528d55618`（通过 GitHub 分支 ref 解析并读取 commit）
- PA-220 固定源码：`IronDemiGod/pa220-openwrt@801717ba6a8a59ed2cfd53bf3e1b19841fecd2e8`
- uBootKit 审计基线：`chenxin527/uboot-qsdk12.5-build@9ce315735ac88fd0a7c7697bc527a8616ed0a972`
- 当前批次尚未检查本地工作区；远端分支/文件状态不等于本地工作区状态。
- 证据级别：USER-OBSERVED（用户贴出的设备/命令输出）；FILE-VERIFIED（本次实际回读仓库文件）；ASSISTANT-CLAIM（历史助手声称，未独立证实）；INFERENCE（推断）。
- 结论状态：PASS / FAIL / BLOCKED / NOT RUN / UNVERIFIED / EVIDENCE MISSING。

## 首批证据条目

| ID | 会话/时间 | 工作主题 | 原始证据或可恢复内容 | 证据等级 | 结论 | 仓库位置 | Git 状态 | 修复动作/处理结果 |
|---|---|---|---|---|---|---|---|---|
| P3-H001 | 2026-11-19 上午；P3 初始会话（历史会话标题待补全） | 固定输入 | 用户明确给定源码 commit `801717ba6a8a59ed2cfd53bf3e1b19841fecd2e8`、集成分支 `feature/pa220-ubootkit-web`；审计基线 `9ce315735ac88fd0a7c7697bc527a8616ed0a972` | USER-OBSERVED（历史检索摘要） | PASS（固定值已核对） | 多份 P3 报告 | 已提交 | 保留固定输入，不修改源码基线 |
| P3-H002 | P3 初始构建会话；2026-11-19 | 0001 patch 应用 | 用户提供/会话摘要记载：固定 checkout 上 `git apply --check` 成功；第二 hunk 有 1 行 offset；hunk 行数由程序化方式核验 | USER-OBSERVED（可恢复摘要，完整终端日志待检索） | PASS（受摘要证据限制） | `patches/pa220/0001-pa220-read-only-maintenance-command.patch` | 已提交；blob `8cbca887ee554b94e1ea51d73a8adf36d3f49c36` | 本次回读补丁正文；后续追查原始命令输出/日志是否归档 |
| P3-H003 | P3 初始构建会话；2026-11-19 | 0001 构建产物 | 报告记录 GCC 4.7.0 / Cavium SDK build 52；image data CRC `0x1c54a9b4`、length `1358120`、header CRC `0xea85a539`；ELF/MAP/object/BIN 大小及 SHA256 见 P3-START 报告 | FILE-VERIFIED（报告正文）；原始构建日志待取 | PASS（报告转录，原始日志证据缺口） | `doc/pa220/reports/P3-START-2026-11-19.md` | 已提交 | 核对是否另有完整 build log、产物索引及原始文件可归档；不重建 |
| P3-H004 | P3 初始 RAM 启动会话；2026-11-19 | 原厂 Stage2/TFTP RAM 启动 | 报告记载用户经原厂 Stage2/TFTP 加载镜像并进入 CLI；版本构建时间 Nov 19 2026 15:20:46 | FILE-VERIFIED（报告正文）；串口原始日志待检索 | PASS（报告转录，完整串口记录缺口） | `doc/pa220/reports/P3-START-2026-11-19.md` | 已提交 | 追查原始会话输出；不得重启补证 |
| P3-H005 | P3 初始 RAM 启动会话 | `loadaddr` 与 `__tmp_load` | `loadaddr=0x20000000`；named block base `0x20000000`、size `0x10000000`；reserved load base/size 环境变量未设置 | FILE-VERIFIED（报告转录） | PASS（字段观察）；地址域安全结论仍 UNVERIFIED | `P3-START-2026-11-19.md` | 已提交 | 保留 alias/physical address 未收敛限制 |
| P3-H006 | P3 初始 RAM 启动会话 | named/free bootmem 区间 | 报告记录 13 named blocks、2 free blocks；`__tmp_load` 区间 [0x20000000,0x30000000)，与 free block 1 起点相接；若干低地址区间半开区间核算 | FILE-VERIFIED（报告转录） | PASS（仅针对报告列出的 descriptor 区间） | `P3-START-2026-11-19.md` | 已提交 | 不将其扩展解释为全物理地址/alias 写入安全证明 |
| P3-H007 | P3 初始 RAM 启动会话 | CLI `status/env/memory` | `help pa220` 列出 status/env/memory；报告列出 board type 20021、DRAM 8192 MiB、CPU 1000 MHz、SPI NOR env 布局、memory block 信息 | FILE-VERIFIED（报告转录）；逐条原始用户输出仍需追溯 | PASS（报告转录） | `P3-START-2026-11-19.md` | 已提交 | 检查 TEST-PLAN 的测试映射及原始日志归档 |
| P3-H008 | P3 源码审计会话；2026-11-19 | board descriptor/MAC | 源码审计记录 global data 缓存字段；MAC tuple 缺失时可能生成 fallback MAC | FILE-VERIFIED | PASS（源码审计）；EEPROM/fallback 实际来源 NOT VERIFIED | `P3-READONLY-AUDIT-2026-11-19.md` | 已提交 | 不宣称 MAC 来自 EEPROM |
| P3-H009 | P3 源码审计会话；2026-11-19 | CORE/SCLK/DDR 语义 | 源码审计记录 `cvmx_clock_get_rate` 在 U-Boot 构建返回 global data；CORE/SCLK/DDR 值不是独立频率计数器测量 | FILE-VERIFIED | PASS（语义审计） | `P3-READONLY-AUDIT-2026-11-19.md` | 已提交 | 对照后续 clock 实机输出和 SCLK 语义历史会话 |
| P3-H010 | P3 源码审计会话；2026-11-19 | 网络 link 查询副作用 | `cvmx_helper_link_get()` 可能更新 LED/SFP 状态；ethact 只是当前接口选择，不是 carrier | FILE-VERIFIED | NOT VERIFIED（物理 link） | `P3-READONLY-AUDIT-2026-11-19.md` | 已提交 | 保留 NOT VERIFIED，不运行 MDIO 探测补证 |
| P3-H011 | P3 源码审计会话；2026-11-19 | SPI/eMMC/USB 只读边界 | 报告：SPI Winbond probe 候选；eMMC mmc_init/startup 可能发送 CMD6 修改 EXT_CSD；USB 电源 GPIO 默认拉低且 usb start 有副作用 | FILE-VERIFIED（源码报告） | eMMC BLOCKED；USB NOT VERIFIED；SPI 初始 NOT RUN，后续状态需核对 | `P3-READONLY-AUDIT-2026-11-19.md` | 已提交 | 追踪后续 SPI `sf probe` 的实际用户输出及归档 |
| P3-H012 | 2026-11-19 约 08:00 UTC 前后历史会话 | TLV EEPROM | 历史检索可恢复：五个 tuple、checksum、END `0xffff`、终止偏移 `0x9a`；EEPROM `0x57` / Bus 0 / 1-byte offset；payload 读取返回值未检查；完整 `tlv_eeprom print` 原文未归档 | USER-OBSERVED（历史检索摘要）；具体原始输出待取回 | PASS（结构检查）；EVIDENCE MISSING（完整原文） | TEST-PLAN / P3-COMPLETION 有摘要；原始日志路径未确认 | 部分已提交；原始输出缺失 | 继续检索会话；若只能恢复摘要则标记 session-transcribed，不伪造原始日志 |
| P3-H013 | 2026-11-19 上午 | 0002 补丁远端/本地状态 | 用户历史提供：HEAD `679a926f...`；0002 文件缺失；源码 checkout 固定；`pa220_board.c` 有 122 行未提交修改；0001 已应用；GCC 4.7.0；旧 BIN 约 1.3M | USER-OBSERVED（历史检索摘要） | 状态冲突已识别；当时本地文件状态不能代表当前远端 | 当前远端 0002 文件已存在 | 已提交于当前 HEAD（需追查恢复提交） | 查找丢失/恢复过程和 Git 提交时间线；本地 122 行修改是否仍存在未知 |
| P3-H014 | 2026-11-19 上午 | 0002 hunk 损坏与修复 | 历史会话标题/用户消息显示曾出现“错误：补丁在第 65 行损坏”；后续用户明确要求禁止手工猜测/手算 hunk 行数，必须由 Git 生成 diff | USER-OBSERVED（历史会话索引/摘要） | FAIL（曾损坏）；最终修复状态需逐会话确认 | 当前 `0002-...patch` 已回读 | 已提交；最终 patch blob `f7427063c29cfa2fe39c17b9257d998ddc68b2f5` | 追查最终修复命令、patch-check 和构建结果；补丁正文已回读 |
| P3-H015 | 2026-11-19 上午 | 0002 board/clock 实机验证 | TEST-PLAN/最终报告声称 0002 构建并 RAM 启动；board type 20021、rev 1.1、serial 012801031163、MAC base 08:30:6b:e1:be:00、count 255；CORE 1000 MHz、SCLK 500 MHz、DDR 800 MHz/1600 MT/s | FILE-VERIFIED（报告正文）；用户原始串口记录待检索 | 报告声称 PASS；独立原始证据尚待回读 | TEST-PLAN 与报告引用 `reports/P3-BOARD-CLOCK-2026-11-19-serial.log` | 被引用日志在 `doc/pa220/reports/` 路径不存在；根目录 `reports/` 路径尚待核验 | 核对实际路径、原始日志存在性及内容；若只有会话文本则 session-transcribed |
| P3-H016 | 2026-11-19 09:41:36Z | TLV 状态修订 | 实际 Git commit `0a6f080a58c9a93e3d4950f8898fc46528d55618` 将 TEST-PLAN 中 TLV 从 NOT RUN 改为 PASS；diff 仅显示该一行修改 | FILE-VERIFIED（GitHub commit diff） | PASS（状态修订提交存在） | `doc/pa220/TEST-PLAN.md` | 已提交，当前分支 HEAD | 追查该提交之前/之后其他报告是否同步，及证据充分性 |
| P3-H017 | 2026-11-19 历史会话；约 09:34 UTC | P3/P4 阶段声明 | 助手曾声称 P3 COMPLETE、P4 IN PROGRESS；另声称 P4 首批源码审计完成 | ASSISTANT-CLAIM | 本次审计期间不采纳为最终完成判定 | TEST-PLAN / P3-COMPLETION / P4-PLAN | 已提交但可能阶段提前 | 只有历史审计完成后才重新判定 P3；本次不启动 P4 开发 |

## 首批发现的仓库维护疑点

1. `P3-READONLY-AUDIT-2026-11-19.md` 曾在第 9 节将串口日志引用为仓库根目录 `reports/P3-BOARD-CLOCK-2026-11-19-serial.log`，但 `TEST-PLAN.md` 引用 `reports/...`；本次对 `doc/pa220/reports/...` 的探测为 404，根目录路径尚未核验。
2. P3 报告对 TLV、SPI NOR、0002 实机测试给出 PASS，但原始用户串口证据仍需逐会话恢复并逐项核对。
3. P3-START 报告记录完整 SHA256/CRC，但构建原始日志、文件产物当前是否保留及其仓库索引尚未核实。
4. TEST-PLAN 当前写有 P3 COMPLETE 和 P4 IN PROGRESS；这属于此前阶段状态，不是本次历史审计已完成的证明。
5. 本地工作区状态未从 GitHub API 得到，不能声称 clean，也不能声称无用户未提交修改。

## 尚未覆盖的历史范围

- 目前只检索到历史上下文的若干索引/摘要，不等于已逐条遍历所有会话。
- 0001/0002 补丁修复、构建与 RAM 启动每次会话的完整用户输出尚未恢复。
- TLV EEPROM 的完整 `tlv_eeprom print`、`eeprom read` / `md.b`、SPI NOR `sf probe` 用户原始输出尚未逐字恢复。
- SCLK 语义确认、CORE/SCLK/DDR 单位换算、global data 与独立测量、board descriptor fallback、MAC 数量来源的讨论尚未逐条恢复。
- P3 中断会话的用户纠正、待归档命令和后续误排事项尚未完整梳理。
- 以上均保持 OPEN；不得据此宣布 P3 历史审计完成。
