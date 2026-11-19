# PA-220 U-BootKit 测试计划

## 1. 测试阶段

### A. 静态审计

- 核对 PA-220 固定源码 commit 和集成补丁。
- 检查配置项、命令注册、驱动依赖、链接地址、镜像头和内存布局。
- 检查补丁能否在干净源码树应用、重复执行是否幂等。
- 对所有新增命令审查副作用：环境写入、SPI/eMMC 写入、CPLD 写入、复位、看门狗和网络状态修改。

### B. 构建验证

建议命令：

```sh
./build-pa220.sh /path/to/tools-gcc-4.7 2>&1 | tee pa220-build.log
```

记录：
- 集成仓库 commit 与分支。
- PA-220 源码 commit。
- OCTEON 工具链版本与路径。
- 补丁列表及应用结果。
- 完整构建日志、退出码、产物大小和 SHA-256。
- 镜像头的 board type、image type、长度和 CRC。

### C. RAM 启动验证

- 通过原厂 U-Boot TFTP 加载镜像到已确认的 RAM 测试地址。
- 按设备安装资料执行必要的 I-cache 操作和跳转。
- 确认 pa-220# 提示符、串口速率、DRAM 信息和 autoboot 行为。
- 记录串口完整日志。
- 不执行 saveenv，不执行 sf erase/write/update，不写 CPLD。

### D. 外设只读验证

逐项测试：
- MGT Ethernet 链路、DHCP/TFTP/Ping。
- eMMC 容量、分区表、只读文件访问。
- SPI NOR JEDEC ID、容量和只读内容读取。
- CPLD 只读寄存器、板载 EEPROM/MAC。
- USB 控制器枚举与 Mass Storage。
- 温度/LED 状态读取。

任何探测命令都必须先检查是否有写寄存器或清状态副作用。

### E. 服务与异常路径验证

- Web 服务启动/退出后 CLI 和 autoboot 能恢复。
- 错误请求、超长请求、断开连接、超时和重复连接不导致崩溃。
- 文件上传只写 RAM 缓冲区；长度上限、CRC/SHA 错误和空间不足均有安全失败路径。
- Web 初期没有持久化写操作入口。

### F. 存储写入验证（后续独立授权）

- 只在专门的可恢复测试阶段进行。
- 写前展示设备、分区、偏移、长度和备份状态。
- 对写入范围做整数溢出和边界检查。
- 写后读回并校验。
- 预先演练恢复流程。
- SPI NOR 和 CPLD flash-select 绝不混入普通固件安装流程。

## 2. 结果状态

每个测试用例只能使用以下结果：
- NOT RUN：未执行。
- PASS：按记录步骤执行且结果符合预期。
- FAIL：结果不符合预期。
- BLOCKED：因缺少硬件、工具链或资料无法执行。
- NOT APPLICABLE：该平台无对应硬件/功能。

## 3. 证据要求

每项 PASS 必须保留：
- 测试设备/硬件修订信息（可识别范围内）。
- 软件 commit、镜像 SHA-256 和工具链版本。
- 操作命令与原始串口/终端输出。
- 测试前提、预期结果、实际结果。
- 是否触及持久化存储。

无原始结果的口头判断不得作为实机通过证据。

## 4. P2 实际结果（2026-11-19）

详细证据：`reports/P2-COMPLETION-2026-11-19.md`。

| 测试项 | 结果 | 说明 |
|---|---|---|
| 固定输入与工具链 | PASS | 固定源码/补丁/工具链已核对；最小 OCTEON 目标编译与链接通过 |
| 干净构建 | PASS | 固定源码全新 checkout 构建退出码 0 |
| ELF/BIN/map 产物 | PASS | 文件生成；大小、头信息、关键符号和 SHA256 已记录 |
| ELF loader / Cache / named-block 编入 | PASS | 对象文件、linker map 和最终 ELF 符号表均有直接证据 |
| 新版 Stage3 RAM 启动 | PASS | 原厂 Stage2/TFTP 加载后成功进入 U-Boot 命令行 |
| 旧版 RAM 启动对照 | PASS | 旧版 U-Boot 同样可从 RAM 启动 |
| `__tmp_load` 运行时 descriptor | NOT RUN | 转入 P3 只读诊断；未确认前不得开放任意地址上传 |
| `pa220 status/env` 设备运行结果 | NOT RUN | 转入 P3 只读诊断 |
| TLV EEPROM/I²C 只读窗口 | NOT RUN | 转入 P3；禁止任何写操作 |
| RAM 镜像启动后 CRC 变化 | OBSERVATION | 新旧 U-Boot 均观察到变化；不是失败判据，原因未定，暂不追查 |
| LOAD MemSiz 与 `uboot_end` 4-byte 差异 | DEFERRED | 非阻断性链接布局疑点，后续统一处理 |

注：`OBSERVATION` 与 `DEFERRED` 是阶段问题记录，不替代第 2 节规定的测试结果状态。P2 已收尾；P3 继续补齐只读运行时诊断。任何持久化介质写入仍需独立阶段审查和明确授权。

## 5. P3 实际结果（2026-11-19）

详细原始输出、构建产物和地址区间核算见 `reports/P3-START-2026-11-19.md`。

| 测试项 | 结果 | 说明 |
|---|---|---|
| P3 补丁应用 | PASS | 固定 PA-220 commit 上 `git apply --check` 成功；两个 hunk 行数已核验 |
| P3 构建 | PASS | OCTEON SDK GCC 4.7.0 构建成功，镜像头 CRC/长度及 ELF/MAP/BIN SHA256 已记录 |
| P3 ELF/MAP | PASS | 新命令已进入最终 ELF；入口和 LOAD 起点保持 0xc0000000 |
| P3 Stage3 RAM 启动 | PASS | 原厂 Stage2/TFTP 加载 P3 镜像后成功进入 U-Boot CLI |
| `help pa220` | PASS | 正常列出 status/env/memory |
| `pa220 status` | PASS | 板型、DRAM、CPU clock、Ethernet、CPLD/PHY/LED 只读状态正常显示 |
| `pa220 env` | PASS | SPI NOR 环境后端和布局信息正常显示；未执行 saveenv |
| `pa220 memory` | PASS | loadaddr 与 `__tmp_load` base 均为 0x20000000；size 为 0x10000000 |
| `bdinfo` / `meminfo` | PASS | DRAM 容量均报告 8 GiB；memstart/address-domain 关系仍待解释 |
| `namedprint` / `freeprint` | PASS | 13 个 named blocks、两个 free blocks；关键相邻边界经半开区间核算一致 |
| TLV EEPROM/I2C 只读内容 | PASS | 五个 tuple、checksum、END `0xffff`、终止偏移 `0x9a` 已完成只读核查；无写入 |
| 网络/存储扩展只读诊断 | NOT RUN | P3 后续工作 |
| 任意地址 RAM 上传安全性 | BLOCKED | cached/uncached/physical alias 与统一 pre-write validator 尚未完成；不得开放上传 |

本轮没有执行任何持久化介质写入、named block 分配/释放或 DRAM 测试。（历史检查点）当时 P3 仍进行中；最终收尾以第 7 节为准。

## 6. P3 只读诊断源码审计（2026-11-19）

详细记录：`reports/P3-READONLY-AUDIT-2026-11-19.md`。

| 测试/审计项 | 结果 | 说明 |
|---|---|---|
| 板级 descriptor/MAC 缓存 API | PASS（源码审计） | 可读 global data；无需重新访问 EEPROM |
| 板级初始化副作用边界 | PASS（源码审计） | CPLD LED、USB GPIO、QLM 等初始化写路径已识别 |
| CORE/IO/DDR 时钟 API | PASS（源码映射）；DDR 注释 FAIL | CORE/SCLK 由 CSR multiplier 派生；DDR gd->mem_clk 在 DRAM init 后可能由 IPD/DCLK counter ratio 测量；无独立外部频率计测试 |
| 网络环境配置 | PASS（既有实机结果） | ethact/ethprime/IP 配置已显示 |
| 网络物理 link 状态 | NOT VERIFIED | cvmx_helper_link_get() 会更新 link LED 并检查 SFP，不作为纯只读查询 |
| SPI NOR probe | NOT RUN | Winbond W25Q128 配置下 sf probe 仅列为后续只读候选 |
| eMMC 初始化/信息查询 | BLOCKED | mmc_init/startup 可能通过 CMD6 修改 EXT_CSD 配置字段 |
| USB Mass Storage 枚举 | NOT RUN | USB 电源 GPIO 默认拉低；usb start 初始化副作用尚未审完 |
| 新增 board/clock 命令 | PASS | 0002 已构建并 RAM 启动；`help pa220`、`pa220 board`、`pa220 clock` 实机验证通过，详见 `reports/P3-READONLY-AUDIT-2026-11-19.md` |
| P3 整体 | **IN PROGRESS（历史证据审计）** | CLI/只读诊断历史阶段曾标记完成；全历史证据追溯、构建原件归档及 DDR clock note 缺陷仍未闭环 |

源码审计与实机验证分阶段完成。0002 新增命令已在 RAM 启动镜像上验证；本轮没有访问或修改持久化介质。


### 6.1 P3 第二个命令增量实机验证结果

补丁：`patches/pa220/0002-pa220-board-clock-readonly-diagnostics.patch`。

- **PASS**：0002 已应用、构建并通过原厂 Stage2/TFTP RAM 启动；本轮直接在已启动镜像中验证，没有重新构建或重新启动。
- **PASS**：`help pa220` 正常列出 status/env/memory/board/clock 五个子命令。
- **PASS**：`pa220 board` 正常显示 Board type 20021 (PA220)、revision 1.1、serial 012801031163、MAC base 08:30:6b:e1:be:00、MAC count 255；board descriptor 显示 from initialized descriptor。
- **NOT RUN**：MAC descriptor 的 EEPROM/fallback 实际来源未单独标识；输出只说明可能来自 EEPROM 或 generated fallback，不能据此断言来源。
- **PASS**：`pa220 clock` 显示 CORE 1000 MHz、SCLK 500 MHz、DDR 800 MHz base / 1600 MT/s；单位换算正确。
- **PASS**：时钟输出明确限定为 initialized global-data/configuration values，而非 independent measurements。
- **PASS（观察范围内）**：三个命令均正常返回 CLI，输出格式正常，未观察到异常；这不构成全面内存安全证明。
- 完整原始串口日志：`reports/P3-BOARD-CLOCK-2026-11-19-serial.log`。
- 实机验证报告：`reports/P3-READONLY-AUDIT-2026-11-19.md`。
- 本轮未执行持久化介质写入。任意地址 RAM 上传安全性仍为 **BLOCKED**；本轮命令通过不构成上传安全性证明。
- 不重复既有 status/env/memory/bootmem/TLV 测试。



## 7. P3 最终收尾与阶段交接（2026-11-19）

**历史阶段状态：P3 CLI/只读诊断曾标记 COMPLETE。当前全历史证据审计：IN PROGRESS。** `reports/P3-COMPLETION-2026-11-19.md` 保留当日阶段决策，但不得覆盖当前审计任务；构建产物缺失和 P3-H050 clock-note FAIL 尚未解决。

已完成并有证据的项目：
- 0001/0002 只读维护补丁已应用、构建；0002 镜像已由原厂 Stage2/TFTP RAM 启动。
- `help pa220`、`pa220 status/env/memory/board/clock` 实机验证通过。
- `bdinfo`、`printenv loadaddr`、`namedprint`、`freeprint`、`meminfo` 已完成。
- TLV EEPROM 五个 tuple、checksum、END `0xffff`、终止偏移 `0x9a` 已只读核查；没有 EEPROM/TLV 写入。
- SPI NOR `sf probe` 已识别 W25Q128BV、16 MiB、4 KiB erase granularity；没有执行写入/擦除。
- board/clock 原始串口证据：`reports/P3-BOARD-CLOCK-2026-11-19-serial.log`；实机结果报告：`reports/P3-READONLY-AUDIT-2026-11-19.md`。

保留为未验证/阻断的事项：
- MAC descriptor 的 EEPROM/fallback 来源没有单独证明。
- CORE/SCLK 是 reset CSR multiplier 派生值；DDR 的 gd->mem_clk 在 DRAM init 后可能由 CN70XX IPD/DCLK counter ratio 动态测量。0002 patch 统一 NOTE 不准确，见 P3-H050。
- Ethernet 物理 link 状态 NOT VERIFIED；不将 ethact 当作 carrier 状态。
- eMMC 初始化/信息查询仍 BLOCKED；USB 枚举 NOT VERIFIED。
- 统一 RAM 写入 validator `octeon_load_range_check(uint64_t addr, uint64_t len)` 尚未实现；cached/uncached/physical alias、RAM 边界、整数溢出/地址回绕仍未收敛。TFTP/HTTP/ELF 接收安全性保持 BLOCKED。
- P1 中 LOAD MemSiz 与 `uboot_end` 4-byte 差异、RAM 镜像 CRC 变化等非阻断风险继续保留，P3 完成不代表这些风险已解决。

本阶段没有执行 `saveenv`、EEPROM/TLV 写入、CPLD 写入、SPI/eMMC/NAND/USB block 写入/擦除、`bootloaderupdate`、`namedalloc`、`namedfree` 或 `mtest`。

## 8. P4 网络维护基础：首批源码审计（2026-11-19）

**P4 状态：PAUSED（等待 P3 历史证据审计完成）。** 固定输入仍为 PA-220 `801717ba6a8a59ed2cfd53bf3e1b19841fecd2e8` 与 uBootKit `9ce315735ac88fd0a7c7697bc527a8616ed0a972`。

首批审计结果：
- DHCP Client：PA-220 已有 `CONFIG_CMD_DHCP`、`DhcpRequest/DhcpHandler` 与 `dhcp` 命令，不移植 uBootKit 的 bootp.c；优先验证既有实现。
- Ping：PA-220 已有 `CONFIG_CMD_PING`、`ping.o` 和 `ping` 命令，不移植 uBootKit 的 ping.c；优先验证既有实现。
- TFTP GET/PUT/Server：PA-220 已有旧版实现，但 GET/Server 的接收路径会在 `store_block()` 中直接 memcpy 到 `load_addr + offset`，没有 PA-220 的统一 RAM 范围 validator。GET 与 Server 接收功能均不得用于未经验证的 RAM 写入。
- uBootKit 新版 TFTP 的 IPQ/FDT/TrustZone 边界检查依赖 IPQ 平台常量，不可复制到 PA-220；必须先实现 PA-220 地址域规范化和 named-block 权威边界校验。
- P4 首批源码审计与任务拆分见 `reports/P4-NETWORK-BASELINE-AUDIT-2026-11-19.md` 和 `P4-PLAN.md`。本节不代表 DHCP/Ping 已实机验证。


## 9. P3 历史证据审计覆盖状态（2026-11-19）

**历史审计状态：IN PROGRESS。** 本节是对既有 P3 COMPLETE 阶段记录的审计覆盖说明，不删除或改写当日历史记录。此前 P3 CLI/只读诊断 COMPLETE 是历史阶段决策；本次用户要求重新追溯 P3 全部历史会话、原始证据和 GitHub 归档，故在本次审计满足全部完成条件之前，不得将“P3 历史证据审计”或当前项目状态标记为 COMPLETE，也不得开展 P4 实际开发。

当前已发现的证据限制/补录项：

- board/clock 原始串口日志存在于仓库根目录 `reports/P3-BOARD-CLOCK-2026-11-19-serial.log`，已回读；不得误报为缺失。
- `doc/pa220/test-logs/2026-11-19_P3_tlv_reader_001.txt` 实际是 TLV 源码摘录，不是设备串口输出；不能作为 TLV 实机原始日志引用。
- EEPROM offset `0x9a` 的 `eeprom read` / `md.b` 用户原始输出已从历史消息恢复，并以 `session-transcribed` 文件补录；完整 `tlv_eeprom print/display` 的 tuple 原始全文仍为 EVIDENCE MISSING。
- 0x81000000 Stage2/TFTP 镜像验证目前仅以 session-transcribed 摘要补录；完整原始传输/校验输出仍待恢复。
- SPI NOR `sf probe` 的一行用户原始结果已恢复，但来源是 2026-11-18（P3 开始前）的继承证据：W25Q128BV、page 256 Bytes、erase 4 KiB、total 16 MiB；已另存 session-transcribed 记录，不代表 P3 当日重新执行。
- 构建原始日志、ELF/MAP/BIN 产物文件是否归档仍待逐项核验；报告中的 SHA256/CRC 不自动等同于原始产物已归档。

逐项账本：`reports/P3-HISTORICAL-EVIDENCE-AUDIT-2026-11-19.md`。
持续检查点：`reports/P3-HISTORICAL-AUDIT-CHECKPOINT.md`。

本节状态优先于此前 P3 完成声明，直到历史审计最终报告明确解除。P4-PLAN 和已有 P4 源码审计仅作为历史文档保留；P4 实际开发暂停。


补充证据索引：
- CLI/bootmem 历史摘要：`test-logs/2026-11-19_P3_cli-bootmem.session-transcribed.txt`。
- 0002 构建产物历史摘要：`test-logs/2026-11-19_P3_0002-build-artifacts.session-transcribed.txt`。
- TLV 结构历史摘要：`test-logs/2026-11-19_P3_tlv-structure.session-transcribed.txt`。
- SPI NOR 继承证据（2026-11-18）：`test-logs/2026-11-18_SPI-NOR-probe.session-transcribed.txt`。


- P3-START 的 bootmem 表格已由历史审计纠正：用户 namedprint 给出的 __boot_vector1__ base 0x0fff5c00、size 0x8000，对应终点为 0x0fffdc00，而非旧报告误写的 0x10003c00。修正见 P3-START 第 4.1 节及第 8 节勘误。


## 8. P4-1 网络维护基础（2026-11-19）

历史阶段决策曾于 2026-11-19 将 P3 标记 COMPLETE 并开启 P4；该状态已被当前用户最新审计任务覆盖。P4-1 既有源码审计仅作为历史文档，当前 P4 PAUSED；源码审计证据：
- `reports/P4-NETWORK-BASELINE-AUDIT-2026-11-19.md`
- `reports/P4-1-NETWORK-DRIVER-NETLOOP-AUDIT-2026-11-19.md`

| 测试/审计项 | 结果 | 说明 |
|---|---|---|
| PA-220 DHCP 命令入口与状态机 | PASS（STATIC AUDIT） | 复用既有 BootpRequest/DhcpHandler；不移植 uBootKit 的另一代实现 |
| PA-220 Ping 命令与 ARP/ICMP | PASS（STATIC AUDIT） | 复用既有 ping.c；10 秒超时调用 eth_halt |
| MGT AGL 设备注册链 | PASS（STATIC AUDIT） | AGL 模式命名 octrgmii0；专用 management port 名称 octmgmt0 |
| DHCP autoload=n 安全门 | PASS（STATIC AUDIT） | autoload 明确为 n 时 DHCP ACK 不进入 TFTP；必须实机 printenv 确认 |
| DHCP/Ping RAM 环境及 NetLoop 退出路径 | PASS（STATIC AUDIT） | DHCP 成功后 setenv 更新 RAM 环境；NetLoop 清理 handler 并 halt 网卡；不 saveenv |
| 最终构建 .config / include/autoconf.mk | EVIDENCE MISSING | 固定源码 GitHub 中无构建输出快照；需从已有构建目录只读提取 |
| 运行时 autoload/ethact/ethprime/device list | NOT RUN | 待串口只读核验 |
| DHCP-only 实机测试 | NOT RUN | 必须先确认 autoload=n |
| Ping 成功/超时/接口恢复实机测试 | NOT RUN | 待 DHCP 阶段后执行 |
| TFTP 接收 / Server / PUT 实机测试 | BLOCKED | validator 未实现；禁止执行 |

本批仅进行固定源码只读审计和文档更新；未构建、未运行 DHCP/Ping/TFTP、未修改持久化环境或存储。下一步先从既有构建目录读取配置快照，再读取串口运行时网络环境。


- P3 构建产物需区分主 MAP 与辅助 System.map：0001 构建中主 MAP 576,978 bytes；用户报告 System-octeon_pa220.map 为 0 bytes。0002 后续存在 host nm 生成非空 System.map 的历史观察，但后续又出现空文件结果；最终状态不得在未核对具体构建目录/时间点时统一标为 PASS。
- EEPROM：CONFIG_SYS_I2C_EEPROM_ADDR_LEN=1 与板级 24C256 注释之间的寻址关系、物理器件容量仍 NOT VERIFIED；OCTEON_EEPROM_MAX_TUPLE_LENGTH=256 仅是单 tuple 上限，不等于物理容量或整个 EEPROM 的有效访问窗口。


- **P3 clock-note issue (FAIL / unresolved):** 固定源码复核表明 CORE/SCLK 是 reset CSR multiplier 派生值；但 DDR 的 gd->mem_clk 在 DRAM init 后可能由 CN70XX IPD/DCLK counter ratio 动态测量。0002 patch 的统一 NOTE 和 DDR source 行未准确表达该区别。原始 board/clock log 保留不改，已验证补丁未擅自修改；如要修复需另行生成补丁并重新构建验证。详情见 P3-READONLY-AUDIT 第 12 节及历史账本 P3-H050。


- **Board/MAC provenance:** 历史 TLV 用户输出中的 BOARD_DESC 与 MAC_ADDR tuples 和 board/clock 原始日志运行时字段一致，固定源码确认 tuple 命中后复制到 global data；本设备本次运行的 descriptor/MAC 来源可交叉确认（账本 P3-H051）。通用 random-MAC fallback 仍存在。


- **P3 artifact archive:** 历史 build/CRC/SHA 用户证据已记录，但当前 GitHub tree 未找到 P3 0001/0002 ELF、BIN、主 MAP 或完整 build log；根目录两个 build log 均为 0 bytes。P2-3 的 20261118 产物不得冒充 P3。详见 `reports/P3-BUILD-ARTIFACT-AUDIT-2026-11-19.md`。构建执行历史 PASS 与产物归档完整性分开判定。


- P3 0001/0002 历史命令与结果摘要已分别保存为 session-transcribed 文件：`test-logs/2026-11-19_P3_0001-build-artifacts.session-transcribed.txt`、`test-logs/2026-11-19_P3_0002-patch-recovery.session-transcribed.txt`。两者均非原始 build/terminal log；原始产物缺失见 P3-BUILD-ARTIFACT-AUDIT。


## 10. 当前阶段控制覆盖（2026-11-19 最新用户指令）

当前 P3 全历史证据审计仍为 **IN PROGRESS**。此前 TEST-PLAN 第 7 节的 P3 CLI/只读诊断 PASS 及第 8 节的 P4-1 静态审计，是历史阶段记录；不能据此宣布本次全历史审计 COMPLETE。P3-HISTORICAL-EVIDENCE-AUDIT 和 CHECKPOINT 所列证据缺口、P3-H050 clock-note FAIL、P3-H053 构建原件缺失等仍须纳入最终判定。

P4-1 两份网络报告仅为既有静态源码审计，没有构建或实机网络测试；当前不得继续读取 P4 构建目录、执行 DHCP/Ping 或开展 P4 开发。P4 执行状态以 P4-PLAN 的 PAUSED 为准。


## 11. P4 当前执行恢复（2026-11-19）

本节按用户最新明确指令覆盖此前第 8、9、10 节中“P4 PAUSED / 不得执行 P4”的历史控制语句。当前继续 P4，不重做 P1/P2/P3，也不重新开展 P3 历史证据审计。

当前证据与状态：
- 有效 PA-220 配置头已由用户提供：CONFIG_OCTEON_PA220=1、CONFIG_OCTEON_PCI_HOST=0、CONFIG_OCTEON_CN70XX=1、CONFIG_BOARDDIR=board/octeon/pa220。它确认目标板配置，但不独立证明最终网络 CONFIG 编译值。
- 用户已提供运行时 autoload=n、ethact=octrgmii0、ethprime=octrgmii0；disable_networking、ipaddr、serverip、gatewayip、netmask、dnsip、bootfile 均未定义。不得重复索取。
- DHCP/Ping/MGT AGL 注册链已有固定源码静态审计；不得把静态审计标记为实机通过。
- 最终网络编译配置证据仍 PARTIAL；Ethernet 运行时设备枚举/注册证据仍 PARTIAL。
- DHCP-only、Ping 成功、Ping 超时、Ping 后恢复均 NOT RUN。
- TFTP 接收/Server/PUT 保持 BLOCKED；不执行任何 TFTP 命令。

按 P4-1（最终构建配置）、P4-2（Ethernet 实际注册）、P4-3（DHCP-only）、P4-4（Ping 成功/超时/恢复）、P4-5（归档）的顺序推进。只复用已有构建资料，不重新构建；只在静态证据不足时索取最少的新实机输出。所有操作继续遵守 RAM-only 和禁止持久化写入约束。

当前检查点：`reports/P4-CONTINUATION-2026-11-19.md`。
