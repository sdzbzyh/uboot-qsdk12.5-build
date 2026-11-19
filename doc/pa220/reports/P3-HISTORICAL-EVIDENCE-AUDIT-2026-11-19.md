# P3 历史证据审计账本（2026-11-19）

> 状态：**IN PROGRESS / INCOMPLETE**。本账本为首批落盘版本，不代表 P3 全历史会话已遍历。仅将可明确追溯的用户输出、已回读文件和助手声明分级记录；不得把报告中的转述升级为原始串口证据。

## 审计基线

- 集成仓库：`sdzbzyh/uboot-qsdk12.5-build`
- 分支：`feature/pa220-ubootkit-web`
- 审计开始时 HEAD：`0a6f080a58c9a93e3d4950f8898fc46528d55618`；首批账本/检查点提交后 HEAD 已更新（见检查点）
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
| P3-H012 | 2026-11-19 07:43–08:05Z 历史会话 | TLV EEPROM | 历史记录确认五个 tuple 边界：`0x00–0x24` len `0x24`、`0x24–0x34` len `0x10`、`0x34–0x50` len `0x1c`、`0x50–0x82` len `0x32`、`0x82–0x9a` len `0x18`；五个 checksum 被报告通过，END `0xffff`；EEPROM `0x57` / Bus 0 / 1-byte offset。完整 tuple type/len/csum/version/value 原始输出仍未恢复；源码审计另指出 payload `eeprom_read()` 返回值未检查 | USER-OBSERVED（tuple 结构/结果的历史检索摘要）；FILE-VERIFIED（源码审计文件） | 结构检查 PASS（摘要证据）；完整原文 EVIDENCE MISSING；payload read return handling 是源码风险 | TEST-PLAN / P3-COMPLETION 有摘要；`doc/pa220/test-logs/2026-11-19_P3_tlv_reader_001.txt` 实为源码摘录，不是设备输出 | 结果摘要已提交；原始 print/display 未归档 | 已另存末端 offset read 的 session-transcribed 文件；不得把源码摘录冒充原始设备日志 |
| P3-H013 | 2026-11-19 上午 | 0002 补丁远端/本地状态 | 用户历史提供：HEAD `679a926f...`；0002 文件缺失；源码 checkout 固定；`pa220_board.c` 有 122 行未提交修改；0001 已应用；GCC 4.7.0；旧 BIN 约 1.3M | USER-OBSERVED（历史检索摘要） | 状态冲突已识别；当时本地文件状态不能代表当前远端 | 当前远端 0002 文件已存在 | 已提交于当前 HEAD（需追查恢复提交） | 查找丢失/恢复过程和 Git 提交时间线；本地 122 行修改是否仍存在未知 |
| P3-H014 | 2026-11-19 上午 | 0002 hunk 损坏与修复 | 历史会话标题/用户消息显示曾出现“错误：补丁在第 65 行损坏”；后续用户明确要求禁止手工猜测/手算 hunk 行数，必须由 Git 生成 diff | USER-OBSERVED（历史会话索引/摘要） | FAIL（曾损坏）；最终修复状态需逐会话确认 | 当前 `0002-...patch` 已回读 | 已提交；最终 patch blob `f7427063c29cfa2fe39c17b9257d998ddc68b2f5` | 追查最终修复命令、patch-check 和构建结果；补丁正文已回读 |
| P3-H015 | 2026-11-19 上午 | 0002 board/clock 实机验证 | 用户历史检索摘要与报告记录 board type 20021、rev 1.1、serial 012801031163、MAC base 08:30:6b:e1:be:00、count 255；CORE 1000 MHz、SCLK 500 MHz、DDR 800 MHz/1600 MT/s | USER-OBSERVED（历史检索摘要）+ FILE-VERIFIED | PASS（实际原始日志已回读） | `reports/P3-BOARD-CLOCK-2026-11-19-serial.log` | 已提交；blob `1db8f7cbcfedcda5dfbd302308728504b6c76be1` | 根目录日志确实存在，正文与 TEST-PLAN/报告所述字段一致；此前误查 `doc/pa220/reports/` 的 404 不构成缺失 |
| P3-H016 | 2026-11-19 09:41:36Z | TLV 状态修订 | 实际 Git commit `0a6f080a58c9a93e3d4950f8898fc46528d55618` 将 TEST-PLAN 中 TLV 从 NOT RUN 改为 PASS；diff 仅显示该一行修改 | FILE-VERIFIED（GitHub commit diff） | PASS（状态修订提交存在） | `doc/pa220/TEST-PLAN.md` | 已提交，当前分支 HEAD | 追查该提交之前/之后其他报告是否同步，及证据充分性 |
| P3-H017 | 2026-11-19 历史会话；约 09:34 UTC | P3/P4 阶段声明 | 助手曾声称 P3 COMPLETE、P4 IN PROGRESS；另声称 P4 首批源码审计完成 | ASSISTANT-CLAIM | 本次审计期间不采纳为最终完成判定 | TEST-PLAN / P3-COMPLETION / P4-PLAN | 已提交但可能阶段提前 | 只有历史审计完成后才重新判定 P3；本次不启动 P4 开发 |
| P3-H018 | 2026-11-19 08:59:07Z | 0x81000000 Stage2/TFTP RAM 镜像验证 | 用户报告 TFTP 将 `u-boot-octeon_pa220.bin` 加载到 `0x81000000`，传输 1,362,280 bytes；`bootloadervalidate` 通过：header 192、data 1,362,088、header CRC `0xdcc5dc0c`、data CRC `0x0e351bbc`、link address `0xffffffffc0000000` | USER-OBSERVED（历史检索摘要，完整串口原文待恢复） | PASS（摘要证据）；启动/跳转未据此授权 | `doc/pa220/test-logs/2026-11-19_P3_stage2-tftp-validation.session-transcribed.txt` | 已补录为 session-transcribed；不是原始终端日志 | 原始完整串口输出仍 EVIDENCE MISSING；明确这只是加载/校验，不等于执行镜像 |
| P3-H019 | 2026-11-19 07:36:31Z | DRAM/bootmem 地址布局 | 用户提供 DRAM 8 GiB；`loadaddr`/`__tmp_load` base `0x20000000`、size `0x10000000`；`__uboot_code_data` base `0x20f000000`、size `0x01000000`；free ranges `0x0f100000` size `0x00ef55a0` 与 `0x30000000` size `0x1df000000`；meminfo 8 GiB | USER-OBSERVED（历史检索摘要）+ FILE-VERIFIED（P3-START 汇总） | PASS（所列区间核算）；完整地址域映射 UNVERIFIED | `P3-START-2026-11-19.md` | 已提交 | 继续核对原始 namedprint/freeprint 输出与后续 alias 语义会话 |
| P3-H020 | 2026-11-19 07:55:44Z | EEPROM 终止偏移读取 | 用户原文：`eeprom read 0x31000000 0x9a 8`；设备回显 `EEPROM @0x57 read: addr 31000000 off 009a count 8 ... done`；`md.b 0x31000000 8` 输出 `31000000: ff ff ff ff ff ff ff ff ........` | USER-OBSERVED（历史检索恢复的逐字输出） | PASS（偏移处读取为 8×0xff）；不证明物理容量 | 应补入 `doc/pa220/test-logs/` session-transcribed 记录 | 目前该完整用户输出仅在历史会话可恢复；尚未归档为独立日志 | 后续创建标明 `session-transcribed` 的记录并注明来源会话时间 |
| P3-H021 | 2026-11-19 09:27:09Z 前后 | SPI NOR probe | 现有历史结果为助手转述：`SF: Detected W25Q128BV ... erase size 4 KiB total 16 MiB`；检索未找到用户原始 `sf probe 0:0` 串口输出 | ASSISTANT-CLAIM（未恢复独立用户输出） | UNVERIFIED（原始证据缺失） | TEST-PLAN/P3-COMPLETION 声称 PASS | 报告有结论；原始日志未发现 | 继续追踪历史会话；若无用户输出，保留 EVIDENCE MISSING，不得伪造原始日志 |
| P3-H022 | 2026-11-19 08:44:57Z–08:48:45Z | SCLK 语义及 0002 补丁生成 | 用户要求 GCC 4.7.0；仅新增 `help pa220`/`pa220 board`/`pa220 clock`；明确禁止手工修改/猜测 unified diff hunk 行数，要求独立副本修改、由 `git diff` 自动生成并 `git apply --check`；用户随后报告 0002 应用成功、外层有未跟踪 patch 与 `.before-hunk-fix` | USER-OBSERVED（历史检索摘要） | 0002 应用成功（后续构建另核）；SCLK 语义需结合源码证据 | `patches/pa220/0002-pa220-board-clock-readonly-diagnostics.patch` | 已提交，blob `f7427063c29cfa2fe39c17b9257d998ddc68b2f5` | 将此用户约束作为补丁审计永久规则；不得手工算 hunk |
| P3-H023 | 2026-11-19 08:44:59Z 前后 | SCLK 初始化来源 | 助手声称 `gd->bus_clk` 在 `__octeon_setup_clocks()` 初始化；`CVMX_CLOCK_SCLK` 返回 Hz；需继续核对源文件调用链 | ASSISTANT-CLAIM（待固定源码逐行回读） | UNVERIFIED | `P3-READONLY-AUDIT-2026-11-19.md` 有 API 语义结论 | 已提交 | 与固定源码核对初始化顺序、单位和 gd 字段来源，不仅依赖助手声明 |

## 首批发现的仓库维护疑点

1. `P3-READONLY-AUDIT-2026-11-19.md` 与 `TEST-PLAN.md` 引用的根目录 `reports/P3-BOARD-CLOCK-2026-11-19-serial.log` 已实际找到并回读；此前仅对 `doc/pa220/reports/` 的错误路径探测返回 404。该项不再是缺失。
2. board/clock 原始串口日志已存在；TLV `eeprom read` 末端 8 字节原始输出可从用户历史消息逐字恢复，但完整 `tlv_eeprom print/display` 输出仍缺失；SPI `sf probe` 仍只有助手转述。
3. P3-START 报告记录完整 SHA256/CRC，但构建原始日志、ELF/MAP/BIN 产物是否已归档及其索引尚未核实。
4. TEST-PLAN 当前写有 P3 COMPLETE 和 P4 IN PROGRESS；这属于此前阶段状态，不是本次历史审计已完成的证明。
5. 本地工作区状态未从 GitHub API 得到，不能声称 clean，也不能声称无用户未提交修改。

## 尚未覆盖的历史范围

- 目前只检索到历史上下文的若干索引/摘要，不等于已逐条遍历所有会话。
- 0001/0002 补丁修复、构建与 RAM 启动每次会话的完整用户输出尚未恢复。
- TLV EEPROM 的完整 `tlv_eeprom print`、`eeprom read` / `md.b`、SPI NOR `sf probe` 用户原始输出尚未逐字恢复。
- SCLK 语义确认、CORE/SCLK/DDR 单位换算、global data 与独立测量、board descriptor fallback、MAC 数量来源的讨论尚未逐条恢复。
- P3 中断会话的用户纠正、待归档命令和后续误排事项尚未完整梳理。
- 以上均保持 OPEN；不得据此宣布 P3 历史审计完成。


| P3-H024 | 2026-11-19 06:58–07:18Z | 0001 补丁修复历史 | GitHub commit 链实际显示：`794b6d8` 新增只读 named-load-block 命令；`2118ad4` 修正 hunk header；`3e5251f` 处理 include hunk offset；`74eddb` 调整稳定上下文；`f3a144e` 调整插入 offset；`6db6a25` 扩展上下文以便 git apply；`679a926` 修正 hunk old/new 行数（从 7/128 改为 6/127）。 | FILE-VERIFIED（GitHub commit diff） | 中间版本存在 patch apply/hunk 问题；最终版本通过状态需与用户实际命令记录核对 | `patches/pa220/0001-pa220-read-only-maintenance-command.patch` | 全部变更已在历史提交中；当前 blob 已回读 | 保留修复时间线；不得用“初版一次通过”概括 |
| P3-H025 | 2026-11-19 07:52:31Z | `P3_tlv_reader_001.txt` 文件性质 | GitHub commit `5a7e78b` 添加该文件；回读内容为 TLV reader、print/display path、command dispatch、EEPROM address-length 等源码摘录（816 行），不是 U-Boot 设备串口输出 | FILE-VERIFIED（GitHub commit diff/文件正文） | NOT A RAW TEST LOG | `doc/pa220/test-logs/2026-11-19_P3_tlv_reader_001.txt` | 已提交 | 保留原文件避免破坏历史；本账本明确标注其实际性质，避免引用为原始设备证据 |
| P3-H026 | 2026-11-19 07:40–07:52Z | P3 初始 CLI/bootmem 记录 | GitHub commits `4d71f98`、`e9ce0b8`、`33cfdf1` 更新 P3-START、ROADMAP 检查点和 TEST-PLAN；P3-START 正文包含测试摘要、地址区间和产物哈希，但没有完整的 namedprint/freeprint/CLI 原始终端捕获文件 | FILE-VERIFIED（提交 diff/文件正文） | 报告汇总存在；原始完整输出是否仅在历史会话中尚待核对 | `doc/pa220/reports/P3-START-2026-11-19.md`、`doc/pa220/TEST-PLAN.md` | 已提交 | 后续从历史会话提取可恢复的用户原文；缺失部分标 EVIDENCE MISSING |
| P3-H027 | 2026-11-19 08:10–08:12Z | 0002 board/clock 补丁修复历史 | GitHub commit 链：`32e4dbe` 添加 0002；`aa718c6` 调整多个 hunk 行数；`5be99f3` 规范化 unified diff 上下文与行数；`cf5a927` 修正最终 help hunk 上下文。用户历史要求禁止手工猜测/手算 hunk 行数，必须独立副本修改、由 `git diff` 生成并 patch-check。 | FILE-VERIFIED（GitHub commit diff）+ USER-OBSERVED（历史用户约束摘要） | 修复链已确认；是否所有修改由 git diff 自动生成仍需对照会话原始命令 | `patches/pa220/0002-pa220-board-clock-readonly-diagnostics.patch` | 当前最终 blob 已提交并回读 | 将用户约束列为强制审计规则；不能把中间 hunk 修订说成自动正确 |
| P3-H028 | 2026-11-19 09:12:46Z | board/clock 原始串口日志 | GitHub commit `8873182` 新增 24 行串口日志；本次回读确认文件包含完整 `help pa220`、`pa220 board`、`pa220 clock` 输出 | FILE-VERIFIED（原始日志正文） | PASS（该三命令日志范围内） | `reports/P3-BOARD-CLOCK-2026-11-19-serial.log` | 已提交，blob `1db8f7cbcfedcda5dfbd302308728504b6c76be1` | 已核对路径和正文；此项原先误判为缺失已纠正 |
| P3-H029 | 2026-11-19 09:12–09:41Z | P3 状态收敛/完成声明 | 提交 `e6f4087` 将 board/clock 改为 PASS；`87bb79b` 补入原始日志引用；`a12f0b6` 将 P3 标记 COMPLETE 并开 P4；`85751cf` 建立 P3-COMPLETION；`0a6f080` 将 TLV 状态改为 PASS。 | FILE-VERIFIED（GitHub commit history/diff） | 历史状态变化可追溯；是否有充分原始证据支撑所有 PASS 仍在审计 | TEST-PLAN、ROADMAP、P3-COMPLETION | 已提交 | 本次审计完成前不接受历史 COMPLETE 声明为当前审计结论 |
| P3-H030 | 2026-11-19 09:27Z 前后 | SPI NOR probe 证据 | P3 完成报告/TEST-PLAN 记载 `sf probe` 识别 W25Q128BV、16 MiB、4 KiB erase；历史检索仍未恢复用户粘贴的完整原始命令输出。 | ASSISTANT-CLAIM（报告/历史助手转述）；FILE-VERIFIED（报告内容） | EVIDENCE MISSING（原始设备输出）；不重复 probe | `doc/pa220/TEST-PLAN.md`、`doc/pa220/reports/P3-COMPLETION-2026-11-19.md` | PASS 摘要已提交，原始日志未找到 | 报告应注明 PASS 仅基于历史摘要，除非后续找到用户原始输出；不得伪造串口日志 |
