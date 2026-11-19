# PA-220 P3 阶段完成报告（2026-11-19）

- 阶段：P3 CLI / 只读诊断完善
- 最终状态：**COMPLETE**
- 集成分支：`feature/pa220-ubootkit-web`
- 本报告创建时已核对基线 HEAD：`4675e6948a9fdd532adcdde036baf1597936c10a`
- PA-220 固定源码：`IronDemiGod/pa220-openwrt@801717ba6a8a59ed2cfd53bf3e1b19841fecd2e8`
- uBootKit 固定审计源码：`chenxin527/uboot-qsdk12.5-build@9ce315735ac88fd0a7c7697bc527a8616ed0a972`

## 1. 阶段结论

P3 的 CLI/只读诊断实现范围已完成：维护命令补丁完成构建和 RAM 启动验证，新增 board/clock 命令通过实机检查；此前分批完成的内存/bootmem、TLV EEPROM 与 SPI NOR 只读检查均有记录。**P3 正式关闭为 COMPLETE，并将项目阶段推进至 P4。**

COMPLETE 不表示每个可选硬件诊断均已验证，也不关闭 P1 风险。未完成/不安全的项目继续明确标记，不能因阶段收尾改写为 PASS。

## 2. 已完成项目与证据

| 项目 | 结果 | 实测/证据 |
|---|---|---|
| 0001 只读维护命令 | PASS | 补丁应用、构建、最终 ELF 链接；P3 初始构建/产物证据见 `P3-START-2026-11-19.md` |
| 0002 board/clock 命令 | PASS | 已应用、构建并通过原厂 Stage2/TFTP RAM 启动 |
| RAM 启动 | PASS | 0002 镜像进入 U-Boot CLI；本次收尾不重复构建或启动 |
| `help pa220` | PASS | 列出 status/env/memory/board/clock |
| `pa220 status` | PASS | 板型、DRAM、CPU clock、Ethernet 配置、CPLD/PHY/LED 只读状态输出 |
| `pa220 env` | PASS | 显示 SPI NOR 环境后端和布局；未执行 saveenv |
| `pa220 memory` | PASS | loadaddr 与 `__tmp_load` base 为 0x20000000，size 为 0x10000000 |
| `pa220 board` | PASS | type 20021 (PA220)、revision 1.1、serial 012801031163、MAC base 08:30:6b:e1:be:00、count 255 |
| `pa220 clock` | PASS | CORE 1000 MHz、SCLK 500 MHz、DDR 800 MHz base / 1600 MT/s |
| `bdinfo` | PASS | DRAM 8 GiB；memstart/address-domain 解释仍作为风险记录 |
| `printenv loadaddr` | PASS | 运行时 loadaddr 已读取 |
| `namedprint` / `freeprint` | PASS | 13 个 named blocks、2 个 free blocks；关键半开区间边界核算无重叠 |
| `meminfo` | PASS | DRAM 8 GiB |
| TLV EEPROM 只读检查 | PASS | 五个 tuple、checksum、END `0xffff`、终止偏移 `0x9a` 已核查；未写 EEPROM |
| SPI NOR 识别 | PASS | `sf probe` 识别 W25Q128BV、16 MiB、4 KiB erase granularity；未执行写/擦除 |

board/clock 原始串口记录：`reports/P3-BOARD-CLOCK-2026-11-19-serial.log`。
board/clock 实机汇总：`reports/P3-READONLY-AUDIT-2026-11-19.md`。
P3 初始构建、0001 与 RAM/bootmem 证据：`doc/pa220/reports/P3-START-2026-11-19.md`。

## 3. 未验证项与持续风险

| 项目 | 状态 | 处理 |
|---|---|---|
| MAC descriptor EEPROM/fallback 实际来源 | NOT VERIFIED | 输出只显示可能来源，不能断言来自 EEPROM |
| CORE/SCLK/DDR 独立频率测量 | NOT RUN | 当前显示值来自初始化 global-data/configuration，不是独立测量 |
| Ethernet 物理 link/carrier | NOT VERIFIED | `ethact` 只是当前设备选择；通用 link API 存在 LED/SFP 等副作用 |
| eMMC 容量/分区/文件系统运行时查询 | BLOCKED | `mmc_init/startup` 可能发 MMC CMD6 修改 EXT_CSD 配置字段；禁止以只读名义触发 |
| USB Mass Storage 枚举 | NOT VERIFIED | USB 电源 GPIO 默认拉低；初始化副作用尚未审完 |
| 任意地址 RAM 接收 | BLOCKED | 统一 `octeon_load_range_check(uint64_t addr, uint64_t len)` 未实现 |

P1 已记录的 cached/uncached/physical alias、RAM 边界、整数溢出/地址回绕、LOAD MemSiz 与 `uboot_end` 4-byte 差异、RAM 镜像 CRC 变化等问题继续保留。P3 COMPLETE 不表示这些问题已解决。

## 4. 安全确认

P3 收尾没有执行以下操作：
- `saveenv`；
- EEPROM/TLV 写入；
- CPLD 写入；
- SPI NOR/eMMC/NAND/USB block 写入或擦除；
- `bootloaderupdate`；
- `namedalloc`、`namedfree`、`mtest`；
- 任意地址 TFTP/HTTP/ELF RAM 接收。

## 5. P4 阶段交接

P4 主题：**网络维护基础功能：DHCP Client、TFTP、Ping**。

首批源码审计已确认：
- DHCP Client 和 Ping 在 PA-220 固定源码中已有原生实现，优先复用和验证，不直接移植另一代 U-Boot 的实现。
- TFTP GET/Server 接收会在 `store_block()` 中直接写 `load_addr + offset`；统一 RAM validator 未完成前不得启用接收。
- uBootKit 的 IPQ/FDT/TrustZone 地址范围不能直接用于 PA-220。
- P4 当前只完成源码审计与计划，尚未进行 P4 构建或网口实机验证。

P4 计划：`doc/pa220/P4-PLAN.md`。
P4 首批源码审计：`doc/pa220/reports/P4-NETWORK-BASELINE-AUDIT-2026-11-19.md`。

下一步先核实最终构建配置及 MGT 网络主循环，再在确认 `autoload=n` 的条件下验证 DHCP-only 和 Ping；不运行 `tftpboot`、`tftpsrv` 或 `tftpput`，直至 validator 完成并审查。


## 6. 后续历史证据审计覆盖说明（2026-11-19）

本报告记录的是当日 P3 CLI/只读诊断阶段的历史完成判定，不替代 2026-11-19 用户随后提出的 P3 全历史证据审计。后续历史会话补充追溯属于证据补档，不改变 P3 CLI/只读诊断阶段的完成判定。未恢复的原始材料必须明确标记 EVIDENCE GAP，不得推断或补造。

审计已发现并正在核对：TLV 完整原始 print/display 输出缺失；`test-logs/2026-11-19_P3_tlv_reader_001.txt` 实为源码摘录；SPI `sf probe` 原始用户输出尚未恢复；构建原始日志/产物归档仍需核对；0001/0002 hunk 修复时间线需要逐会话追溯。board/clock 串口日志已确认存在于仓库根目录 `reports/`，并非缺失。详见 `P3-HISTORICAL-EVIDENCE-AUDIT-2026-11-19.md` 与 `P3-HISTORICAL-AUDIT-CHECKPOINT.md`。

上述证据补档不构成 P3 阶段阻断。P3 CLI/只读诊断阶段按本报告第 1～5 节和 TEST-PLAN 第 7 节正式保持 **COMPLETE**；P4 按 ROADMAP 与 P4-PLAN 正式 **IN PROGRESS**。未恢复的原始材料继续列为证据缺口，不因 P3 COMPLETE 而改写为 PASS。


## 7. SPI NOR 证据来源补充

本报告中的 W25Q128BV / 16 MiB / 4 KiB erase 识别结果，来源于用户在 2026-11-18 10:10:39Z 提供的先前 `sf probe` 输出，早于 P3 首次进入时间（2026-11-19）。因此它是 P3 收尾时引用的继承只读证据，不是 P3 当日新执行的测试。P3 源码审计报告所记 `NOT RUN` 指该源码审计批次未再次执行 probe，与先前观察不矛盾。逐字可恢复的一行输出见 `../test-logs/2026-11-18_SPI-NOR-probe.session-transcribed.txt`。本次不重复 probe。


## 8. DDR clock explanatory-note correction（2026-11-19）

本报告引用的原始 board/clock 输出仍保留原文，但其统一 NOTE 将 CORE/SCLK/DDR 一概描述为非独立测量值，后来固定源码复核发现这对 DDR 不准确：CORE/SCLK 来自 reset CSR multiplier；DDR 的 `gd->mem_clk` 在 DRAM 初始化后可能由 CN70XX 的 IPD/DCLK counter ratio 动态测量。0002 patch 中的 DDR source 行也没有体现该后续测量路径。该解释性缺陷已登记为 P3 未解决项，详见 `P3-HISTORICAL-EVIDENCE-AUDIT-2026-11-19.md` 的 P3-H050 和 `P3-READONLY-AUDIT-2026-11-19.md` 第 12 节。原始日志不改写，已验证补丁不在本次审计中擅自修改或重建。


## 9. Board descriptor / MAC 来源交叉证据补充（2026-11-19）

此前“MAC descriptor 来源未单独证明”是历史收尾时的保守状态。本次历史审计恢复了用户 TLV 输出：BOARD_DESC type 0x0002 的 board type 0x4e35、revision 1.1、serial 012801031163；MAC_ADDR type 0x0004 的 base 08:30:6b:e1:be:00、count 255。它们与原始 board/clock 串口日志的运行时字段逐项一致。固定源码也确认 board descriptor tuple 命中后复制到 global data 并清除 missing flag；MAC tuple 命中后复制 MAC descriptor，只有缺失时才生成随机 MAC。因此本板本次运行的 board descriptor/MAC 来源可由 EEPROM tuple 与运行时输出交叉支持。通用 fallback 路径仍存在。详见历史账本 P3-H051。
