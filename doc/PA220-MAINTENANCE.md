# PA-220 Maintenance U-Boot: 功能盘点与开发路线

## 目标

在现有 PA-220 OCTEON U-Boot 2013.07 的基础上，逐步增加一套可通过串口和 MGT 网口使用的维护环境，覆盖只读诊断、网络管理、镜像接收与校验、eMMC 固件维护和启动管理。

本项目不是把 Qualcomm IPQ U-Boot 移植到 MIPS。PA-220 继续使用独立的 Cavium OCTEON 源码树；本仓库通过可审查的补丁叠加维护功能。

## 平台边界

- CPU / SoC：Cavium OCTEON III CN7130，MIPS64，大端，n32 ABI。
- U-Boot：Cavium SDK 5.1 系列，U-Boot 2013.07。
- 启动角色：PA-220 的 SPI Stage 3，由 Palo Alto 前级 Stage 2 加载；前级已经完成 DRAM 初始化。不能把本镜像当成可从冷复位独立初始化 DRAM 的通用第一阶段。
- 网络：MGT 为 AGL/RGMII + Marvell 88E1512；前面板八个端口由两条 QSGMII 链路和 Marvell 88E1680 提供。
- 存储：板级 DTS 定义 8-bit eMMC、16 MiB SPI NOR、板载 TLV EEPROM、RTC、两个 USB 3.0 控制器和 CPLD。
- 环境变量：当前配置使用 SPI NOR 环境，主区 0xe00000，冗余区 0xe10000。saveenv 会写 SPI，不能当作 RAM-only 操作。

硬件定义和驱动是否在配置中，不等于已经在用户的机器上验证工作。所有功能都要区分“编译配置存在”和“实机验证通过”。

## 当前功能盘点

| 能力 | PA-220 当前源码配置 | 说明 |
|---|---|---|
| 串口命令行、hush 脚本、bootmenu | 已配置 | 命令行提示符为 pa-220# |
| DHCP 客户端、Ping、DNS、NFS、TFTP 下载/上传/服务器 | 已配置 | DHCP 是 U-Boot 自己获取地址，不是给电脑分配地址 |
| MII / PHY 管理 | 已配置 | 使用 OCTEON MDIO 与 Marvell PHY |
| eMMC / MMC | 已配置 | 8-bit 总线；EXT2/3/4 和 FAT 读取支持 |
| EXT4 / FAT 写入 | 已配置 | 具备命令配置，尚未做实机写入验证 |
| SPI NOR / sf | 已配置 | 驱动和命令存在；所有写入暂时保持禁用的操作纪律 |
| USB XHCI / Mass Storage | 已配置 | 两个控制器在 DTS 中定义，尚待实机验证 |
| GPT / EFI / DOS 分区 | 已配置 | 分区识别命令 |
| 镜像与校验 | 已配置 | iminfo、CRC32、SHA-1、MD5、SHA-256 等 |
| I2C / EEPROM / RTC / GPIO | 已配置 | 包括 PA-220 板载 TLV EEPROM |
| CPLD | 板级命令已实现 | cpld display/read；写寄存器 d 可能切换 SPI 芯片或重启 |
| HTTP Web 管理 | 未实现 | 需要为旧版 U-Boot 网络栈增加 HTTP 服务 |
| DHCP Server / DHCP 报文过滤 | 未实现 | 与现有 DHCP Client 不同 |
| IPQ 9008 / bootipq / MBN | 不适用 | Qualcomm 专有启动机制 |
| OpenWrt Sysupgrade TAR 自动解析与安装 | 未实现 | 需要单独设计文件解析、校验和安装事务 |

## 已开始实现：只读维护命令

本阶段通过本仓库的补丁目录向固定版本 PA-220 源码注入命令：

- 补丁：patches/pa220/0001-pa220-read-only-maintenance-command.patch
- 命令：pa220、pa220 status、pa220 env
- 构建入口：./build-pa220.sh /path/to/tools-gcc-4.7

命令设计为只读：

- pa220 / pa220 status：显示板卡编号、DRAM 容量、CPU 时钟、U-Boot relocation 地址、网络环境变量和 CPLD 状态。
- pa220 env：显示环境变量后端、主/冗余偏移和明确的 SPI 写入警告。

该命令不会调用 saveenv、sf 写命令或 CPLD 写操作。补丁已经进入开发分支，但必须重新运行构建脚本生成新镜像后才会出现在设备上；旧的 BIN 不会自动改变。

## 开发阶段

### M0 — 基线与只读诊断（当前阶段）

- [x] 固定 PA-220 上游源码 commit，保持独立源码树。
- [x] 完成配置级功能盘点。
- [x] 增加只读 pa220 status 和 pa220 env 命令补丁。
- [ ] 在用户的 OCTEON 工具链上重新构建并检查日志。
- [ ] RAM 启动后验证输出、网口、CPLD、eMMC、USB 等实际硬件状态。

### M1 — 网络维护服务

- 增加只读 HTTP 状态页，显示板卡、内存、网络、CPLD 和存储识别信息。
- 优先只监听 MGT 接口；不暴露到前面板业务网络。
- 复用 U-Boot 自身的网络收包路径，不引入 Linux socket/线程假设。
- HTTP 服务退出后必须能返回串口命令行和正常 autoboot 流程。

### M2 — HTTP 文件接收（只进入 RAM 暂存区）

- 提供文件上传和接收进度。
- 先确定 U-Boot 可用 DRAM 区间、当前镜像、malloc 区域、boot parameters 和内核加载地址，不能凭空指定固定缓冲地址。
- 限制最大上传大小，检查长度、CRC32、SHA-256 和文件格式。
- 上传完成后只显示“已接收/校验结果”，不触发任何 Flash 写入。

### M3 — 固件识别与 eMMC 安装事务

- 先识别 PA-220 实际使用的 OpenWrt 镜像和分区布局，再实现解析器。
- 支持列表和 dry-run：显示将写入的分区、偏移、长度和备份方案。
- 写入前校验板型、镜像结构、长度边界及目标分区容量。
- 安装前保留可恢复的旧内核或恢复镜像；写入后读回校验。
- 默认只把 eMMC 纳入安装器；SPI NOR 不由 Web 安装器操作。

### M4 — 启动管理和恢复

- 增加正常内核、备用内核、RAM 启动和恢复启动入口。
- 先使用可审计的 eMMC 文件/配置作为启动选择状态；不要为了实现菜单而频繁写 SPI 环境区。
- 内核文件存在性、镜像格式和启动参数在启动前检查。
- 启动失败回退策略必须基于设备实测，不假设有可靠的硬件 bootcount。

### M5 — SPI 维护（独立、高风险、默认关闭）

- 首先只做只读探测、JEDEC ID、容量和 CPLD 芯片选择状态报告。
- 先确认正常/备用 SPI 芯片区分、完整备份及校验。
- 真正的 SPI 更新必须是独立命令，要求串口物理确认、明确目标芯片和地址范围，并提供写后读回校验。
- Web 接口不提供 CPLD flash-select 写入，也不允许直接写 SPI。
- 不执行 cpld write -f d ... 作为常规维护步骤。

## HTTP 服务的安全边界

U-Boot 环境不是完整 Linux 用户空间；HTTP 服务不能依赖 POSIX socket、后台线程或动态内存无限增长。早期版本应使用有限状态机、固定大小请求缓冲和严格长度限制。

初版只读页面不需要写权限。文件上传阶段也只写入 RAM 暂存区。任何会改变持久存储的动作都应与上传/查看分离，并要求明确的二次确认。普通 HTTP 不提供传输加密，因此维护接口必须限定在隔离的 MGT 维护网络；不能把它开放到不可信网络。

## 本地构建与验证

在 Debian 本地仓库切换到本开发分支并更新：

    git fetch origin
    git checkout feature/pa220-maintenance
    git pull --ff-only

使用已安装的 OCTEON SDK 工具链：

    ./build-pa220.sh /home/debian/OCTEON-SDK/tools-gcc-4.7 2>&1 | tee pa220-build.log

新镜像默认输出到：

    bin/pa220/u-boot-octeon_pa220.bin

重新构建后先检查日志和镜像头，再通过原厂 U-Boot TFTP 加载到 RAM 测试。当前阶段不刷 SPI、不执行 saveenv、不执行 sf erase/write/update。
