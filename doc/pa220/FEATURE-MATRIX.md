# uBootKit → PA-220 功能适配矩阵

本表是项目级总清单，目的是保证 uBootKit 功能逐项得到结论。P1 固定源码静态审计与 P2 基线/构建复核已完成；本表后续状态结合 P5-2C 的云端、Host 和实机证据更新。逐项审计结论仍须以对应 audit 报告为准；配置存在、源码实现、目标构建、RAM 启动和实机功能验证继续分层记录。

## 状态说明

- **已配置**：PA-220 固定源码中有相应配置/驱动/命令。
- **已实现-待验证**：PA-220 补丁或代码已存在，但构建/实机证据不足。
- **可适配-待实现**：预计可在 OCTEON/U-Boot 2013.07 环境实现。
- **需研究**：需先查明硬件、协议、内存或启动链边界。
- **不适用**：依赖 Qualcomm/IPQ 专有平台。
- **实机通过**：仅在附带可复现测试证据后使用。

## 功能矩阵

| 功能域 | uBootKit 能力/目标 | PA-220 现状 | 初步适配结论 | 后续工作 |
|---|---|---|---|---|
| 串口与 CLI | 命令行、命令帮助、脚本/菜单 | 串口 CLI、hush、bootmenu 已配置 | 已配置 | 验证交互和命令兼容性 |
| 板卡信息 | SoC、板型、内存、时钟、启动状态展示 | 既有 pa220 只读补丁 | 已实现-待验证 | 构建后验证字段与寄存器读数 |
| Web UI | 浏览器状态页、系统信息、网络设置 | P5-2C 已实现只读 HTTP 服务与五个 GET URI；实机 HTTP 8/8 PASS | **实机通过（只读范围）** | 后续仅在 RAM validator 完成后评估上传；无命令执行/写入接口 |
| DHCP Client | 自动获取 U-Boot 网络地址 | 已配置 | 已配置 | MGT 实机验证 |
| DHCP Server | 给 PC/客户端分配地址 | 未实现 | 可适配-待实现 | 评估地址池、租约、冲突与报文处理 |
| DHCP 报文过滤 | 防止上级 DHCP 报文干扰 | uBootKit 具有平台定制逻辑 | 需研究 | 确认 MGT 直连/桥接场景是否需要 |
| Ping/DNS | 网络诊断 | 已配置相关网络栈能力 | 已配置 | 实机验证 |
| TFTP | 下载/上传镜像 | PA-220 已有旧版实现；RAM store_block 无 __tmp_load pre-write guard，也无显式 syncw | 已配置，但安全 RAM 接收需加固 | 统一 runtime named-block validator；每 block memcpy 前检查；传输完成后执行 syncw 语义同步；确认 direct-flash 分支关闭 |
| HTTP 下载 | wget/HTTP 获取文件 | 需核实 PA-220 网络栈的 HTTP 客户端支持 | 需研究 | 检查旧版 U-Boot 的 wget/HTTP 代码依赖 |
| HTTP 文件上传 | 浏览器上传文件 | 未实现；uBootKit 参考实现使用 IPQ 固定地址，PA-220 同名 flush_cache 实际为全局 I/D-cache 操作 | 可适配-待实现 | 使用运行时 __tmp_load；Content-Length 全量预留；每个 chunk 写入前检查；完成后使用 PA-220 syncw 语义同步；禁止 IPQ 地址/API |
| 镜像校验 | CRC、哈希、镜像头检查 | CRC32、SHA-1/MD5/SHA-256 等配置已存在 | 已配置 | 核验命令和算法实现 |
| eMMC/MMC | 设备识别、分区、读写 | MMC/eMMC 已配置 | 已配置 | 先只读识别和读回；写入需独立审批 |
| 文件系统 | FAT、EXT2/3/4 | 读取与部分写入命令已配置 | 已配置 | 逐项验证驱动与实际分区 |
| USB 存储 | XHCI、Mass Storage、文件访问 | DTS 定义两个 USB 3.0 控制器，配置已启用相关支持 | 需研究 | 检查供电、端口、枚举和存储实机状态 |
| SPI NOR | 探测、读、擦写 | SPI NOR/sf 已配置 | 需研究（高风险） | 只读 JEDEC/容量/芯片选择；写入功能默认关闭 |
| 环境变量 | 查看、设置、持久化 | SPI 环境主区 0xe00000、冗余区 0xe10000 | 已配置（写入危险） | 只读显示；禁止 RAM 测试期间 saveenv |
| 分区表 | GPT/EFI/DOS 识别 | 相关配置已存在 | 已配置 | 验证实际 eMMC 分区表 |
| 固件管理 | 识别、解析、安装预览、进度、读回校验 | P7-1 新增只读 parser/`pa220 image preview`；OCTEON/uImage CRC、FIT/FDT bounds、ELF PT_LOAD bounds；目标映射 UNMAPPED；OCTEON 完整目标构建与实机 preview 待验证 | **实现中（dry-run only）** | 完成 OCTEON build 和 uImage 实机 preview；再取证 FIT 语义、SPI boot chain 与 eMMC GPT 映射；禁止写入 |
| 启动管理 | 正常/备用/恢复/RAM 启动 | 有 linux_ram 等现有启动脚本 | 需研究 | 查清内核选择、回退机制和环境依赖 |
| EEPROM | 读取 MAC/板卡数据 | 板载 TLV EEPROM，MAC 读取逻辑存在 | 已配置 | 只读核对内容与 MAC 地址映射 |
| I2C/RTC/GPIO | 外设状态与控制 | 配置中存在相应支持 | 需研究 | 分别验证总线、地址、寄存器和安全副作用 |
| CPLD | 状态、LED、硬件控制 | cpld display/read 板级命令已实现 | 已实现-待验证 | 只读验证；写寄存器 d 可能切换 SPI 芯片/重启 |
| LED/传感器 | 前面板 LED、温度状态 | 板级资料和现有代码包含部分支持 | 需研究 | 确认寄存器语义、阈值与实机行为 |
| 9008/MBN/bootipq | Qualcomm 紧急下载/启动 | 无对应 Qualcomm 硬件 | 不适用 | 不移植 |
| IPQ PPE/GMAC/SoC 控制 | Qualcomm 网络加速和 SoC 专用功能 | OCTEON 架构不同 | 不适用 | 使用 OCTEON 自身网络驱动，不照搬寄存器代码 |

## 必须补全的逐项审计

本表目前是功能域级清单。后续审计必须为每一项 uBootKit 功能记录：
1. 原始文件路径、命令/函数入口和依赖模块。
2. 关键数据结构、内存分配和网络收包模型。
3. Qualcomm 专有依赖及可替换的 U-Boot 通用接口。
4. PA-220 对应配置、驱动、硬件寄存器和现有代码。
5. 适配结论、补丁编号、编译结果和实机验证证据。

任何无法证明的适配结论都应保留为“需研究”，而不是标记完成。

## P1-A1 页面与 URI 审计补充（2026-11-18）

证据：`audit/P1-A1-PAGE-URI-CALLCHAIN.md`，uBootKit 固定 commit `9ce315735ac88fd0a7c7697bc527a8616ed0a972`，PA-220 固定 commit `801717ba6a8a59ed2cfd53bf3e1b19841fecd2e8`。

- Web 页面统一通过 `/main.js` 发起业务请求；静态页面/URI 分发和 handler 注册框架具有结构复用价值，但 PA-220 尚无 HTTPD/TCP 服务实现，不能标记为已实现。
- `sysinfo`：JSON/页面格式可借鉴；SMEM、QCA MMC host、IPQ API 硬件采集必须以 OCTEON/PA-220 接口重写。
- `network`：PA-220 有旧网络栈及环境变量基础接口；uBootKit handler 修改环境后调用 `saveenv()`，不得直接移植。PA-220 环境保存在 SPI NOR。
- `env`：PA-220 有 getenv/env_import 等基础接口；所有 set/unset/reset/restore 路径均涉及 `saveenv()`，第一版 Web 仅允许只读展示。
- `syslog`：uBootKit 依赖 `gd->console_out` membuff；PA-220 对应缓冲实现 UNKNOWN，需进一步检查。
- `backup/mibib/mac/webterm/upgrade`：路由和主要副作用入口已定位；逐模块调用链仍按 P1-A2 顺序审计，upgrade 最后完成。
- 配置启用、编译成功、实机验证均 NOT VERIFIED；本批只完成源码审计。


## P1-A2 第一组 Web 业务模块审计（2026-11-18）

证据：`audit/P1-A2-READONLY-WEB-MODULES.md`。本批只完成 `sysinfo.c`、`network.c`、`env.c`、`syslog.c)，不是 P1-A2 全部完成。

- **sysinfo**：通用 JSON/页面可借鉴；SMEM、IPQ API、QCA MMC host、NAND 数据源不能直接用于 PA-220，硬件采集需重写。PA-220 OCTEON SPI/MMC/GPT 配置存在，但未构建/实测。
- **network**：PA-220 旧网络栈通过 NetInitLoop 从环境读取 ipaddr/serverip；Web 运行期修改环境不等于更新当前网络状态。网络写 handler 调用 saveenv，PA-220 common/env_sf.c 会擦写 SPI 环境区，禁止照搬。
- **env**：PA-220 有 getenv/getenv_default/env_import 和 hexport_r 基础；全量只读导出需重写并脱敏。所有写接口调用 saveenv；env_restore 还存在上传长度检查不足问题。
- **syslog**：uBootKit 依赖 gd->console_out/membuff；PA-220 global_data/console 检查到的是 precon 缓冲，不等价于运行期日志环形缓冲，后端需重写。
- 四模块均为 uBootKit 源码存在且由 CONFIG_HTTPD 的 failsafe 构建路径编入；目标 defconfig 是否实际启用、构建结果和实机功能均 NOT VERIFIED。
- P1-A2 尚余 misc、webterm、backup、mibib、mac，upgrade 仍按要求最后审计。


## P1-A2 第二组 Web 业务模块审计（2026-11-18）

证据：`audit/P1-A2-MISC-WEBTERM.md`。本批完成 `misc.c`、`webterm.c)，不代表 P1-A2 全部结束。

- **misc**：静态页面分发、gzip 资源处理和版本信息可重写；HTTPD response 生命周期需适配 PA-220 旧版环境。/reboot 是复位动作，Web 初版不提供。
- **webterm**：/webterm/exec 通过 run_command 执行任意 U-Boot 命令，可能间接触发 saveenv、SPI/eMMC/文件系统写入；PA-220 Web 初版禁止。cmdlist 会暴露命令面，初版不提供。upload 依赖 QCA 固件辅助接口，且 set_file_info_env 副作用 UNKNOWN，不直接移植。
- misc/webterm 均由 CONFIG_HTTPD 的 failsafe 构建路径编入；实际目标配置启用、编译和实机状态 NOT VERIFIED。
- P1-A2 尚余 backup.c、mibib.c、mac.c；upgrade.c 仍按要求最后审计。


## P1-A2 第三组存储/MAC 模块审计（2026-11-18）

证据：`audit/P1-A2-BACKUP-MIBIB-MAC.md`。本批完成 `backup.c`、`mibib.c`、`mac.c)，upgrade.c 尚未审计。

- **backup**：uBootKit 通过 SPI/NAND/MMC/SMEM 读取并导出数据；PA-220 可研究 eMMC GPT 和经硬件选择确认的 SPI NOR 只读备份。无 NAND/SMEM 对应机制。需修复整数溢出、范围、短读和设备探测边界；默认禁止无认证全盘导出。
- **mibib**：依赖 Qualcomm 9008/SMEM/MIBIB/LED；本文件主要解析上传文件并更新运行时 SMEM 分区状态，不是直接 flash 写入。PA-220 使用 GPT/板级布局，不适用。
- **mac**：CONFIG_FAILSAFE_MAC_MANAGEMENT 默认关闭。uBootKit MAC 写路径会触发 SPI erase/write、NAND erase/write 或 MMC block_write。PA-220 MAC 来源为 TLV EEPROM/OCTEON board API；仅适配只读展示，任何 EEPROM 写入另行审查和授权。
- 本批未执行存储写入、CPLD 写入、编译或实机验证。
- P1-A2 仅剩 upgrade.c，仍按要求最后审计；不得将本批结论视为 P1-A2 完成。


## P1-A2 最后一组：upgrade 与持久化写入审计（2026-11-18）

证据：`audit/P1-A2-UPGRADE-PERSISTENT-WRITE-AUDIT.md`。按要求将 upgrade 放在 P1-A2 最后审计；至此用户指定的全部业务模块均完成源码静态审计。

- /upload 只执行镜像识别/验证；/result 才调用 failsafe_write_image()，并通过 run_command() 执行写命令。
- 已逐项记录 firmware、UBOOT、ART、CDT、PTABLE、SIMG 的 SPI/NAND/eMMC/分区写路径，以及 firmware 更新附带的 BOOTCONFIG/BOOTCONFIG1 隐式写入。
- INITRAMFS 不调用 failsafe_write_image()，而在 HTTP 服务退出后 boot_from_mem()；不直接写持久化存储，但仍需 PA-220 专用 FIT/地址/启动路径审查。
- QCA flash/MIBIB/ART/CDT/SMEM/bootconfig 机制不适用于 PA-220；PA-220 GPT/eMMC/SPI 写命令具备破坏性，不移植 Web 刷写入口。
- check_fw_type() 接收 size 却不据此限制 magic 读取范围；上传验证不等同于签名验证或安全启动。
- P1-A2 源码审计范围已覆盖 sysinfo/network/env/syslog/misc/webterm/backup/mibib/mac/upgrade；仍未编译、未实机验证，未执行任何持久化写入。


## P1-A3 第一批网络服务审计（2026-11-18）

证据：`audit/P1-A3-BOOTP-DHCP.md`。P1-A3 整体仍在进行。

- **BOOTP/DHCP Client**：PA-220 固定 U-Boot 已有旧版实现，CONFIG_CMD_NET/CONFIG_CMD_DHCP 配置入口存在；无需移植 uBootKit 的 bootp.c，后续只需核对最终配置和实机网络行为。
- **DHCP Server**：uBootKit 有 CONFIG_DHCPD 服务端；PA-220 没有对应服务端对象。其 UDP handler、NetLoop 调度、地址池和租约机制需在 PA-220 旧网络栈上重写/验证。
- DHCP Server 当前最多 8 个 RAM 租约槽，无租约到期回收，池耗尽回退可能重复分配；不能原样移植。
- 编译和实机状态 NOT VERIFIED；TFTP/WGET/TelnetD/eth/net/net_abort 尚未完成审计。


## P1-A3 第二批网络服务审计（2026-11-18）

证据：`audit/P1-A3-TFTP-WGET-TELNET.md`。P1-A3 仍在进行。

- **TFTP GET/PUT/Server**：PA-220 已有旧版实现和 CONFIG_CMD_TFTPPUT/CONFIG_CMD_TFTPSRV；无需移植新版 tftp.c。PA-220 store_block 默认写 RAM，但若 CONFIG_SYS_DIRECT_FLASH_TFTP 启用会调用 flash_write；固定板级头未发现该宏，最终 .config 未验证。PA-220 TFTP load_addr 缺少 uBootKit 新版的 IPQ 内存边界保护，需补板级内存范围检查。
- **WGET**：PA-220 无 wget.o/TCP client，需独立实现；uBootKit 版本只支持 HTTP，不支持 TLS/IPv6/chunked。
- **TelnetD**：PA-220 无对应 TCP server；uBootKit TelnetD 无认证并执行任意 run_command，PA-220 不移植。
- TelnetD 自动启动检查变量为 telnet_en（默认 true），而命令帮助写 telnetd_enable，源码文档与实现不一致。
- TFTP/WGET/TelnetD 均未在 PA-220 编译或实机验证。


## P1-A3 第三批网络栈/网卡/中止路径审计（2026-11-18）

证据：`audit/P1-A3-ETH-NET-ABORT.md`。至此 P1-A3 网络服务源码静态审计完成。

- **Ethernet**：PA-220 保留 OCTEON eth_device driver，CONFIG_OCTEON_ETH、QSGMII/AGL 配置和 CVMX IPD/PIP/PKO 实现存在；不移植 IPQ Ethernet/PPE。
- **NetLoop**：PA-220 已有旧版 Ethernet/ARP/IP/ICMP/UDP/NetLoop；net_process_received_packet() 没有 TCP 分支。HTTPD/WGET/TelnetD 仍需 TCP transport + 收包/调度层重写。
- **DHCP Client/TFTP**：PA-220 已有旧版等价实现；DHCP Server 需重写并统一 NetLoop 调度。
- **net_abort**：uBootKit 通过 autoboot.c 调用，默认 net_abort_en=true；UDP 37541 收到 UBOOT:ABORT 后可中断 autoboot 并进入 httpd/cli_loop。无认证，PA-220 不直接移植。
- 本阶段所有新增网络服务均未在 PA-220 编译或实机验证。


## P1-A4 第一批存储后端审计（2026-11-18）

证据：`audit/P1-A4-EMMC-SPI-CORE.md`。P1-A4 整体进行中。

- **eMMC**：PA-220 使用 OCTEON 专用 octeon_mmc.c，提供 block_read/block_write/block_erase；不要替换为 QCA MMC/SDHCI。mmc_bwrite 有容量检查但存在加法溢出审查点；mmc_berase 缺少容量和零长度检查，属于高风险路径。
- **SPI NOR**：PA-220 启用 OCTEON SPI、SPI_FLASH、Winbond W25Q128、CONFIG_CMD_SF；板级注释明确 intended read-only。sf write/erase/update 命令虽存在，项目仍禁止执行。实际 bus/CS 与双 Flash/CPLD 选择关系尚未实测。
- **EXT_CSD**：CONFIG_OCTEON_MMC_NO_RST_N=1 明确保护 RST_n_FUNCTION OTP；必须保留。boot/RPMB 配置类命令需独立审查。
- 未执行任何 mmc/SF/saveenv/CPLD 写操作；GPT/文件系统/环境后端尚未完成。


## P1-A4 第二批 GPT/文件系统审计（2026-11-18）

证据：`audit/P1-A4-GPT-FILESYSTEM.md`。P1-A4 整体仍在进行。

- **GPT**：PA-220 启用 EFI_PARTITION/PARTITION_UUIDS/CMD_GPT；旧版 gpt write 会经 gpt_restore()/write_gpt_table() 对 eMMC LBA0、主/备 GPT header 和 entry array 执行 block_write。gpt_mmc_default() 忽略 gpt_restore() 返回值；旧命令无 verify 子命令。只读 GPT 检查可复用，写入不开放。
- **EXT4**：PA-220 有 ext4load/ext4ls/ext4write 源码入口；ext4fs_write 会更新文件数据、inode、bitmap、superblock、group descriptors 和 journal。覆盖文件先删除旧文件，失败可能丢失旧文件。
- **FAT**：PA-220 有 fatload/fatls/fatwrite；fatwrite 更新文件数据、FAT 表和目录项。新版/旧版 file_fat_write API 签名不同。
- 文件系统库最终 .config 是否完整启用仍待构建确认；未执行 GPT/EXT4/FAT 写入。


## P1-A4 第三批环境变量/SPI 后端审计（2026-11-18）

证据：`audit/P1-A4-ENV-SPI-BACKEND.md`。P1-A4 仍在进行，固件分区映射/bootloader update 尚待审计。

- PA-220 环境由 CONFIG_ENV_IS_IN_SPI_FLASH 选择 common/env_sf.c，主/冗余偏移 0xE00000/0xE10000，环境大小 8 KiB，sector 64 KiB，bus0/CS0。
- 冗余 saveenv() 每次擦除一个 64 KiB SPI NOR sector，写 8 KiB 环境结构、恢复 sector 剩余 56 KiB，并更新旧副本 flags；这是真实 SPI 持久化写入。
- 启动 env_relocate_spec() 读取两份环境并按 CRC/flags 选择，不在正常读取路径写 SPI。
- setenv()/env_import() 仅改变运行时 env；saveenv() 才持久化。uBootKit network/env handler 调用 saveenv 的写接口不能直接移植。
- PA-220 include/environment.h 未发现 uBootKit 同名公开 env_export() API；只读导出需用 PA 旧版 hexport_r() 封装。
- 未执行 saveenv/SF 写入；实际 SPI bus0/CS0 物理芯片映射仍 NOT VERIFIED。


## P1-A4 第四批 OCTEON Bootloader 更新路径审计（2026-11-18）

证据：`audit/P1-A4-BOOTLOADER-UPDATE.md`。

- PA-220 配置启用 CONFIG_CMD_OCTEON_BOOTLOADER_UPDATE，Makefile 编入 cmd_octeon_bootloader.o。
- CONFIG_SYS_NO_FLASH 排除并行 NOR updater；当前固定 PA-220 配置链未发现 CONFIG_OCTEON_SPI_BOOT_END，因此通用 OCTEON SPI bootloader updater 分支未启用。
- CONFIG_OCTEON_MMC 启用 eMMC updater：Stage1 可 block_write 覆盖 LBA0–15；failsafe Stage2 可写 LBA0x10 起始区域。当前设备是否使用该 eMMC boot chain 尚未确认，不得运行。
- Stage1 路径无上传长度至少 8 KiB 的检查；短镜像可能越界读取 RAM。SPI updater 未启用分支另有 image_map 未初始化即 memset 的源码缺陷。
- PA-220 当前 Stage3 位于 SPI 启动链，由原厂前级加载；QCA APPBL/ART/CDT/SMEM 和通用 Octeon eMMC Stage1/2 不能直接映射。
- 未执行 bootloaderupdate/mmc write/erase/sf write/erase/update/saveenv。


## P1-A4 第五批 USB 存储及介质边界审计（2026-11-18）

证据：`audit/P1-A4-USB-STORAGE.md`。至此 P1-A4 存储/分区/文件系统/环境后端/固件更新源码审计完成。

- PA-220 配置启用 USB host/mass-storage 命令；usb read 读取到 RAM，usb write 经 block_write 写入外接 USB 存储。
- USB 外接介质与内部 eMMC/SPI NOR 分开处理；仍属于持久化介质，Web 不开放通用 usb write。
- CONFIG_CMD_OCTEON_ERASEENV 受 CONFIG_ENV_IS_IN_FLASH 条件控制；PA-220 使用 CONFIG_ENV_IS_IN_SPI_FLASH，该 parallel-NOR eraseenv 命令不属于当前 PA 配置路径。
- P1-A4 所有写入入口仅作源码追踪，没有执行；编译和实机状态 NOT VERIFIED。


## P1-A5 第一批 IPQ API/PA 板级硬件接口审计（2026-11-18）

证据：`audit/P1-A5-IPQ-PA-HARDWARE-API.md`。

- IPQ ipq_api.c 包含 SMEM/MIBIB/flash detect、GPIO/LED/button、9008 recovery、内存范围检查和 board info；PA-220 无 QCA SMEM/MIBIB，使用 eMMC GPT、TLV EEPROM、OCTEON CVMX GPIO 和 CPLD，不能直接移植。
- common/main.c 在 CONFIG_HTTPD 下 autoboot 前调用 do_network_check()/do_httpd_check()；do_network_check() 可能 setenv 后 saveenv，failsafe_env_exists() 也可能 saveenv。不能将 CONFIG_HTTPD 启动钩子整体移植到 PA-220。
- PA-220 现有板级 CPLD 操作：reg d bit0 pulse（normal-flash bootloader watchdog keepalive）、reg5 bit2 PHY reset、reg6/reg7 LED；保留现有初始化，不新增写操作。
- PA-220 TLV EEPROM 用于 board descriptor、MAC、clock/DDR clock；只读信息可研究复用。


## P1-A5 第二批 QLM/MDIO/PHY/Bootbus 时序审计（2026-11-18）

证据：`audit/P1-A5-QLM-MDIO-PHY-BOOTBUS.md`。

- PA-220 DTS 描述 8 个 Marvell 88E1680 PHY（MDIO 8–15）和一个 88E1512（MDIO 1），由 OCTEON PHY 初始化代码按 marvell,reg-init 执行 MDIO 寄存器配置。
- PA-220 board_configure_qlms() 存在，但固定源码未找到调用点；实际 DLM0 QSGMII/QSGMII 配置路径 NOT VERIFIED。
- board_prepare_os_handoff() 由 octeon_bootloader_shutdown() 调用；board_enter_cli() 由 main_loop() 调用。CPLD reg5/reg7 的写入时序已确认。
- octeon_boot_bus_board_post_init() 在 octeon_boot_bus_late_init() 内被调用，但固定源码中 late_init 的调用点尚未找到；reg d watchdog pulse 可达性 NOT VERIFIED。
- 88x3140 PHY SPI Flash 编程命令受 CONFIG_PHY_MARVELL_FLASH_PGM 控制，PA-220 配置未定义该开关，且板上 PHY 为 88E1680/88E1512，不适用。
- USB power GPIO2/GPIO3 在 U-Boot early_board_init() 被拉低；USB Mass Storage 命令源码存在不等于 U-Boot 阶段端口可用。


## P1-A5 第三批 OCTEON Watchdog/OS handoff 审计（2026-11-18）

证据：`audit/P1-A5-WATCHDOG-HANDOFF.md`。

- PA-220 CONFIG_HW_WATCHDOG 已启用；Makefile 编入 octeon_wd.o 和 cmd_octeon_wd.o。
- watchdog_enable/watchdog_timeout 环境 callback 在变量 create/overwrite 时立即调用 hw_watchdog_start；delete 时 disable。callback 不检查变量值，设为字符串 "0" 也会启动。
- octwd test 会故意不喂狗并等待 NMI，不得开放 Web。
- octeon_bootloader_shutdown() 在 OS handoff 调用 hw_watchdog_disable()，然后关闭网络/USB；该函数关闭 core0 watchdog，未调用 disable_all。
- OCTEON CIU watchdog 与 CPLD reg d flash-failover watchdog 是两套独立硬件。


## P1-A5 第四批板卡信息/DRAM 内存边界审计（2026-11-18）

证据：`audit/P1-A5-BOARD-INFO-MEMORY.md`。

- PA-220 board identity 来自 TLV EEPROM，CVMX sysinfo 提供 system_dram_size、cpu_clock_hz、board_type/revision/serial 等字段；checkboard() 默认 weak stub 不能作为完整信息接口。
- Stage2 已完成 DRAM 初始化；RAM-resident Stage3 从 dram_size_mbytes 环境变量接收 DRAM 容量，gd->ram_size 最终由该值/已检测值设置；默认 8192 MiB 不是实机容量证据。
- PA-220 内存配置包含 TEXT_BASE=0xC0000000、SDRAM_BASE=0x80000000、LOAD_ADDR 默认0x20000000、MALLOC_LEN=12MiB、BOOTMAPSZ=1MiB、mtest 0x22000000–0x2A000000；项目既有 TFTP 地址 0x81000000 与默认 LOAD_ADDR 不同。
- PA-220 安全 RAM 上传区尚未完成地址图，必须按 MIPS64/n32、cached/uncached alias、Stage2 handoff、U-Boot/heap/stack/GD/FDT/bootmap/mtest 等边界重写检查；未确认前不开放上传。


## P1-A5 ELF64 / Cache 一致性审计补充（2026-11-18）

证据：`audit/P1-A5-CALLCHAIN-RAM-CONVERGENCE.md` 第 23～26 节，以及 `P1-COMPLETION-AND-P2-ORDER.md`。

- **ELF/bootelf**：PA-220 有 OCTEON 专用 `load_elf64_image()`，编译入口为 `CONFIG_CMD_OCTEON_ELF`。当前 `valid_elf_image()` 不接收文件长度；ELF64 PT_LOAD 和 section fallback 都缺少完整 source payload / program-header / destination-range 检查。little-endian 8-byte 转换循环对非 8-byte 整数倍长度存在尾部越界风险。结论：已有实现，**可适配-待加固**；所有 segment 必须在写入前验证到运行时 `__tmp_load`。
- **PA-220 Cache API**：`flush_cache()` 实际执行全局同步并失效 I/D-cache；`flush_dcache_range()` 实际执行 `syncw`；`invalidate_dcache_range()` 是空实现。不可按 API 名称推断为逐行范围 flush。
- **bootm**：镜像 load/decompress 完成后调用 `flush_cache()`；OS handoff hook 本身不执行另一个 cache-range 操作。
- **bootelf**：loader 每个 segment/section 写入后调用 `flush_cache()`；`do_bootelf_exec()` 本身不再 flush，也不走 `arch_preboot_os()` 临时块释放路径。
- **统一 RAM validator**：生产函数 octeon_load_range_check(uint64_t addr, uint64_t len) 已实现；Host Harness 32 scenarios / 121 assertions / 0 failures（含 ASan/UBSan）。运行时 __tmp_load descriptor 为唯一写入窗口，检查 globals 一致性、U-Boot reservation、已知 named reserved blocks、地址 alias 和溢出。OCTEON 目标编译/完整链接待 Actions；TFTP/HTTP/ELF 写入调用点尚未接入，全部保持 BLOCKED。
- TFTP/HTTP payload 完成后的数据同步与 ELF/bootm 执行代码的 I/D-cache 维护必须区分；不得为每个网络 chunk 调用 PA-220 的全局 `flush_cache()`。


## P2 构建复核进度（2026-11-18）

证据：`audit/P2-1-BUILD-INPUT-LOCK.md`。

| 子项 | 内容 | 状态 |
|---|---|---|
| P2-1 | 集成分支/HEAD、固定源码 commit、uBootKit 审计 commit、构建脚本、补丁、PA-220 原生入口、配置入口和产物命名 | **已完成 GitHub 静态核查**；本地 apply check 留待干净构建 |
| P2-2 | Linux 构建主机、宿主机依赖、OCTEON SDK GCC/binutils 完整版本和路径 | **已完成环境核查与最小交叉编译/链接验证**；完整 U-Boot 构建仍待 P2-3/P2-4 |
| P2-3 | 全新固定源码 checkout、补丁应用、完整构建、配置快照、map/ELF、BIN 和 SHA256 | **构建成功；主要产物静态核查完成，仍有差异项待收敛** |
| P2-4 | OCTEON ELF64 loader、Cache API、named-block API 等关键目标对象/符号实际编入及链接/重定位边界 | 待执行 |

状态纪律：旧构建日志或配置宏不作为本轮构建成功证据；P2-2/P2-3/P2-4 未完成前不得标记 P2 完成。


## P2-3 构建证据补充（2026-11-18）

PA-220 固定源码在独立干净 checkout 上完成完整构建，退出码为 0。新增只读 `pa220` 命令已由 ELF 符号表、linker map 和 BIN strings 证明进入构建产物。此结果只将相关功能推进到“已编译-待 RAM/实机验证”，不代表命令已在设备上运行或 CPLD 读数已验证。

- 板卡/内存/时钟/网络/CPLD 只读诊断：**已编译-待验证**；构建证据见 `audit/P2-3-CLEAN-BUILD-ARTIFACT-AUDIT.md`。
- EEPROM/TLV、named block、Cache 一致性、ELF64 loader：**尚未完成最终对象编入核查或实机只读验证**；不得由配置宏或 P1 静态审计直接标记为已编译/实机通过。
- 所有存储写入路径继续保持禁止；P2-3 未执行设备写入。


## P5-1 原生 TCP / HTTP 适配审计（2026-11-20）

专项证据：`audit/P5-1-PA220-NATIVE-TCP-HTTP-ADAPTATION.md`。

- PA-220 固定 U-Boot 2013.07 的 `NetReceive()` 目前只处理 ICMP/UDP，IPv4 TCP protocol 6 被丢弃；`net/Makefile` 无 TCP/HTTPD 对象。
- 现有 `NetLoop()` 是阻塞式协议状态机，使用单一 UDP/ARP handler 和单一 timeout callback；不应直接让 HTTPD 占用/覆盖这些全局状态。
- Ethernet `eth_rx()/eth_send()/eth_init()/eth_halt()` 与原始帧组装接口可复用。TCP 必须新增独立分派入口、TCP 校验和/连接状态/重传管理，并严格遵守 RX buffer 只在当前收包回调内有效的生命周期。
- 建议以显式前台 `pa220 web` 命令管理服务，独立轮询 `eth_rx()`、TCP 定时处理和 watchdog；Ctrl-C 与统一 cleanup 负责退出，禁止自动进入 Web 或后台驻留。
- 首版只允许有界 GET/HEAD、固定 URI allowlist、静态只读 handler；禁止任意命令执行、环境写入、saveenv、存储/CPLD 写入、上传、安装和重启。
- 本批仅完成静态源码审计与设计收敛；TCP/HTTP 实现、编译、RAM 启动和 MGT 实机访问均 NOT VERIFIED。


## P5-1A TCP 核心实现进度（2026-11-20）

- **TCP 报文基础层：已提交，待编译/测试。** 新增 `u-boot/include/net/tcp.h` 与 `u-boot/net/tcp.c`，实现无对齐假设的 TCP 固定头解析、data offset/option TLV 边界检查、MSS 选项识别、IPv4 TCP 伪首部校验和计算/验证入口，以及 32 位序列号比较。
- **TCP 完整连接核心：未实现。** 尚无单连接状态机、SYN/SYN-ACK/ACK、数据接收窗口/缓冲、发送 ACK、FIN/RST 状态处理、超时/重传及发包能力。
- 本提交暂不接入 `NetReceive()`，不修改 `net.c`、`net.h`、`net/Makefile`、UDP/ARP handler 或全局 timeout；未实现 HTTP/Web。
- 进度报告：`audit/P5-1A-TCP-CORE-PROGRESS.md`。编译、单元测试、RAM 启动和实机验证均 NOT VERIFIED。


## P5-1A.2 TCP 单连接状态机（2026-11-20）

- 新增单连接状态机 API：监听、SYN/SYN-ACK/最终 ACK、序列号与累计 ACK、固定接收窗口/缓冲区、有限发送与重传、FIN/RST、空闲超时和 TIME-WAIT 清理；实现位于 `u-boot/net/tcp.c`，接口位于 `u-boot/include/net/tcp.h`。
- 输出回调与调用方毫秒时钟注入；不持有 RX packet 指针，不使用全局 NetSetTimeout，不接入 NetReceive。
- 目前仅单一 outstanding 发送范围、按序接收；SACK/乱序重组/零窗口探测/拥塞控制/独立窗口更新 ACK 尚未实现。
- 目标工具链编译、单元测试、链接、RAM 启动及实机网络验证均 NOT VERIFIED；本提交不代表完整 TCP 核心完成。
- 详细接口、状态与限制见 `audit/P5-1A-TCP-CORE-PROGRESS.md`。


## P5-1A.3 验证状态（2026-11-20）

- 当前远程 HEAD 已重新核对为 P5-1A.2 检查点 `5c33f6c4496cdc08a1cab66facf2c03b3bcc6754`；相对该检查点无新增提交。
- 用户 Debian 13.7 构建主机及 Cavium GCC 4.7.0 / SDK Build 52 已由此前 P2/P3 实际编译记录确认存在，工具链路径为 `/home/debian/OCTEON-SDK/tools-gcc-4.7`。此前 P3 已在该主机完整构建成功。本轮尚未执行 TCP 对象编译；ChatGPT 当前临时容器不能直接访问用户主机，不能将容器内路径不存在误判为用户构建环境不存在。后续通过用户在既有主机执行命令完成验证。
- 实际 TCP 源码已静态审查；宿主机测试桩尚未建立，协议测试执行数为 0（未执行）。发现 FIN 后 CLOSE-WAIT 仍可能接受后续数据、TIME-WAIT 缺少重复 FIN 专门处理两个待复现风险，尚未修复或宣称为已复现缺陷。
- P5-1A 保持“已实现、待目标编译及协议验证”；不得进入 P5-1B，直至目标编译、协议测试及必要链接检查完成。


## P5-2C 实机验收状态（2026-11-23）

正式验收：`reports/pa220/P5-2C/ACCEPTANCE.md`；原始网络证据与 provenance 说明：`reports/pa220/P5-2C/physical-evidence/`。

| 项目 | 状态 | 证据/边界 |
|---|---|---|
| TCP core Host focused regression | PASS | 76/76 assertions；历史 P5-1A 验证记录 |
| TCP core OCTEON object / P5-1B integrated final link | PASS | Cavium GCC 4.7.0 / SDK 5.1.0；N32；完整 PA-220 link |
| P5-2C Host Harness | PASS | 29 scenarios / 161 assertions / 0 failures |
| 前台 `pa220 web` lifecycle | PASS | 显式启动、前台服务；不自动接管 autoboot |
| HTTP GET allowlist | PASS | `/api/board`、`/`、`/api/status`、`/api/memory`、`/api/network` 均 200 |
| 未知 URI / 不支持方法 | PASS | GET `/not-found` 为 404；POST `/api/status` 为 405 |
| 物理 HTTP 客户端 | PASS | 8/8 cases；每个连接单请求后 EOF/close |
| PCAPNG packet evidence | PASS | 可见 200/404/405、FIN/ACK、RST+ACK 与后续新连接 200 |
| RST 对部分 HTTP 请求的中断 | NOT DEMONSTRATED | abort 流程先 FIN（无 HTTP payload），服务器回 400 后客户端 RST+ACK |
| 原始 UART transcript | NOT AVAILABLE | `pa220-serial.log` 是明确标注的会话记录重建件 |
| 上传/文件接收 | BLOCKED | 统一 `__tmp_load` 写入前 validator 尚未作为 P6 完成 |

本次实机使用 RAM 加载 Stage-3 镜像；没有 saveenv、CPLD 写、SPI/eMMC/NAND 写、复位或重启。BIN 不得刷写。镜像 SHA-256 是用户在 Windows TFTP 主机计算并与云端期望值一致；GitHub 上传副本未在本次审查中独立重新计算 SHA-256。


## P4-2 validator 实施状态（2026-11-24）

- 生产 validator 与 Host Harness 已加入集成仓库；Host 32/121/0 PASS。
- 当前不是完整上传功能：TFTP store_block()、HTTP payload/chunk、ELF64 PT_LOAD/section 写入尚未接入 validator。
- OCTEON 目标对象、N32 relocatable link、完整 U-Boot final link 和符号/map 检查待 Actions。
- 在目标验证及调用点接入/回归完成前，TFTP 接收、HTTP 上传和任意 ELF segment 写入仍为 BLOCKED。


## P4-2 / P6 状态覆盖（2026-11-24）

本节为当前状态权威覆盖；较早的 32/121 Host 和目标构建待执行记录属于历史进度。

| 功能/门 | 当前状态 | 证据边界 |
|---|---|---|
| 统一 RAM validator | **已实现；Host PASS** | 42 scenarios / 161 assertions / 0 failures；GCC + ASan/UBSan |
| TFTP DATA pre-write guard | **已实现；Host PASS** | 14 scenarios / 64 assertions / 0 failures；测试导入生成器中的生产函数模板，非完整 TFTP 会话 |
| OCTEON validator object / N32 link | **PASS** | Cavium SDK 5.1.0；Actions #45375848020 |
| TFTP `tftp.o` / `libnet.o` 集成 | **编译/链接 PASS** | 不代表网络接收成功 |
| 完整 PA-220 U-Boot final link | **PASS** | ELF/map/System.map 符号检查 |
| TFTP patch generator idempotence | **PASS** | Actions #45375848020 |
| live `__tmp_load` descriptor | **NOT RUN** | 设备报告断电 |
| TFTP GET 实际接收 | **NOT RUN / BLOCKED** | 等待只读 descriptor 预检及 P6 完整文件生命周期软件门 |
| TFTP Server/WRQ | **NOT RUN** | 不属于首轮 GET 验收 |
| TFTP PUT | **NOT RUN** | 本地 RAM source read 不属于 receive guard 覆盖范围 |
| HTTP upload | **BLOCKED** | HTTP 保持只读 |
| ELF PT_LOAD/section 写入保护 | **BLOCKED** | 尚无独立 destination/source validator 集成验收 |
| DHCP Client / Ping 实机 | **NOT RUN** | P4 遗留项 |

Actions #45346000355、#45374041373、#45375848020 均成功；最新 artifact ID `14910393863`。详细证据与 P6-0 源码审查见 `reports/pa220/P4-2/IMPLEMENTATION.md` 和 `reports/pa220/P6-0-SOURCE-REVIEW.md`。


## P7-1 只读镜像解析进度（2026-11-27）

- 生产入口：`pa220 image preview`，只解析有效 `fileaddr/filesize` RAM reservation，不调用加载、启动或存储写入路径。
- 当前格式范围：OCTEON bootloader header、legacy uImage、FIT/FDT header bounds、ELF32/ELF64 PT_LOAD bounds。
- Host GCC 与 ASan/UBSan 测试通过；OCTEON 完整目标构建和实机 preview 尚待验收。
- 所有安装目标当前为 UNMAPPED；不得借用 IPQ/QCA 分区名称。P7 整体未完成。
