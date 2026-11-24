# PA-220 U-BootKit 项目路线图

## 节点总览

| 节点 | 名称 | 主要交付 | 完成判据 |
|---|---|---|---|
| P0 | 项目文档体系 | 章程、仓库基线、功能矩阵、决策记录、测试计划、节点报告 | 文档进入 GitHub，版本和分支边界清楚 |
| P1 | uBootKit 功能源码审计 | 已完成 A0～A5 固定源码静态审计；明确适配结论、风险和运行时待验证项 | 所有目标功能均有适配结论和证据来源；未构建/未实机验证项单独标记 |
| P2 | PA-220 基线与构建复核 | 固定源码、补丁叠加、工具链、干净构建、关键对象静态核查和 RAM 启动 | **已完成（2026-11-19）**；未完成的只读硬件核查转入 P3，见 `reports/P2-COMPLETION-2026-11-19.md` |
| P3 | CLI/只读诊断完善 | 板卡、内存、时钟、网络、存储、EEPROM、CPLD 只读信息 | 编译和 RAM 启动验证通过；无持久化写入 |
| P4 | 网络维护基础 | DHCP Client/Ping 实机验证与 TFTP 接收安全门 | **IN PROGRESS**；P4-2 validator Host 42/161、OCTEON N32 对象/完整 U-Boot 链接 PASS；TFTP store_block guard Host 14/64、tftp.o 编译及 libnet.o 链接 PASS；实际接收仍 BLOCKED；DHCP/Ping 实机验证未完成 |
| P5 | HTTP/Web 只读管理 | TCP 接入、前台生命周期、有界 HTTP parser、只读 URI handler 与实机验收 | **P5-2C 已完成（2026-11-23）**；云端/Host/实机证据见 `reports/pa220/P5-2C/ACCEPTANCE.md` |
| P6 | RAM 文件接收与校验 | 上传、进度、大小限制、CRC/SHA、格式识别 | 上传仅影响 RAM，边界和错误路径通过测试 |
| P7 | 固件解析与安装预览 | 固件格式解析、目标映射、dry-run 报告 | 不写存储即可完整展示安装计划 |
| P8 | eMMC 安装事务 | 备份/回退设计、分区写入、读回验证 | 专项审查和用户授权后，测试设备写入成功并可恢复 |
| P9 | 启动管理与恢复 | 正常/备用/RAM/恢复启动选择 | 真实设备验证回退条件，不依赖未经证明的 bootcount |
| P10 | SPI 维护（独立高风险节点） | 只读探测、备份校验、受控更新设计 | 目标芯片/地址/备份确认，单独审查和授权 |
| P11 | 集成回归与发布 | 功能矩阵闭环、用户文档、版本和发布产物 | 所有适用功能有状态，风险和限制明确 |

## 依赖关系

- P1 是后续适配的输入，不允许以现有维护草案代替完整 uBootKit 源码审计。
- P2 与 P1 可并行准备，但任何实机功能结论都依赖可重复构建的镜像。
- P3 是 P4/P5 的诊断基础。
- P5 的 Web 只读版本必须先于 P6 文件上传。
- P6 必须先于 P7/P8；不得把接收文件与写入存储绑定为一个动作。
- P8 与 P10 是不同风险域。SPI 不得被默认纳入 eMMC 安装器。
- P9 的自动回退设计依赖设备启动链和持久化状态的实测证据。

## 节点工作纪律

- 每个节点有明确输入、输出、验收标准和结束报告。
- 不因某个功能在 Qualcomm 平台存在，就推定它可在 OCTEON 上复用。
- 新增补丁按功能拆分，尽量小而可审查；构建脚本负责确定性叠加。
- 任何持久化写入功能必须先完成 dry-run、边界检查、备份和恢复设计。
- 用户使用 TFTP 将镜像加载到 RAM 进行测试，因此早期阶段优先保留 RAM 测试方式；但 RAM 加载地址仍必须依据已确认的启动链资料和设备状态使用。

## P1 子阶段审计进度（2026-11-18）

本节是源码审计进度记录，不改变 P2–P11 的既定路线和依赖关系。

| 子阶段 | 内容 | 当前状态 | 证据 |
|---|---|---|---|
| P1-A0 | uBootKit 固定源码树与模块边界 | 已完成 | audit/P1-A0-UBOOTKIT-SOURCE-MAP.md |
| P1-A1 核心 | HTTPD/TCP/failsafe 核心链路 | 已完成 | audit/P1-A1-HTTP-TCP-WEB.md |
| P1-A1 页面 | fsdata 页面 URL、main.js 请求与 URI handler 映射 | 已完成 | audit/P1-A1-PAGE-URI-CALLCHAIN.md |
| P1-A2 第一组 | sysinfo/network/env/syslog | 已完成 | audit/P1-A2-READONLY-WEB-MODULES.md |
| P1-A2 第二组 | misc/webterm | 已完成 | audit/P1-A2-MISC-WEBTERM.md |
| P1-A2 第三组 | backup/mibib/mac | 已完成 | audit/P1-A2-BACKUP-MIBIB-MAC.md |
| P1-A2 最后 | upgrade 及全部持久化写入路径 | 已完成 | audit/P1-A2-UPGRADE-PERSISTENT-WRITE-AUDIT.md |
| P1-A3 | 网络服务（DHCP/BOOTP/TFTP/WGET/Telnet/网络驱动） | 已完成源码静态审计；未编译/未实机验证 | audit/P1-A3-BOOTP-DHCP.md；audit/P1-A3-TFTP-WGET-TELNET.md；audit/P1-A3-ETH-NET-ABORT.md |
| P1-A4 | 存储、分区、文件系统、环境后端和固件管理 | 已完成源码静态审计；未编译/未实机验证 | audit/P1-A4-EMMC-SPI-CORE.md；audit/P1-A4-GPT-FILESYSTEM.md；audit/P1-A4-ENV-SPI-BACKEND.md；audit/P1-A4-BOOTLOADER-UPDATE.md；audit/P1-A4-USB-STORAGE.md |
| P1-A5 | IPQ 硬件信息/平台钩子与 OCTEON 对照 | **已完成源码静态审计**：板级初始化/QLM/MDIO/PHY/CPLD/boot-bus 调用链已收敛；TLV 当前 U-Boot 有效访问窗口 256 bytes（物理容量 UNKNOWN）；默认 8 GiB load block 源码公式为 physical [0x20000000,0x30000000)，0x81000000 UNSAFE；__tmp_load 生命周期/释放例外/descriptor 查询已确认；OCTEON ELF64 loader、PT_LOAD/section 写入路径和 PA-220 Cache API 语义已完成审计；TFTP/HTTP/ELF 统一 pre-write validator 仍未实现。最终 relocation/load block 运行时数值及硬件行为待构建后只读验证，不阻塞当前构建 | audit/P1-A5-IPQ-PA-HARDWARE-API.md；audit/P1-A5-TLVEEPROM-CPLD.md；audit/P1-A5-QLM-MDIO-PHY-BOOTBUS.md；audit/P1-A5-WATCHDOG-HANDOFF.md；audit/P1-A5-BOARD-INFO-MEMORY.md；audit/P1-A5-CALLCHAIN-RAM-CONVERGENCE.md；P1-COMPLETION-AND-P2-ORDER.md |

P1-A2 已完成的是固定 uBootKit commit 下的源码静态审计，不表示 PA-220 适配代码已实现、已编译或已实机验证。所有存储写入路径均只作静态追踪，没有执行。


## P1 总结与 P2 启动顺序（2026-11-18）

P1-A0～A5 的固定源码静态审计已完成。P1 完成仅表示源码调用链和适配边界有证据，不表示任何新功能已构建或实机通过。完整结论和下一阶段顺序见 `P1-COMPLETION-AND-P2-ORDER.md`。

### P2：基线与可重复构建复核

按以下顺序执行，不提前跳到 HTTP 上传或持久化安装：

1. **锁定构建输入**：确认集成仓库仍在 `feature/pa220-ubootkit-web`；PA-220 固定源码仍为 `801717ba6a8a59ed2cfd53bf3e1b19841fecd2e8`；uBootKit 固定审计源码仍为 `9ce315735ac88fd0a7c7697bc527a8616ed0a972`；确认补丁顺序、工具链版本和构建脚本，不修改两个固定源码仓库或 main。
2. **干净构建**：执行 `build-pa220.sh`，保存完整日志、最终 `.config`、工具链版本、map/ELF、`uboot_end`、产物大小和 SHA256。构建失败先修复构建确定性，不用未验证的硬件猜测补丁。
3. **构建产物静态核对**：确认 `CONFIG_CMD_OCTEON_ELF` 和 `cmd_elf64.o` 的实际编入情况；确认 `load_elf64_image`、`flush_cache`、`flush_dcache_range`、`cvmx_bootmem_find_named_block` 等目标符号/对象；记录 linker 与 relocation 计算所需符号。
4. **RAM 启动**：优先使用原厂 Stage2 的 TFTP 将新 Stage3 加载到 RAM 测试，不执行 SPI/eMMC/NAND/EEPROM 写入。validator 本体虽已实现且 Host PASS，但 OCTEON 目标验证及 TFTP 调用点接入尚未完成；不得把新 U-Boot 自己的 TFTP 接收路径用于任意 RAM 地址的安全上传。
5. **只读运行时确认**：记录启动日志中的 DRAM 容量、Stack Pointer、Base DRAM address、TLB map size（若有）；执行 `bdinfo`；执行 `printenv loadaddr octeon_reserved_mem_load_base octeon_reserved_mem_load_size`；通过只读 named-block 查询确认 `__tmp_load` descriptor 的 base/size，并记录最终 relocation 地址及使用长度。若现有 CLI 没有 named-block 只读显示命令，将其列入 P3 只读诊断，不为此阻塞 P2。
6. **TLV EEPROM 只读确认**：按 P1-A5 文档执行 `i2c dev 0`、`i2c probe 57`，并只读读取 offset 0、0x100、0x200 等窗口；记录 END tuple/MAC/board descriptor/clock tuple。严禁 eeprom write / TLV set/delete。
7. **P2 验收**：干净环境构建成功，产物头/大小/hash/map 记录完整；RAM 启动结果与只读运行时数据分开记录。编译成功不得标记为实机通过。

### P2 之后的开发依赖顺序

- **P3 CLI/只读诊断**：先输出板卡、时钟、DRAM、relocation、named block、网络/存储只读状态。
- **P4 网络维护基础**：复核 DHCP Client/TFTP 等已有路径；服务调度和网络行为通过实机验证。
- **P5 HTTP/Web 只读管理**：先建立 HTTP/TCP 服务和只读 handler，不提供任意命令执行、环境写入或存储写入。
- **P6 RAM 文件接收与校验**：在开放 HTTP/TFTP 上传前，先实现统一 `__tmp_load` pre-write validator，并分别接入 TFTP block、HTTP chunk、完整 payload reservation 和 ELF segment；加入 checked add/multiply、source/destination 范围与 Cache 同步。未完成此项不得把上传功能标记为安全可用。
- **P7 固件解析与安装预览**：只解析 RAM payload，生成 dry-run 目标映射，不写介质。
- **P8 eMMC 安装事务**：单独设计备份/回退/读回校验，获得专项审查和明确授权后才测试。
- **P9 启动管理与恢复**：依赖真实启动链和持久化状态证据。
- **P10 SPI 维护**：独立高风险节点，默认只读探测/备份；写入另行审查授权。
- **P11 集成回归发布**：矩阵逐项闭环，清楚区分源码、构建、RAM 启动和实机功能状态。

### 持续有效的运行时验证纪律

不影响阶段性构建且可在构建后通过只读命令确认的事项，不作为静态审计阻塞项；必须记录并在相应构建后提醒用户执行命令、反馈结果。任何 `saveenv`、EEPROM 写入、SPI/eMMC/NAND/flash 持久化写入仍然禁止，除非未来独立获得明确授权。


## P2 阶段收尾状态（2026-11-19）

P2-1、P2-2、P2-3 干净构建与 RAM 启动、P2-4 关键对象/符号静态核查均已完成到足以进入下一阶段的程度。详细证据见 `reports/P2-COMPLETION-2026-11-19.md`。

| 子项 | 内容 | 状态 |
|---|---|---|
| P2-1 | 固定源码、分支、补丁、构建脚本与输入核查 | **完成** |
| P2-2 | Debian 构建主机与 OCTEON SDK 工具链核查 | **完成** |
| P2-3 | 干净构建、ELF/BIN/map、哈希、原厂 Stage2/TFTP RAM 启动 | **完成**；RAM 镜像 CRC 变化作为非阻断疑点记录 |
| P2-4 | ELF loader、Cache、named-block 对象/符号及链接布局核查 | **关键对象/符号核查完成**；4-byte 布局差异记录待后续处理 |

P2 未完成的 `__tmp_load` 运行时 descriptor、板级只读命令、TLV EEPROM 只读检查转入 P3，不阻塞主线。任何存储写入仍禁止。

## P2-3 构建与 RAM 启动记录（2026-11-18 至 2026-11-19）

干净构建退出码为 0，产物哈希经再次计算与原始审计报告一致；新构建镜像通过原厂 Stage2/TFTP 从 RAM 启动并进入命令行。旧版 U-Boot 对照也可从 RAM 启动。新旧版本启动后均观察到加载镜像 RAM 数据 CRC 变化，现有证据不能将其归因于新增代码；不再重复扫描，详见 P2 收尾报告。

## P2-3 干净构建与产物审计（2026-11-18）

历史构建证据见 `audit/P2-3-CLEAN-BUILD-ARTIFACT-AUDIT.md`。后续已完成 RAM 启动与 P2-4 关键对象/符号核查；当前阶段状态以 `reports/P2-COMPLETION-2026-11-19.md` 为准。

## P3 最终收尾与 P4 启动（2026-11-19）

**历史阶段记录：P3 CLI/只读诊断曾标记 COMPLETE。** 该标签仅保留当日阶段决策，不代表当前 P3 全历史证据审计已完成；当前审计 IN PROGRESS，最终判定以 P3 历史证据账本/检查点为准。

历史 P3 CLI/只读诊断完成标签不等于所有候选诊断均通过。Ethernet 物理 link 仍 NOT VERIFIED；eMMC 初始化仍 BLOCKED；USB 枚举仍 NOT VERIFIED；MAC descriptor 来源已由 EEPROM tuple 与运行时输出交叉验证（P3-H051）；CORE/SCLK 为 CSR multiplier 派生值，DDR gd->mem_clk 在 DRAM init 后可能由计数器测量（P3-H050）；统一 `octeon_load_range_check(uint64_t addr, uint64_t len)` 尚未实现，任意地址 TFTP/HTTP/ELF 接收仍 BLOCKED。P1 的非阻断风险不因历史阶段标签而关闭。

### P4：网络维护基础（PAUSED — P3 历史证据审计未完成）

本阶段主题：**DHCP Client、TFTP、Ping**。固定审计基线不变：
- PA-220：`IronDemiGod/pa220-openwrt@801717ba6a8a59ed2cfd53bf3e1b19841fecd2e8`
- uBootKit：`chenxin527/uboot-qsdk12.5-build@9ce315735ac88fd0a7c7697bc527a8616ed0a972`

首批源码审计已确认 PA-220 已具备 DHCP Client、Ping、TFTP GET/PUT/Server 的旧版实现；DHCP Client 和 Ping 以验证现有实现为主，不直接移植另一代 U-Boot 代码。TFTP 接收路径仍直接 memcpy 到 load_addr + offset，尚未调用新 validator；禁止启用未经验证的任意地址接收。uBootKit 的 IPQ/FDT/TrustZone 地址限制不适用于 PA-220。

P4 计划及首批审计见 `P4-PLAN.md`、`reports/P4-NETWORK-BASELINE-AUDIT-2026-11-19.md`。当前仅完成源码审计，尚未进行 P4 构建或网口实机验证。


## P3 历史证据审计状态更正（2026-11-19）

此前 10:19–10:22Z 提交中曾声明 P3 历史证据追溯不再阻止 P4；该声明属于后续并行会话的历史决策，现已被当前用户最新任务明确覆盖。当前 P3 全历史证据审计仍 IN PROGRESS，P4 实际开发/构建/实机测试 PAUSED。

P3 未解决风险继续保留；阶段状态更正不等于风险关闭。


## P3 / P4 阶段状态更新（2026-11-19）

**P3 CLI/只读诊断：历史阶段曾标记 COMPLETE；当前全历史证据审计 IN PROGRESS。** 历史结果与未解决风险见 `reports/P3-COMPLETION-2026-11-19.md`。当前不得宣布 P3 COMPLETE。

**P4 网络维护基础：PAUSED。** P4-1 已有两批 PA-220 固定源码静态审计，仅作为历史材料保留：
- DHCP/Ping/TFTP 首批协议和命令路径：`reports/P4-NETWORK-BASELINE-AUDIT-2026-11-19.md`。
- MGT AGL 注册、配置条件、NetLoop/DHCP/Ping 生命周期：`reports/P4-1-NETWORK-DRIVER-NETLOOP-AUDIT-2026-11-19.md`。

当前仍缺最终构建 `.config`/ `include/autoconf.mk` 证据及串口运行时环境/设备列表；DHCP 和 Ping 实机测试 NOT RUN。下一步复用既有构建目录只读检查配置，再进行受控串口检查。未确认 `autoload=n` 前不执行 DHCP。TFTP 接收、TFTP Server 与 TFTP PUT 暂不实测；validator 目标验证和写入点接入完成前继续禁止接收。


## 当前阶段控制覆盖（2026-11-19 最新用户指令）

10:19–10:22Z 的并行提交曾将 P4 重新激活，并声明历史证据补档不阻断 P3 COMPLETE。该决定早于/不符合当前用户本轮明确任务要求。当前控制状态为：

- P3 全历史证据审计：**IN PROGRESS**；不得提前标记 P3 COMPLETE。
- P4 实际开发/构建/实机测试：**PAUSED**。
- 已有 P4-1 固定源码静态审计报告保留为历史材料，不代表本次继续执行 P4。
- 当前状态以本节、P4-PLAN 第 8 节、TEST-PLAN 第 10 节及 P3 历史审计检查点为准。


## P5 独立只读 Web 管理（启动于 2026-11-20）

P5 与 P4 并行推进：P4 负责 DHCP/Ping/TFTP 网络维护与实机验证；P5 仅负责只读 HTTP/Web。两者共享集成分支时，任何修改 `net.c`、`net.h`、`net/Makefile` 前必须重新核对 HEAD 和并行提交，不能覆盖另一会话的变更。

### P5-1：原生 TCP 接收与 Web 生命周期适配

专项静态审计已归档：`audit/P5-1-PA220-NATIVE-TCP-HTTP-ADAPTATION.md`。

- PA-220 原生 `NetReceive()` 不分派 TCP，现有 `NetLoop()` 是阻塞式协议循环，且 UDP/ARP/timeout 均为单例全局状态。
- 实现顺序：独立 TCP 静态单连接状态机 → 最小 IPv4 TCP 分派 hook → 前台 `pa220 web` 生命周期/统一清理 → 有界 HTTP parser 与只读 URI handler。
- 复用 PA-220 原生 Ethernet 收发和网络帧接口；不移植 Qualcomm/IPQ 专有网络驱动、寄存器、固定地址或 Cache API。
- 第一版只读：仅限 GET/HEAD 和审查过的状态 handler；无命令执行、环境/存储/CPLD 写入、上传、安装或重启。

当前状态：**P5-1 静态适配审计完成；代码实现未开始；未编译、未 RAM 启动、未实机验证。** 下一工作单元为独立 TCP 核心文件，暂不改动 `NetReceive()`，以降低与 P4 对网络核心文件并行修改的冲突风险。


## P5 最新状态与交接（2026-11-21）

本节为 P5 状态的最新覆盖记录；早期“代码实现未开始”的历史文字不再代表当前 TCP 核心状态。

- **P5-1A.2 独立静态单连接 TCP 核心：已实现。**
- **P5-1A.3 Host 协议回归：PASS，76 assertions passed / 0 failed。**
- **OCTEON SDK 5.1.0 下载、SHA-256 校验及 PA-220 完整交叉编译：PASS。** Actions run #3：`44856960192`；生成 BIN 1,362,216 bytes。
- **RAM 启动、真实网口 TCP 收发：NOT RUN。**
- **P5-1B：尚未实施，下一工作单元为最小 IPv4 TCP 分派 hook。**

### P5-1B 范围边界

仅在固定 PA-220 源码 `801717ba6a8a59ed2cfd53bf3e1b19841fecd2e8` 的 `NetReceive()` 中，为 IPv4 protocol 6 增加显式注册的窄分派接口。入口必须位于 IPv4 header/长度/校验、目的地址过滤及分片处理之后；TCP segment 长度仍需独立边界核验。未注册时保持原有丢弃行为。ARP、ICMP、UDP 的处理不得改变。

P5-1B 不包含 TCP 输出封装、Ethernet/MAC 邻居处理、前台 Web 生命周期、HTTP parser 或任何上传/写入接口。P5 与 P4 并行，修改 `net.c` / `net.h` / `net/Makefile` 前必须基于最新远端 HEAD 核对实际差异；不覆盖并行改动。所有补丁必须由真实源码差异生成，禁止手工猜测 hunk 行数。


## P5 验证范围更正（2026-11-21）

Actions #3 的 1,362,216-byte 镜像证明 PA-220 **baseline + 0001/0002 现有补丁**完整 OCTEON 构建通过；76/0 则证明独立 TCP core 的 Host focused regression 通过。二者不能合并解释为 TCP core 的目标交叉编译通过：当前 `build-pa220.sh` 只应用 `patches/pa220/*.patch`，这两个补丁不包含 TCP core；Actions 日志没有 `net/tcp.o` 编译证据。

故当前精确状态为：
- TCP core Host regression：PASS（76/0）。
- PA-220 baseline 完整 OCTEON 构建：PASS。
- TCP core OCTEON 目标对象编译/链接：NOT RUN。
- TCP 网络接入、RAM 启动及实机网络验证：NOT RUN。

**下一工作单元改为 P5-1A.3.1-T：独立 TCP core 的 OCTEON 目标对象编译验证。** 使用现有云端 SDK Release 和 PA-220 固定源码生成的配置/include 环境，不重复 Host 回归或完整 baseline 构建。只有该目标对象编译通过后，才进入 P5-1B 最小 IPv4 TCP 分派 hook。


## P5-1A.3 目标对象验证完成与 P5-1B 交接（2026-11-21）

专用 Actions run `44858430936` 已使用 Cavium GCC 4.7.0 / SDK Build 52，在固定 PA-220 源码配置下编译独立 TCP core 为 MIPS N32 OCTEON relocatable object：`net/tcp.o`，49,564 bytes，SHA-256 `16ccf75c9cb4e779753ee99fdfeadba21802e9d6a6ed009cb8b9d33da7240ca2`。配置、对象编译和 ELF inspection 均 PASS。完整证据见 `reports/pa220/P5-1A.3/20-octeon-tcp-object-compile.md`。

当前 P5-1A 验证状态：
- Host focused regression：PASS（76 passed / 0 failed）。
- Standalone TCP core OCTEON target object compile：PASS。
- PA-220 baseline full build：PASS。
- TCP core 与 PA-220 U-Boot 的集成链接：NOT RUN（因为当前尚未将 TCP core 加入 PA-220 net/Makefile）。
- RAM boot / physical TCP network: NOT RUN。

下一工作单元 **P5-1B**：将 TCP core 源码和头文件通过真实源码差异补丁纳入 PA-220；增加仅在显式注册时启用的 IPv4 protocol 6 接收分派；保持 ARP/ICMP/UDP 原路径不变；随后执行实际 PA-220 完整构建，确认 `net/tcp.o` 编入并最终链接成功。此阶段不实现 Web 循环、TCP Ethernet 输出或 HTTP parser。


## P5-1B 原生 IPv4 TCP 接收分派完成（2026-11-21）

**P5-1B 已完成代码集成、Cavium OCTEON 目标完整构建和最终链接验收。** 本节覆盖此前 P5-1B “尚未实施”的历史状态。

- 固定源码：`sdzbzyh/pa220-openwrt@801717ba6a8a59ed2cfd53bf3e1b19841fecd2e8`。
- 正式补丁：`patches/pa220/0003-pa220-native-ipv4-tcp-dispatch.patch`，包含现有 TCP 核心源码/头文件、网络注册接口、IPv4 protocol 6 分派和 `net/tcp.o` 构建集成。
- 分派位于原有 IPv4 头部/长度/校验、目的地址过滤和分片重组之后；TCP segment 长度独立验证；未显式注册 handler 时维持丢弃行为。
- ARP、ICMP、UDP 原有接收分支未改写。
- PA-220 最终链接启用 `--gc-sections`；通过从活动 `NetReceive()` 路径引用的链接保留表，确保注册入口及 TCP core 公共 API 保留在最终 ELF。
- GitHub Actions run `44861736535`：SDK 下载/校验、固定源码、补丁、完整目标构建、对象、最终 ELF、map、System.map 检查全部 PASS（所有验收返回码为 0）。
- 目标对象：`net/tcp.o` 53,016 bytes；网络聚合对象 `net/libnet.o` 401,774 bytes。
- 最终 ELF：7,792,898 bytes；MIPS N32 big-endian OCTEON executable，entry `0xc0000000`；最终 BIN 1,367,048 bytes。
- 最终 ELF/System.map 已确认包含 `net_set_tcp_handler`、`tcp_core_init/listen/input/poll/read/write/close`、`tcp_segment_parse`、IPv4 checksum 和 sequence helper 等符号。
- 完整日志与证据：`reports/pa220/P5-1B/10-octeon-integrated-build.log`、`11` 至 `17` 的 ELF/object/map/symbol 检查文件；Actions artifact ID `14690151455`。

**仍未验证：** RAM 启动、真实网口 TCP 接收/发送。P5-1B 只完成接收分派；没有 TCP Ethernet/IP 输出、MAC 邻居解析、前台 `pa220 web` 生命周期或 HTTP parser。

**下一入口：P5-1C — 前台 Web 生命周期与 TCP core 连接管理集成。** 该阶段应显式注册/注销 TCP handler、建立单连接上下文并接入 `tcp_core_input()`；另行安排 RAM 启动和真实网口验证。不得将本阶段构建成功等同于实机网络通过。


## 当前检查点与下一工作入口（2026-11-23）

### P5 完成状态

P5-1A 至 P5-1D 及 P5-2A、P5-2B、P5-2C 的阶段工作已完成；本轮正式验收以 `reports/pa220/P5-2C/ACCEPTANCE.md` 为准。P5-2C 包含：

- Host Harness：29 scenarios、161 assertions、0 failures；前台 `pa220 web` 命令测试通过。
- Cavium OCTEON N32 relocatable link 与 PA-220 完整 U-Boot 最终链接通过。
- 云端 BIN：1,380,488 bytes；SHA-256 `825b0e56312da6f4158cd94a5e2da2796b188bfe5d9dcecb5928d9304ad67343`。
- PA-220 实机通过 TFTP 将 Stage-3 镜像加载到 RAM 地址 `0x81000000` 后执行；Windows HTTP 测试 8/8 cases PASS，PCAPNG/TShark 输出支持 200/404/405 与关闭/RST 后新连接恢复行为。
- 串口日志是会话记录重建，不是原始 UART capture；RST 连接在 HTTP payload 发送前 FIN，服务器回 400 后客户端 RST+ACK，不得描述为 RST 中断部分请求。

### P4 状态未被 P5 取代

P4 网络维护基础仍保留历史计划中的未完成项：DHCP Client/Ping 实机验证和统一 RAM 接收范围验证器。P5 的静态地址 HTTP 测试成功不等于 DHCP/Ping 或 TFTP 接收验证通过。不得将 P4 标记为 COMPLETE。

### 下一开发入口：统一 RAM 写入前范围验证器

进入 P6 文件接收前，先完成 P4-2/P6 共用的安全前置项：

1. 基于固定 PA-220 源码和运行时 `cvmx_bootmem_find_named_block("__tmp_load")` descriptor，设计并实现 `octeon_load_range_check(uint64_t addr, uint64_t len)`。
2. 采用 checked arithmetic 和半开区间检查；处理 cached/uncached/physical alias 规范化；每次使用前重新验证 descriptor，确认与相关 globals 一致。
3. 证明目标区间不覆盖 U-Boot image、relocation、stack、GD、malloc、exception/bootmap/FDT 等保留区域；不得假定 `0x81000000` 是安全上传缓冲区。
4. Host 测试覆盖零长度、溢出、回绕、边界相等、跨界、alias、descriptor 缺失/变化等拒绝路径；目标端执行 OCTEON 编译/最终链接和只读运行时核对。
5. 在 validator 经 Host 与目标验证前，不开放 TFTP GET/Server 接收、HTTP upload/chunk 接收或 bootelf PT_LOAD/section 写入；不执行任何实际上传或持久化写入。

验证器通过后，才进入 P6 RAM-only 文件接收与 CRC/SHA/格式识别。P7 固件解析和安装预览仍只做 dry-run；P8 eMMC 写入、P10 SPI 维护属于独立高风险阶段，必须另行审查和明确授权。

**下一会话不要重做 P5-1/P5-2A/B/C，不要重新跑已通过的 Host/OCTEON/HTTP 测试。先恢复最新 HEAD，审查当前 validator 代码/调用点，再直接推进上述安全门。**


### P4-2 validator 实施进度（2026-11-24）

生产 validator 已加入集成树；Host Harness 32 scenarios / 121 assertions / 0 failures，ASan/UBSan 回归通过。专用 Actions 将使用固定 PA-220 源码基线与 Cavium OCTEON SDK 5.1.0 完成目标对象、N32 relocatable link 和完整 U-Boot final link。当前不把 Host PASS 写成目标 PASS；TFTP/HTTP/ELF 写入调用点仍未接入，全部 BLOCKED。详细记录：reports/pa220/P4-2/IMPLEMENTATION.md。


## P4-2 / P6 状态更正（2026-11-24）

本节覆盖本路线图中此前 P4-2 的旧状态描述。

- 生产 `octeon_load_range_check()` Host Harness：42 scenarios / 161 assertions / 0 failures；普通 GCC 与 ASan/UBSan 均通过。
- TFTP DATA `store_block()` guard Host Harness：14 scenarios / 64 assertions / 0 failures；普通 GCC 与 ASan/UBSan 均通过。测试编译的是 patch generator 使用的同一生产函数文本，并用 mock validator / intercepted memcpy 检查拒绝路径无写入。
- CN70XX 修改后的 `tftp.c` 已编译为 `tftp.o` 并进入 `libnet.o`；validator OCTEON object、N32 relocatable link、完整 PA-220 U-Boot final link 均通过。
- 权威证据：[Actions run #45374041373](https://github.com/sdzbzyh/uboot-qsdk12.5-build/actions/runs/45374041373)，集成提交 `908a58f43017f3e252b3ce0c613b4630072203e9`；报告 `reports/pa220/P4-2/IMPLEMENTATION.md`。

**仍未通过/未运行：** PA-220 runtime descriptor 只读核对、真实 TFTP GET/Server 接收、HTTP 上传、ELF segment 写入、DHCP/Ping 实机测试。P4 不得标记 COMPLETE；P6 仅可进入受控设计/实现，实际接收测试须等设备上电后的只读内存布局核对和独立授权。


## 当前阶段状态覆盖（2026-11-24，P6-0）

本节覆盖本路线图中较早的 P4-2 / P6 状态描述；历史记录保留作为审计轨迹。

- **P4-2：代码、Host、OCTEON 编译/链接及生成器幂等性均 PASS。** 权威 Actions：#45346000355、#45374041373、#45375848020；最新验证提交 `20acf9790b6ee15ed758db7006eaee69af0f2bf2`，当前分支 HEAD 在本次审查开始时为 `1a318fd562cdb3b475d92abdc5768b436cf154fd`。
- **P6：设计/源码审查和只读预检准备进行中。** 首个路径为 TFTP GET/RRQ；实际接收 NOT RUN。live `__tmp_load` descriptor NOT RUN，设备报告断电。
- P4-2 TFTP guard 是每个 DATA block 的写入前保护，不等同于完整文件 reservation、SHA-256/CRC 验收、格式识别或失败后有效状态管理。上述 P6 软件门仍需完成。
- DHCP Client/Ping 实机验证 NOT RUN；P4 overall 不得标记 COMPLETE。
- HTTP 上传、ELF PT_LOAD/section 写入仍 BLOCKED；TFTP Server/WRQ 和 PUT 不在首轮 GET 实机验收范围。
- 本轮没有重跑 P4-2 测试，没有进行实际文件传输，也没有任何持久化写入。

详细源码审查、测试向量和只读命令见 `reports/pa220/P6-0-SOURCE-REVIEW.md`。
