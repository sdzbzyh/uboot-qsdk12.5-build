# P1-A1：Web 页面 URL 与 URI handler 调用链审计（续）

- 状态：**本批源码静态审计完成；未移植、未编译、未实机验证**
- 日期：2026-11-18
- uBootKit：chenxin527/uboot-qsdk12.5-build@9ce315735ac88fd0a7c7697bc527a8616ed0a972
- PA-220：sdzbzyh/pa220-openwrt@801717ba6a8a59ed2cfd53bf3e1b19841fecd2e8
- 文档分支：feature/pa220-ubootkit-web
- 前置报告：P1-A1-HTTP-TCP-WEB.md

本报告只补齐页面/URI/handler 映射，不重复 HTTPD/TCP 核心实现审计。

## 1. 页面资源加载

failsafe/fsdata/index.html 引用 /style.css、/main.js，并调用 appInit('sysinfo')。抽查的 network.html、env.html、syslog.html 同样引用统一的 /main.js，分别调用 appInit('network')、appInit('env')、appInit('syslog')。页面业务逻辑集中在 failsafe/fsdata/main.js（8,042 行），不存在与这些页面一一对应的独立 JS 文件。

main.js 的 SidebarManager 定义页面路径：
- 首页：/、/index.html、/cgi-bin/luci、/cgi-bin/luci/
- 信息/日志：/syslog.html
- 更新/救援：/firmware.html、/art.html、/cdt.html、/ptable.html、/simg.html、/uboot.html、/initramfs.html、/mibib.html
- 系统：/settings.html、/network.html、/mac.html、/env.html、/backup.html、/webterm.html
- 重启：/reboot.html

## 2. 页面 URI 注册关系

注册总入口为 failsafe/failsafe.c：
- register_uri_handlers_funcs[] 按序调用各模块注册函数。
- start_web_failsafe() 创建 80 端口 HTTP 实例，再遍历注册表。
- failsafe/modules/modules.h 声明注册函数。
- failsafe/Makefile 在非 SPL 构建中编入 modules/、fs.o、failsafe.o，并由 makefsdata 将 fsdata/* 生成 httpd-fsdata.c。
- modules/Makefile 编入 backup/env/helpers/mibib/misc/network/sysinfo/syslog/upgrade/webterm；mac.o 仅在 CONFIG_FAILSAFE_MAC_MANAGEMENT 下编入。

misc.c 页面/资源注册：
| URL | Handler | 行为 |
|---|---|---|
| /、/cgi-bin/luci、/cgi-bin/luci/、/index.html | index_handler | 返回 index.html |
| /firmware.html、/art.html、/cdt.html、/initramfs.html、/ptable.html、/simg.html、/uboot.html、/mibib.html | upload_html_handler | 动态生成上传页 HTML |
| /backup.html、/env.html、/mac.html、/network.html、/syslog.html、/webterm.html、/reboot.html、/settings.html | html_handler | 依据 URI 文件名从嵌入式文件系统返回页面 |
| /main.js | js_handler | 返回统一前端 JS |
| /style.css | style_handler | 返回 CSS |
| 未匹配 URI | not_found_handler | 返回 404 页面 |

misc.c 的 html_handler 以 request->urih->uri + 1 作为文件名调用 fs_find_file()。upload_html_handler 动态生成页面，并调用 appInit(page_name)。

## 3. main.js 请求与后端 handler 映射

以下请求来自固定 commit 的 failsafe/fsdata/main.js；C handler 来自对应模块注册函数。

| 前端 URL | 方法/用途 | C handler | 后端行为 |
|---|---|---|---|
| /version | GET | misc.c:version_handler | 返回 version_string |
| /sysinfo | GET | sysinfo.c:sysinfo_handler | 返回系统/硬件 JSON |
| /network/info | GET | network.c:network_info_handler | 返回 ipaddr/netmask/serverip 等环境值 |
| /network/set | POST | network.c:network_set_handler | 修改网络环境变量并 saveenv() |
| /network/reset | POST | network.c:network_reset_handler | 恢复默认网络配置并保存 |
| /env/list | GET | env.c:env_list_handler | env_export() 后返回环境变量 |
| /env/set | POST | env.c:env_set_handler | setenv() 后 saveenv() |
| /env/unset | POST | env.c:env_unset_handler | 删除变量后 saveenv() |
| /env/reset/all | POST | env.c:env_reset_all_handler | 重置全部环境并保存 |
| /env/reset/single | POST | env.c:env_reset_single_handler | 恢复单个默认值并保存 |
| /env/restore | POST | env.c:env_restore_handler | env_import() 后 saveenv() |
| /syslog/poll | GET 轮询 | syslog.c:syslog_poll_handler | 从 gd->console_out membuff 读取日志 |
| /backup | POST | backup.c:backup_handler | 读取所选存储范围并返回备份数据 |
| /mibib/reload | POST | mibib.c:mibib_reload_handler | 解析上传 MIBIB/分区数据并初始化运行时分区表 |
| /mac/info | GET | mac.c:mac_info_handler | 读取 MAC 数据 |
| /mac/set | POST | mac.c:mac_set_handler | 写入 MAC 数据，可能触及 SPI/NAND/MMC |
| /upload | POST | upgrade.c:upload_handler | 接收上传文件并建立升级会话 |
| /result | POST | upgrade.c:result_handler | 根据上传类型进入后续处理/写入路径 |
| /webterm/cmdlist | GET | webterm.c:webterm_cmdlist_handler | 枚举 linker-list 命令表并返回 JSON |
| /webterm/exec | POST | webterm.c:webterm_exec_handler | 调用 run_command() 执行用户提交的 U-Boot 命令 |
| /webterm/upload | POST | webterm.c:webterm_upload_handler | 设置上传文件信息并检查固件类型/MD5 |
| /reboot | GET | misc.c:reboot_handler | HTTP 会话关闭后调用 do_reset() |

main.js 关键请求位置：/version 约 1005 行；/upload、/result 约 1969、2033 行；/sysinfo 约 2471、6135、6157 行；/backup 约 3031、3179 行；/env/* 约 3785–4129 行；/network/* 约 4382–4512 行；/webterm/* 约 4786、5367、5476 行；/syslog/poll 约 6471 行；/mac/* 约 7194、7250 行。

## 4. PA-220 适配对照

### 4.1 静态页面与 URI 分发
uBootKit 有静态资源嵌入、fs_find_file() 和 URI 回调框架。PA-220 固定源码没有 failsafe/ 或 HTTPD 实现（前置 P1-A1 已核对）。页面资源、URI 表思想可复用；HTTPD API、嵌入文件系统构建规则需在 U-Boot 2013.07 中重建。源码存在；PA-220 对应 HTTP 服务源码不存在；编译/实机 NOT VERIFIED。

### 4.2 sysinfo
sysinfo.c:86-286 的 sysinfo_handler 生成 JSON，包含版本、FDT board 信息、gd->bd/RAM 信息、SMEM、SPI、MMC、NAND 和分区数据。
依赖包括 asm/arch-qca-common/smem.h、qca_smem_flash_info、get_smem_ptable_addr()、SMEM 分区结构、ipq_api.h 中的 has_spi()/has_mmc()/has_nand()，以及 Qualcomm MMC host 全局对象。PA-220 有 OCTEON RAM/board 信息、SPI/MMC 子系统和自身驱动接口，但没有证据表明存在 Qualcomm SMEM 或 QCA MMC host API。SMEM/分区字段不能直接搬运；应以 PA-220 board info、OCTEON SPI/MMC API、EEPROM/CPLD 只读接口重写数据源。SPI probe 仍需核实具体副作用和芯片选择。通用 JSON/页面格式可复用，硬件采集层必须重写。

### 4.3 network
network.c:94-171 注册 /network/info、/network/set、/network/reset。读取/修改 ipaddr、netmask、serverip、custom_network；修改路径调用 setenv() 和 saveenv()。默认值可来自 CONFIG_IPADDR、CONFIG_NETMASK、CONFIG_SERVERIP。failsafe.c 还可在 CONFIG_NET_FORCE_IPADDR 下将编译期地址写入网络全局状态。
PA-220 旧网络栈有 NetOurIP、NetServerIP 等状态，配置包含 DHCP/Ping/TFTP；环境变量接口存在。但修改环境变量不等于更新旧网络栈当前运行状态，handler 必须重新设计网络状态同步。
高风险：PA-220 octeon_pa220.h 定义 CONFIG_ENV_IS_IN_SPI_FLASH、CONFIG_ENV_OFFSET=0xe00000、CONFIG_ENV_OFFSET_REDUND=0xe10000。PA-220 实际使用 common/env_sf.c（由 CONFIG_ENV_IS_IN_SPI_FLASH 选择）的 saveenv() 会执行 spi_flash_erase() 和 spi_flash_write()。因此不能照搬网络设置接口，也不能在 RAM 测试中触发保存。只读网络信息可重写；设置接口必须从第一版 Web 中禁用。

### 4.4 env
env.c:135-374 注册列表、设置、删除、全部/单项重置和恢复上传。读取路径使用 env_export()、getenv()、getenv_default()；修改路径使用 setenv()、env_import()，并在每个变更路径调用 saveenv()。
PA-220 common/env_common.c 有 getenv()、getenv_default()、env_import()；common/cmd_nvedit.c 有环境编辑及导入/导出框架。具体 API 签名和编译兼容性仍需后续核对。只读环境展示有较好复用基础；set/unset/reset/restore 必须从 PA-220 第一版 Web 删除或不可达，因为 saveenv() 会擦写 SPI 环境区。

### 4.5 syslog
syslog.c:46-92 的 syslog_poll_handler 从 gd->console_out 读取 membuff，调用 membuff_avail()、membuff_get()。PA-220 global_data 是否包含同样的 console_out membuff、串口输出是否已接入环形缓冲，当前 UNKNOWN。不能仅因两者都有串口控制台就认定可复用。页面/轮询协议可复用；后端需先检查 PA-220 console/membuff 实现，否则重写只读日志缓冲。

### 4.6 其他路由的适配边界
- backup.c 的 /backup 包含 SMEM SPI/NAND 读取及 MMC block 读取分支；Qualcomm SMEM 分区解析不可直接移植，读操作也需核实目标设备、范围和边界。
- mibib.c 的 /mibib/reload 使用 Qualcomm SMEM flash info、MIBIB 解析和运行时分区表初始化；PA-220 不存在 Qualcomm MIBIB 机制，不能直接移植。
- mac.c 的 /mac/set 进入 write_data_to_spi/nand/mmc，并非只改运行时变量；仅保留读取需求并用 PA-220 EEPROM/板级数据接口重写。
- webterm.c 的 /webterm/exec 将 HTTP 表单 cmd 交给 run_command() 执行任意 U-Boot 命令，绕过只读 Web 安全边界；不适合直接纳入 PA-220 Web 初版。/webterm/upload 会设置文件信息环境变量。
- upgrade.c 的 /upload、/result 进入 failsafe_write_image()，可能触发持久化写入、自动重启或从内存启动。其完整写路径留待 P1-A2 最后审计，本报告不将其视为已审计完成。

## 5. 本批结论与状态

1. 页面统一使用 main.js；页面导航 URL 与 API URL 是两层不同路由，均由 C handler 显式注册。
2. 页面资源分发和 URI handler 框架有结构复用价值；PA-220 必须新建适配层，不能直接使用 uBootKit HTTP/TCP ABI。
3. sysinfo、network、env、syslog 四个业务域中，环境变量基础 API 和网络状态/命令有一定通用接口基础；硬件数据源、日志缓冲、网络运行时绑定仍需重写或确认。
4. 网络设置、环境修改/恢复、MAC 写入、Web terminal 命令执行、升级处理均有明显副作用。PA-220 第一版 Web 必须保持只读。
5. RAM chain-load 不会使 saveenv() 变成 RAM-only；PA-220 SPI 环境区仍是持久化风险。

| 项目 | 源码存在 | PA-220 对应 | 编译/实机 |
|---|---|---|---|
| 页面资源/URI 分发 | 已确认 | 需新建 HTTP 服务 | 未验证 |
| sysinfo | 已确认 | 硬件采集需重写 | 未验证 |
| network | 已确认 | 旧网络栈/环境 API 存在，handler 需重写 | 未验证 |
| env | 已确认 | 基础 env API 存在；写接口禁止照搬 | 未验证 |
| syslog | 已确认 | console membuff UNKNOWN | 未验证 |
| backup/mibib/mac/webterm | 路由和主要副作用入口已定位 | P1-A2 逐模块详细审计 | 未验证 |
| upgrade | 路由已定位 | P1-A2 最后审计完整写路径 | 未验证 |

本报告没有执行编译、设备访问、SPI/eMMC 写入或功能代码修改。
