# PA-220 U-BootKit 项目路线图

## 节点总览

| 节点 | 名称 | 主要交付 | 完成判据 |
|---|---|---|---|
| P0 | 项目文档体系 | 章程、仓库基线、功能矩阵、决策记录、测试计划、节点报告 | 文档进入 GitHub，版本和分支边界清楚 |
| P1 | uBootKit 功能源码审计 | 已完成 A0～A5 固定源码静态审计；明确适配结论、风险和运行时待验证项 | 所有目标功能均有适配结论和证据来源；未构建/未实机验证项单独标记 |
| P2 | PA-220 基线与构建复核 | 固定源码、补丁叠加、工具链、干净构建、关键对象静态核查和 RAM 启动 | **已完成（2026-11-19）**；未完成的只读硬件核查转入 P3，见 `reports/P2-COMPLETION-2026-11-19.md` |
| P3 | CLI/只读诊断完善 | 板卡、内存、时钟、网络、存储、EEPROM、CPLD 只读信息 | 编译和 RAM 启动验证通过；无持久化写入 |
| P4 | 网络维护基础 | MGT 网络配置、DHCP Client/Server 评估、下载服务 | 网络功能逐项可复现，退出服务后 CLI/autoboot 正常 |
| P5 | HTTP/Web 只读管理 | 状态页、系统信息、网络与存储概览 | MGT 隔离网络访问；无写接口；压力/异常请求安全 |
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
4. **RAM 启动**：优先使用原厂 Stage2 的 TFTP 将新 Stage3 加载到 RAM 测试，不执行 SPI/eMMC/NAND/EEPROM 写入。统一 pre-write validator 尚未实现前，不把新 U-Boot 自己的 TFTP 接收路径用于任意 RAM 地址的安全上传。
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

**P3 CLI/只读诊断完善：COMPLETE。** 已完成 0001/0002 只读命令补丁、构建、Stage2/TFTP RAM 启动和 `pa220 status/env/memory/board/clock` 实机验证；并完成 `bdinfo`、loadaddr、named/free bootmem、meminfo、TLV EEPROM tuple/checksum/END/终止偏移及 SPI NOR W25Q128BV 只读识别。证据索引见 `TEST-PLAN.md` 第 7 节、`reports/P3-COMPLETION-2026-11-19.md` 及 `reports/P3-READONLY-AUDIT-2026-11-19.md`。

P3 COMPLETE 不等于所有候选诊断均通过。Ethernet 物理 link 仍 NOT VERIFIED；eMMC 初始化仍 BLOCKED；USB 枚举仍 NOT VERIFIED；MAC descriptor 来源未单独证明；时钟值不是独立频率测量。统一 `octeon_load_range_check(uint64_t addr, uint64_t len)` 尚未实现，任意地址 TFTP/HTTP/ELF 接收仍 BLOCKED。P1 的非阻断风险不因阶段完成而关闭。

### P4：网络维护基础（IN PROGRESS）

本阶段主题：**DHCP Client、TFTP、Ping**。固定审计基线不变：
- PA-220：`IronDemiGod/pa220-openwrt@801717ba6a8a59ed2cfd53bf3e1b19841fecd2e8`
- uBootKit：`chenxin527/uboot-qsdk12.5-build@9ce315735ac88fd0a7c7697bc527a8616ed0a972`

首批源码审计已确认 PA-220 已具备 DHCP Client、Ping、TFTP GET/PUT/Server 的旧版实现；DHCP Client 和 Ping 以验证现有实现为主，不直接移植另一代 U-Boot 代码。TFTP 接收路径直接 memcpy 到 `load_addr + offset`，尚无统一 RAM validator，禁止启用未经验证的任意地址接收。uBootKit 的 IPQ/FDT/TrustZone 地址限制不适用于 PA-220。

P4 计划及首批审计见 `P4-PLAN.md`、`reports/P4-NETWORK-BASELINE-AUDIT-2026-11-19.md`。当前仅完成源码审计，尚未进行 P4 构建或网口实机验证。


## P3 历史证据审计覆盖（2026-11-19）

此前 P3 CLI/只读诊断阶段 COMPLETE 是历史阶段记录。用户已要求对 P3 开始至今全部相关会话进行证据追溯和仓库查缺补漏；当前审计状态 **IN PROGRESS**。在 `P3-HISTORICAL-EVIDENCE-AUDIT-2026-11-19.md` 和 `P3-HISTORICAL-AUDIT-CHECKPOINT.md` 明确完成之前，不得开始 P4 实际开发。P4 计划与已有源码审计仅作为历史文档保留，执行状态以 `P4-PLAN.md` 的 PAUSED 标记为准。
