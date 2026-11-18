# P1-A0：uBootKit 源码结构与模块边界（第一轮）

- 状态：已完成本轮源码结构盘点；未开始逐项功能审计
- 日期：2026-11-18
- 审计对象仓库：`chenxin527/uboot-qsdk12.5-build`
- 固定分支/提交：`main` / `9ce315735ac88fd0a7c7697bc527a8616ed0a972`
- Git tree：`aa3ee98f19c4bcc5c56b920b4f54bb84807fdfa7`
- 对照目标：PA-220 固定源码 `sdzbzyh/pa220-openwrt@801717ba6a8a59ed2cfd53bf3e1b19841fecd2e8`
- 文档分支：`feature/pa220-ubootkit-web`
- 本记录不代表任何功能已适配、启用、编译或实机验证。

## 1. 固定版本与仓库状态复核

1. uBootKit 上游 commit API 返回的 SHA 与固定值完全一致；提交为 README 文档改进合并，树 SHA 为上述值。
2. PA-220 源码 commit API 返回的 SHA 与固定值完全一致。
3. 文档分支当前 HEAD 为 `86f966e395c1303f70c489a726f38034498254fe`，提交说明为 `docs(pa220): finalize P0 milestone report`。
4. 文档分支最近提交记录中目前只看到 P0 的两个文档提交（`a5f0d21...`、`86f966e...`），没有 P1 审计提交。
5. 本轮所有源码读取均指定 uBootKit 固定 commit；没有切换上游分支或 PA-220 基线。

## 2. 仓库完整树概况

GitHub Git Trees API 对固定 commit 的递归树返回 `truncated=false`，共 42,315 个 tree entries。顶层包括：

- `.github/`：GitHub Actions 工作流。
- `.gitignore`：仓库忽略规则。
- `LICENSE`：仓库许可文件。
- `README.md`：项目说明、支持设备与固件格式说明。
- `build.sh`：多平台、多设备构建与发布辅助脚本。
- `doc/`：项目文档。
- `staging_dir/`：预置的主机/交叉编译工具链及其头文件、库和工具；属于构建环境产物，不是 U-Boot 功能源码，不能把其中的主机工具误判为目标固件功能。
- `u-boot-2016/`：U-Boot 源码主体及 uBootKit 扩展。

其中 `u-boot-2016/` 子树包含 12,869 项（1,252 个目录、11,617 个文件）。这是本轮结构分类的核心范围。

## 3. U-Boot 主体目录分类

以下按固定树中真实存在的目录归类。目录用途是源码组织层面的初步定位，功能是否实际编入目标镜像仍须检查 Kconfig、Makefile、defconfig 和调用链。

| 目录 | 主要用途 / 审计边界 |
|---|---|
| `api/` | U-Boot standalone/application API。 |
| `arch/` | CPU 架构代码；本项目重点是 `arch/mips/`，其他架构只用于识别通用代码边界。 |
| `board/` | 板级代码；重点检查 `board/qca/arm/common/`、各 IPQ SoC 目录及其板级配置。 |
| `common/` | U-Boot 公共启动流程、命令框架、环境变量、镜像处理和通用管理命令。 |
| `configs/` | 各板型 defconfig；用于确定功能配置入口，不能仅凭文件存在推断启用。 |
| `disk/` | 分区表、磁盘/块设备相关通用实现。 |
| `doc/` | U-Boot 文档、驱动模型和协议说明。 |
| `drivers/` | 设备驱动；重点是 MMC、MTD/SPI NOR、网络、GPIO、I2C、USB、PCI 等。 |
| `dts/` | 设备树及相关构建输入。 |
| `examples/` | standalone 示例程序及 API 示例。 |
| `failsafe/` | uBootKit Web failsafe 主体：HTTP URI 页面资源、模块处理器、入口命令和资源打包。 |
| `fs/` | FAT、ext4、UBIFS、JFFS2 等文件系统读取实现。 |
| `include/` | 公共头文件、架构/驱动接口和配置头文件。 |
| `lib/` | 通用库、压缩、RSA、libfdt 等。 |
| `net/` | U-Boot 网络栈及协议服务；本仓库含 HTTPD、TCP、DHCP server、TFTP、TelnetD 等源码。 |
| `post/` | 上电自检框架及 CPU/板级/驱动测试。 |
| `scripts/` | Kconfig、构建辅助脚本。 |
| `test/` | U-Boot 测试代码。 |
| `tools/` | 主机侧镜像、环境、构建和维护工具；不等于目标端命令。 |

源码根目录同时有 `Kconfig`、`Makefile`、`Kbuild` 等构建入口。固定树中没有独立的 `u-boot-2016/cmd/` 目录；该版本命令实现主要位于 `common/cmd_*.c` 和各子系统目录。

## 4. uBootKit 扩展模块边界（已由源码入口确认）

### 4.1 Web failsafe 框架

- `failsafe/Kconfig`：目前可见的配置入口包括 Web MAC 管理开关及 MAC 格式、分区名、偏移/标签、地址数量等选项。该文件不是整个 HTTPD 的唯一配置入口。
- `failsafe/Makefile`：非 SPL 构建时编入 `modules/`、`fs.o`、`failsafe.o`；通过 `makefsdata` 将 `fsdata/*` 生成 `httpd-fsdata.c`。
- `failsafe/failsafe.c`：定义 `httpd` 命令、创建 80 端口 HTTP 实例、注册各 URI 模块、驱动网络收发主循环，并按配置启动 DHCPD/TelnetD；IPQ5332/IPQ9574 有额外 PPE kick/keep-alive 调用。
- `failsafe/fs.c`、`failsafe/fs.h`：静态 Web 文件系统查找/描述接口。
- `failsafe/fsdata/`：HTML、JS、CSS 页面资源（含 index、backup、env、mac、network、reboot、settings、syslog、webterm 页面）；属于前端资源，不是后端业务实现。
- `failsafe/makefsdata`：将页面资源转为编译期 C 数据的生成脚本。

### 4.2 Web URI 业务模块

`failsafe/modules/Makefile` 将下列模块纳入构建（MAC 模块受 `CONFIG_FAILSAFE_MAC_MANAGEMENT` 控制）：

| 文件 | 从模块命名得到的初步职责定位；具体行为待逐文件核查 |
|---|---|
| `backup.c` | 备份/恢复相关页面与处理逻辑 |
| `env.c` | 环境变量查看/管理 |
| `helpers.c` | 模块共享辅助函数 |
| `mac.c` | MAC 地址管理 |
| `mibib.c` | MIBIB/分区信息相关 |
| `misc.c` | 杂项系统操作 |
| `network.c` | 网络设置/网络信息 |
| `sysinfo.c` | 系统/硬件信息展示 |
| `syslog.c` | 日志查看 |
| `upgrade.c` | 固件上传、识别、校验和升级 |
| `webterm.c` | Web 终端 |

模块注册入口是 `failsafe/failsafe.c` 中的 `register_uri_handlers_funcs[]`，声明集中在 `failsafe/modules/modules.h`。页面、URI handler、底层命令和驱动的真实调用链需逐模块确认。

### 4.3 HTTP/TCP 与网络服务

已确认存在的源码：

- `net/httpd.c`：HTTP server、实例/URI handler 管理、请求/响应及上传缓冲区逻辑。
- `net/tcp.c`、`include/net/tcp.h`：TCP 传输层接口和实现。
- `net/net.c`、`net/eth.c`：U-Boot 网络主循环及以太网设备框架。
- `net/bootp.c`：BOOTP/DHCP 客户端流程。
- `net/dhcpd.c`：DHCP server。
- `net/tftp.c`：TFTP。
- `net/telnetd.c`：Telnet 服务。
- `net/wget.c`：HTTP 下载客户端相关实现。
- `net/Kconfig`、`net/Makefile`：网络配置和目标对象编入入口。

`net/Makefile` 中 HTTPD、DHCPD、TCP、TelnetD、WGET 分别受独立 CONFIG 项控制；目录存在不代表这些功能已在任一具体设备配置中启用。

### 4.4 Qualcomm/IPQ 平台边界

- `board/qca/arm/common/`：Qualcomm ARM/IPQ 共用板级支持。已发现 `board_init.c`、`failsafe.c`、`cmd_bootqca.c`、`fdt_fixup.c`、`env.c`、`ethaddr.c`、`crashdump.c`、固件解包/解密辅助文件等。
- `board/qca/arm/ipq40xx/`、`ipq5018/`、`ipq5332/`、`ipq6018/`、`ipq806x/`、`ipq807x/`、`ipq9574/`：各 SoC 板级初始化、时钟和头文件。
- `board/ipq*/Kconfig`：各平台板型配置入口。
- `board/qca/mips32/`：QCA MIPS32 设备支持（如 qca953x/qca955x/qca956x、以太网 PHY、Flash、PCI 等）。这部分是 Qualcomm MIPS32 代码，不应与 PA-220 的 OCTEON III MIPS64 代码混为一谈。
- `arch/mips/`：U-Boot 通用 MIPS 架构实现。是否能复用到 OCTEON n32 ABI，必须对照 PA-220 2013.07 源码及 OCTEON 专用实现逐项验证。

### 4.5 存储、文件系统与固件路径

优先审计目录/文件边界：

- 命令入口：`common/cmd_mmc.c`、`cmd_sf.c`、`cmd_flash.c`、`cmd_flashrw.c`、`cmd_mtdparts.c`、`cmd_part.c`、`cmd_fat.c`、`cmd_ext4.c`、`cmd_fs.c`、`cmd_ubi.c`、`cmd_ubifs.c`、`cmd_nand.c`。
- 环境变量存储后端：`common/env_mmc.c`、`env_sf.c`、`env_flash.c`、`env_nand.c`、`env_ubi.c`、`env_common.c`。
- 驱动：`drivers/mmc/`、`drivers/mtd/`、`drivers/spi/`、`drivers/block/`。
- 分区与文件系统：`disk/`、`fs/`。
- uBootKit Web 业务：`failsafe/modules/backup.c`、`mibib.c`、`upgrade.c`，以及 Qualcomm `board/qca/arm/common/` 对应实现。

固件升级模块需要特别追踪是否构造并执行 `mmc erase/write`、SPI/NAND erase/write 等破坏性命令；审计期间不运行这些命令，不将持久化写入作为测试默认步骤。

## 5. 对 PA-220 的初步适配边界（仅结构判断）

PA-220 目标是 OCTEON III CN7130、MIPS64 big-endian、n32 ABI，现有 U-Boot 基线为 2013.07。当前只做目录级边界标注：

1. **优先研究的通用功能**：HTTP/TCP 服务器上层、静态页面资源、URI handler 组织、环境变量/系统信息页面、TFTP/BOOTP/DHCP 等协议逻辑、通用镜像解析与文件系统逻辑。仍需核对 U-Boot API 版本差异、编译条件、内存布局和调用链。
2. **可借鉴但需重写/替换底层的功能**：IPQ 专属 Web failsafe 外围控制、网络启动时序、存储分区识别、eMMC/SPI NOR 访问、硬件信息收集。应保留业务逻辑作为参考，不直接移植 Qualcomm SMEM、MIBIB、IPQ PPE、SCM/PSCI 或 IPQ SoC 初始化路径。
3. **明确不应直接移植的部分**：ARM IPQ SoC 启动、时钟/DDR/安全固件接口、ARM 专用板级代码；QCA MIPS32 SoC 初始化/寄存器/PHY 代码也不能直接用于 OCTEON III。
4. **OCTEON 专项待核**：CN7130 的启动/缓存/内存初始化、以太网接口和 PHY、SPI NOR/eMMC 控制器、CPLD/板级寄存器访问、big-endian 与 n32 指针/结构体 ABI、旧版 U-Boot 网络栈和驱动 API。

以上不是适配结论；任何一项进入“可移植/不可移植”最终清单前，必须给出源文件证据与 PA-220 对应证据。

## 6. 下一批逐文件审计顺序

### P1-A1：HTTP/TCP/Web 核心链路
1. `net/httpd.c`、`include/net/httpd.h`
2. `net/tcp.c`、`include/net/tcp.h`
3. `failsafe/failsafe.c`、`failsafe/Kconfig`、`failsafe/Makefile`
4. `failsafe/fs.c`、`fs.h`、`makefsdata`
5. 页面与 URI 注册关系（`modules.h`、页面 JS/HTML 请求 URL）

### P1-A2：Web 业务模块（先只读/低风险）
1. `sysinfo.c`、`network.c`、`syslog.c`、`env.c`
2. `misc.c`、`webterm.c`
3. `backup.c`、`mibib.c`、`mac.c`
4. `upgrade.c`（最后审计，逐条标注破坏性写操作）

### P1-A3：网络服务
1. `net/Makefile`、`net/Kconfig`
2. `bootp.c`、`dhcpd.c`、`tftp.c`、`wget.c`
3. `eth.c`、`net.c`、`net_abort.c`、`telnetd.c`
4. 设备侧网卡驱动与 Qualcomm 网络钩子，确认 Web 启动时序依赖。

### P1-A4：存储与固件管理
1. MMC/eMMC 命令、驱动及块设备层
2. SPI NOR/MTD 命令与驱动
3. 分区表、文件系统、环境变量后端
4. 固件解析、校验、备份、写入命令生成和启动切换。

### P1-A5：硬件信息与平台隔离
1. IPQ common/SoC hooks：硬件信息、MAC、分区、Flash 探测、LED、CPLD 类入口
2. 对照 PA-220 OCTEON III 板级代码、DTS、配置和驱动
3. 汇总通用层、可重写层、不可移植层及风险。

## 7. 本轮尚未完成 / 明确不作结论的事项

- 尚未逐文件阅读各 Web 模块，模块表中的职责目前仅为源码文件名和注册入口给出的初步定位。
- 尚未核实任一 IPQ defconfig 对 HTTPD、DHCPD、TCP、TelnetD、MAC 管理等选项的实际启用状态。
- 尚未追踪 Web 文件上传/下载的数据生命周期、缓冲区边界、请求解析健壮性与认证机制。
- 尚未确认 DHCP 报文过滤的实现位置与具体条件。
- 尚未审计固件解析、分区识别、备份恢复、eMMC/SPI NOR 写入风险。
- 尚未对比 PA-220 U-Boot 2013.07 的 API/驱动差异，也未进行编译或实机验证。
- 未进行任何功能代码修改或硬件写入。

## 8. 续接检查点

下一步从 **P1-A1 HTTP/TCP/Web 核心链路** 开始，先读固定 commit 的完整文件，再逐项追踪函数调用、CONFIG 条件、Makefile 对象关系、请求/上传缓冲区和错误路径。随后与 PA-220 2013.07 源码进行逐项对照。

审计证据必须记录固定 commit 下的文件路径、函数/宏/CONFIG 名称；必要时记录行号或代码片段。所有结论分为“源码存在”“配置启用”“编译成功”“实机验证”四种状态，不得混用。
