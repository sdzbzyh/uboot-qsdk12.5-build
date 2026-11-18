# P1-A2：Web 业务模块审计（第一组：只读/低风险模块）

- 状态：**本组源码审计完成；未移植、未编译、未实机验证**
- 日期：2026-11-18
- uBootKit：chenxin527/uboot-qsdk12.5-build@9ce315735ac88fd0a7c7697bc527a8616ed0a972
- PA-220：sdzbzyh/pa220-openwrt@801717ba6a8a59ed2cfd53bf3e1b19841fecd2e8
- 文档分支：feature/pa220-ubootkit-web
- 范围：sysinfo.c、network.c、env.c、syslog.c，以及前端 main.js 参数对应关系。
- 本报告不是 P1-A2 全部完成报告；misc.c、webterm.c、backup.c、mibib.c、mac.c、upgrade.c 尚未完成逐函数审计。

## 1. 构建编入关系与配置边界

uBootKit：
- u-boot-2016/Makefile:704：`libs-$(CONFIG_HTTPD) += failsafe/`，failsafe 目录由 CONFIG_HTTPD 控制。
- failsafe/Makefile：非 SPL（`CONFIG_SPL_BUILD != y`）时编入 `modules/`、`fs.o`、`failsafe.o`。
- failsafe/modules/Makefile：`sysinfo.o`、`network.o`、`env.o`、`syslog.o` 均为 `obj-y`，没有独立模块开关。
- CONFIG_FAILSAFE_MAC_MANAGEMENT 只控制 mac.o，不影响本组四个对象。
- 本次只确认源码和 Makefile 编入规则；没有读取目标板最终 .config/defconfig 来确认 CONFIG_HTTPD 实际启用，亦未构建。

PA-220：
- 固定配置 octeon_pa220.h 启用 OCTEON MMC、MMC、OCTEON SPI、SPI_FLASH、Winbond SPI Flash、EFI/GPT 等相关能力。
- 这些配置说明相应源码构建入口被配置，不等于设备上的 eMMC/SPI/GPT 已经实机验证。
- PA-220 网络命令由 octeon_cmd_conf.h 中 CONFIG_CMD_NET 等配置提供；它没有 uBootKit 的 CONFIG_HTTPD failsafe 服务。

## 2. sysinfo.c

### 2.1 入口与调用链
- 文件：u-boot-2016/failsafe/modules/sysinfo.c
- `sysinfo_register_uri_handlers()`（284-287）注册 GET 前端请求对应的 `/sysinfo` 到 `sysinfo_handler()`（86-282）。
- 前端：failsafe/fsdata/main.js 约 2471、6135、6157 行调用 `/sysinfo`，用于初次加载/刷新系统信息。
- Handler 仅在 `HTTP_CB_NEW` 处理请求，在 `HTTP_CB_CLOSED` 释放 response->session_data；没有显式检查 GET method。
- 为 JSON 分配固定 16,666 字节缓冲区；响应完成后由 HTTP session close 路径释放。

### 2.2 数据、全局状态与底层依赖
- 版本：`version_string`。
- 板卡/FDT：`gd->fdt_blob`，读取 host_name、model、compatible；同时读取 `gd->bd->bi_arch_number`、`gd->ram_size`。
- IPQ 启动状态：`is_9008_mode()`。
- SMEM：全局 `qca_smem_flash_info`，`get_smem_ptable_addr()`、SMEM 分区结构及 `getpart_offset_size()`。
- IPQ 设备探测：`has_spi()`、`has_mmc()`、`has_nand()`。
- SPI：`spi_flash_probe()`，读取 name/size/page/sector/erase 属性。
- MMC：依赖全局 `mmc_host`（按 CONFIG_SDHCI_SUPPORT 选择 qca_mmc 或 sdhci_host），调用 `find_mmc_device()`、`mmc_get_dev()`、`mmc_get_life_info()`，并读取 GPT 分区。
- NAND：读取 `nand_info[CONFIG_NAND_FLASH_INFO_IDX]` 属性。
- 本 handler 不调用 setenv/saveenv、不执行存储擦写；但 SPI probe、MMC life info 和分区查询属于总线/设备访问，需在 PA-220 适配时逐项确认读取副作用及空指针/设备未就绪处理。
- 除动态 JSON 缓冲区外，不修改全局状态；HTTP session close 释放缓冲区。

### 2.3 PA-220 对照与适配结论
- 通用可复用数据：PA-220 的 generic global_data 有 `ram_size`、`fdt_blob`；板级信息可从 OCTEON board data/PA-220 自有板级接口获取。FDT 字段是否在实际设备 DT 中存在仍需核实。
- SPI：PA-220 配置启用 OCTEON SPI 与 SPI_FLASH，可用自身 SPI flash API 重新实现只读属性读取；不能沿用 IPQ 的 has_spi() 或默认 bus/chip-select 假设。
- MMC：PA-220 配置启用 CONFIG_OCTEON_MMC/CONFIG_MMC，但 uBootKit 的 qca_mmc/sdhci_host 全局对象和 MMC 版本/生命周期字段布局不应直接复用；需对照 OCTEON MMC 驱动 API。
- 分区：PA-220 启用 EFI/GPT 支持，可研究旧版 `get_partition_info_efi()` 等接口是否存在及签名差异；SMEM 分区部分没有 PA-220 等价机制。
- NAND、SMEM、9008 mode、IPQ has_* API：不适用或需替换。PA-220 无 Qualcomm SMEM/MIBIB 机制；PA-220 实际存储以 eMMC、SPI NOR、TLV EEPROM 为主。
- 结论：JSON schema/页面可借鉴；硬件数据采集层必须按 OCTEON/PA-220 重写。不可直接移植 SMEM/IPQ 分支。

### 2.4 源码健壮性待修
- `APPEND_JSON_TO_BUF`（70-73）直接执行 `snprintf(buf + len, left - len, ...)`，没有统一检查剩余空间和负值；多个字段累加后可能使剩余长度失效。
- `spi_flash_probe()`、MMC device/descriptor 和 `get_smem_ptable_addr()` 的返回值/指针在使用前未全部做空值检查。
- `fdt_getprop()` 结果可为空，需确认 `json_escape()` 对 NULL 的处理。
- 上述问题在重写时应一并处理，不应原样复制。

## 3. network.c

### 3.1 入口与前端参数
- `network_register_uri_handlers()`（167-172）注册 `/network/info`、`/network/set`、`/network/reset`。
- `network_info_handler()`（94-120）：GET，读取 ipaddr/netmask/serverip。
- `network_set_handler()`（122-149）：POST，读取表单字段 ipaddr、netmask、serverip。
- `network_reset_handler()`（151-165）：POST，恢复编译期默认值。
- 前端 main.js 约 4382 行 GET /network/info；约 4476 行 POST /network/set；约 4512 行 POST /network/reset。

### 3.2 状态修改与持久化
- 默认值来自 CONFIG_IPADDR/CONFIG_NETMASK/CONFIG_SERVERIP；若未定义，回退 192.168.1.1、255.255.255.0、192.168.1.2。
- `network_set_env()`（53-92）读取 custom_network、ipaddr、netmask、serverip；按需 setenv()，有任何变更即 saveenv()。
- 网络 handler 修改的是环境变量，不直接更新网络栈运行时的 NetOurIP/NetServerIP，也不重启网卡或重建网络循环。
- `failsafe.c` 另有 CONFIG_NET_FORCE_IPADDR 条件编译逻辑；该逻辑不在 network.c 本身。
- network.c 没有直接调用 Qualcomm 专有函数；其主要依赖为 HTTPD API、U-Boot env API 和 saveenv()。

### 3.3 PA-220 对照与风险
- PA-220 u-boot/net/net.c 定义 NetOurIP、NetServerIP；NetInitLoop()（约 258-270 行）从 getenv_IPaddr("ipaddr") 和 getenv_IPaddr("serverip") 初始化运行时网络状态。
- 这说明环境变量与网络运行状态有明确同步时点；Web 服务运行中修改 env 后，不能假设当前 NetOurIP 自动变化，需设计安全的网络重启/重新初始化路径。
- PA-220 octeon_pa220.h 未显示 CONFIG_IPADDR/CONFIG_NETMASK/CONFIG_SERVERIP 编译期宏；其额外环境变量配置包含 loadaddr、ethact、ramimage 等。不能照搬 uBootKit 默认网络配置。
- PA-220 环境后端由 CONFIG_ENV_IS_IN_SPI_FLASH 选择 common/env_sf.c，而非 env_flash.c。配置偏移：主区 0xe00000，冗余区 0xe10000。
- common/env_sf.c 的 saveenv()（47 起，冗余实现）调用 spi_flash_probe()、spi_flash_erase()、spi_flash_write()，并更新冗余标记；这是明确的 SPI NOR 持久化写路径。
- 风险：网络配置 POST 会触发 SPI 环境区写入。PA-220 Web 初版不得提供此接口。若未来要支持网络临时配置，应仅更新 RAM 运行时网络状态，明确不调用 saveenv()，并由用户另行决定是否保存。

### 3.4 源码健壮性待修
- 未对 IP/netmask/serverip 做格式/掩码合法性校验。
- setenv() 失败时 modified 计数可能不增加，函数仍可能返回成功；多字段更新也可能发生部分修改后再保存。
- JSON 响应直接用 %s 拼接环境变量值，没有 JSON escape。
- custom_network 状态按变量是否存在判断，而非解析其值；PA-220 不应照搬其语义。

## 4. env.c

### 4.1 入口、表单参数与调用链
- `env_register_uri_handlers()`（367-375）注册六个接口。
- `/env/list`：GET；`env_list_handler()`（135-168）调用 `failsafe_env_export_text()`（88-133），后者分配 env_t、调用 env_export()，将 NUL 分隔环境转换为换行文本。
- `/env/set`：POST，字段 name/value；`env_set_handler()`（170-218）校验长度、读取现值、setenv()，变更后 saveenv()。
- `/env/unset`：POST，字段 name；`env_unset_handler()`（220-254）setenv(name,NULL) 后 saveenv()。
- `/env/reset/all`：POST；`env_reset_all_handler()`（256-277）set_default_env(NULL) 后 saveenv()。
- `/env/reset/single`：POST，字段 name；`env_reset_single_handler()`（279-325）调用 getenv_default()、setenv()、saveenv()。
- `/env/restore`：POST，字段 envfile；`env_restore_handler()`（327-365）校验 CRC、env_import()、saveenv()。
- 前端 main.js 约 3785、3857、3899、3962、4050、4080、4129 行分别对应 set、重复设置/修改、删除、列表、unset/reset、reset all、restore 交互。

### 4.2 全局状态/API/平台依赖
- 直接读写 U-Boot 全局环境哈希表；setenv/env_import/set_default_env 会改变当前运行时环境。
- 使用 env_export、getenv、getenv_default、setenv、env_import、saveenv、crc32。
- 不包含 IPQ 专有硬件调用；依赖 HTTPD 表单、U-Boot environment API。
- PA-220 common/env_common.c 有 getenv()/getenv_default()/env_import()；common/cmd_nvedit.c 提供 env 命令导入/导出功能，导出命令内部使用 hexport_r()。
- PA-220 include/environment.h 暴露 env_import()，但本次未发现与 uBootKit 同名的公开 env_export() 声明/实现；PA-220 只读环境导出可能需要用 hexport_r() 封装，而不是直接照搬 env_export()。
- PA-220 common/env_sf.c 的 saveenv() 明确擦写 SPI NOR 主/冗余环境扇区；所有修改类 handler 均为持久化风险。

### 4.3 必须记录的缺陷/风险
- env_restore_handler()（344-358）只检查 envfile/data 是否存在，未先检查 envfile->size 至少覆盖 env_t/ENV_SIZE；随后按固定 ENV_SIZE 读取 CRC 数据，短文件可导致越界读取。必须在任何移植版本中先校验上传长度、头部布局和 CRC 范围。
- env_import() 后直接 saveenv()；这是明确的 SPI 写入。
- env_set_handler() 在 setenv() 失败且 changed=false 时将 ret 置 0，可能把未成功修改报告为成功。
- reset all 会替换整个运行时环境，不只是恢复网页相关变量。
- 即便只读 /env/list，也可能暴露敏感启动参数、网络地址或其他环境内容；PA-220 Web 需要字段白名单/脱敏，而不是默认全量导出。

### 4.4 适配结论
- 只读展示：可基于 PA-220 env 哈希表/hexport_r() 重写，需字段白名单和敏感项过滤。
- 修改、删除、重置、恢复：不进入 PA-220 第一版 Web；后续如需实现，必须独立风险审查，默认不调用 saveenv()，并考虑用户授权后的单独持久化流程。

## 5. syslog.c

### 5.1 入口与行为
- `syslog_register_uri_handlers()`（90-93）注册 GET /syslog/poll。
- `syslog_poll_handler()`（46-88）检查 request method，读取 `gd->console_out`；若无 start 返回 503。
- `membuff_avail()` 获取可读字节数，最多读取 SYSLOG_POLL_MAX=8192；分配 want+1 字节，调用 `membuff_get()`，末尾补 NUL 后返回 text/plain。
- 前端 main.js 约 6471 行轮询 /syslog/poll。
- handler 只分配/释放 session 缓冲，但 membuff_get() 会推进/消费缓冲区读位置，属于对日志队列状态的修改，不是纯粹 peek。

### 5.2 PA-220 对照
- uBootKit 依赖 gd->console_out 和 membuff；syslog.c 还 include ipq_api.h，但核心读取路径依赖的是 console membuff。
- PA-220 include/asm-generic/global_data.h 中有 precon_buf_idx，没有 console_out 成员；PA-220 common/console.c 的 precon_buf 是早期 console 初始化前的缓冲，不等价于持续记录所有运行期 console 输出的环形日志。
- 本次未找到 PA-220 等价的 console_out/membuff 日志接口，标记 NOT FOUND in inspected PA-220 global_data/console paths；不能将 precon buffer 当作 syslog 后端。
- 适配方式：保留页面和轮询协议，新增独立、限长的只读日志环形缓冲，并明确读接口是否消费数据；也可设计基于序号的非破坏性读取。

## 6. 本组汇总矩阵

| 模块 | 主要依赖 | PA-220 对应 | 适配结论 | 风险/状态 |
|---|---|---|---|---|
| sysinfo | IPQ SMEM、ipq_api、SPI/MMC/NAND、GPT | OCTEON gd/FDT、SPI/MMC、GPT 配置；无 SMEM/NAND | 页面/JSON 可复用，硬件采集重写 | 指针检查、JSON 缓冲区边界；未编译/未实机 |
| network | env、saveenv、HTTPD | NetOurIP/NetServerIP、getenv_IPaddr、env_sf | 只读信息可重写；设置接口禁用或仅 RAM-only | saveenv 会擦写 SPI；未编译/未实机 |
| env | env_export/import、setenv、saveenv | getenv/getenv_default/env_import、hexport_r；SPI env_sf | 只读白名单导出可重写；写接口禁用 | env_restore 长度校验缺失；saveenv 写 SPI |
| syslog | gd->console_out、membuff | precon_buf_idx/precon_buf 仅早期缓冲 | 页面协议可复用，日志环形缓冲需重写 | membuff_get 消费数据；PA 后端 NOT FOUND |

## 7. 本批完成范围与后续

已完成：四个模块的源码入口、参数、主要调用、全局环境/日志状态、IPQ 依赖、PA-220 对照和初步适配结论。

尚未完成：misc.c、webterm.c、backup.c、mibib.c、mac.c、upgrade.c；其中 upgrade 必须最后逐条标注所有可能的持久化写入。

未执行：功能代码修改、构建、TFTP/RAM 启动、SPI/eMMC 写入或实机测试。所有编译/实机状态均 NOT VERIFIED。
