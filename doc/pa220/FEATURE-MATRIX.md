# uBootKit → PA-220 功能适配矩阵

本表是项目级总清单，目的是保证 uBootKit 功能逐项得到结论。当前分类来自集成仓库 README、既有 PA-220 维护盘点、固定 PA-220 源码资料和已存在补丁；后续 P1/P2 需要进一步按 uBootKit 源码目录、命令注册和调用链逐项核对，不能把本表视为已经完成的逐函数审计。

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
| Web UI | 浏览器状态页、系统信息、网络设置 | 尚无 HTTP 管理服务 | 可适配-待实现 | 设计旧版网络栈 HTTP 服务与页面 |
| DHCP Client | 自动获取 U-Boot 网络地址 | 已配置 | 已配置 | MGT 实机验证 |
| DHCP Server | 给 PC/客户端分配地址 | 未实现 | 可适配-待实现 | 评估地址池、租约、冲突与报文处理 |
| DHCP 报文过滤 | 防止上级 DHCP 报文干扰 | uBootKit 具有平台定制逻辑 | 需研究 | 确认 MGT 直连/桥接场景是否需要 |
| Ping/DNS | 网络诊断 | 已配置相关网络栈能力 | 已配置 | 实机验证 |
| TFTP | 下载/上传镜像 | 已配置 | 已配置 | RAM 下载、文件长度和传输可靠性测试 |
| HTTP 下载 | wget/HTTP 获取文件 | 需核实 PA-220 网络栈的 HTTP 客户端支持 | 需研究 | 检查旧版 U-Boot 的 wget/HTTP 代码依赖 |
| HTTP 文件上传 | 浏览器上传文件 | 未实现 | 可适配-待实现 | 固定缓冲区、限长、接收超时、校验 |
| 镜像校验 | CRC、哈希、镜像头检查 | CRC32、SHA-1/MD5/SHA-256 等配置已存在 | 已配置 | 核验命令和算法实现 |
| eMMC/MMC | 设备识别、分区、读写 | MMC/eMMC 已配置 | 已配置 | 先只读识别和读回；写入需独立审批 |
| 文件系统 | FAT、EXT2/3/4 | 读取与部分写入命令已配置 | 已配置 | 逐项验证驱动与实际分区 |
| USB 存储 | XHCI、Mass Storage、文件访问 | DTS 定义两个 USB 3.0 控制器，配置已启用相关支持 | 需研究 | 检查供电、端口、枚举和存储实机状态 |
| SPI NOR | 探测、读、擦写 | SPI NOR/sf 已配置 | 需研究（高风险） | 只读 JEDEC/容量/芯片选择；写入功能默认关闭 |
| 环境变量 | 查看、设置、持久化 | SPI 环境主区 0xe00000、冗余区 0xe10000 | 已配置（写入危险） | 只读显示；禁止 RAM 测试期间 saveenv |
| 分区表 | GPT/EFI/DOS 识别 | 相关配置已存在 | 已配置 | 验证实际 eMMC 分区表 |
| 固件管理 | 识别、解析、安装、进度、读回校验 | 自动安装器未实现 | 可适配-待实现 | 先实现识别和 dry-run，再设计 eMMC 事务 |
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
