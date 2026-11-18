# PA-220 U-BootKit 项目路线图

## 节点总览

| 节点 | 名称 | 主要交付 | 完成判据 |
|---|---|---|---|
| P0 | 项目文档体系 | 章程、仓库基线、功能矩阵、决策记录、测试计划、节点报告 | 文档进入 GitHub，版本和分支边界清楚 |
| P1 | uBootKit 功能源码审计 | 按文件/函数建立完整功能清单，标注平台依赖 | 所有目标功能均有适配结论和证据来源 |
| P2 | PA-220 基线与构建复核 | 固定源码、补丁叠加、工具链和构建可重复性 | 干净环境构建成功，产物头和哈希记录完整 |
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
| P1-A4 | 存储、分区、文件系统、环境后端和固件管理 | 进行中：eMMC/SPI、GPT、EXT4/FAT、SPI 环境后端已审计；固件分区映射/bootloader update 待审计 | audit/P1-A4-EMMC-SPI-CORE.md；audit/P1-A4-GPT-FILESYSTEM.md；audit/P1-A4-ENV-SPI-BACKEND.md |
| P1-A5 | IPQ 硬件信息/平台钩子与 OCTEON 对照 | 尚未开始 | 待建立独立审计报告 |

P1-A2 已完成的是固定 uBootKit commit 下的源码静态审计，不表示 PA-220 适配代码已实现、已编译或已实机验证。所有存储写入路径均只作静态追踪，没有执行。
