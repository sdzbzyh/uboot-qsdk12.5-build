# PA-220 P4-1 首批网络源码审计（2026-11-19）

- 审计时集成分支 HEAD：`87bb79b897c29ff2ff221c7a6cb14b495e5e7973`
- PA-220 固定源码：`IronDemiGod/pa220-openwrt@801717ba6a8a59ed2cfd53bf3e1b19841fecd2e8`
- uBootKit 固定源码：`chenxin527/uboot-qsdk12.5-build@9ce315735ac88fd0a7c7697bc527a8616ed0a972`
- 范围：PA-220 DHCP Client、Ping、TFTP GET/PUT/Server；与 uBootKit U-Boot 2016 网络实现对照。
- 结果：**首批源码审计完成；未构建、未进行网口实机测试。**

## 1. PA-220 网络配置与命令入口

固定 PA-220 配置：
- `u-boot/include/configs/octeon_cmd_conf.h:12-25` 在未定义 `OCTEON_NO_NETWORK` / `CONFIG_OCTEON_DISABLE_NETWORKING` 时定义 `CONFIG_CMD_NET`、`CONFIG_CMD_PING`、`CONFIG_CMD_DHCP`、`CONFIG_CMD_TFTPPUT`、`CONFIG_CMD_TFTPSRV`。
- `u-boot/net/Makefile:12-23` 在 `CONFIG_CMD_NET` 下编入 `bootp.o`、`eth.o`、`net.o`、`tftp.o`，在 `CONFIG_CMD_PING` 下编入 `ping.o`。
- `u-boot/common/cmd_net.c` 注册 `dhcp`、`ping`、`tftpboot`、`tftpput`、`tftpsrv` 命令。
- `octeon_pa220.h` 选择 `CONFIG_OCTEON_MGMT_ENET`，环境默认 `ethact=octrgmii0`、`ethprime=octrgmii0`、`autoload=n`。配置默认值不等于当前 RAM 环境实际值，运行时仍须读取确认。

这些是固定源码配置证据；最终构建 `.config` 和实机网口行为尚未在本批复核。

## 2. DHCP Client

### 2.1 PA-220 既有路径

- `common/cmd_net.c:90-99` 的 `do_dhcp()` 调用 `netboot_common(DHCP,...)`。
- `net/net.c` 的 `NetLoop()` 对 DHCP 分支清 `NetOurIP` 并调用 `DhcpRequest()`。
- `net/bootp.c:608-700` 构造 BOOTP/DHCP 请求，设置 UDP handler；`DhcpRequest()` 最终调用 `BootpRequest()`。
- `net/bootp.c:897-971` 的 `DhcpHandler()` 处理 OFFER/ACK 状态机，ACK 后调用 `BootpCopyNetParams()` 更新运行时网络参数。
- `BootpCopyNetParams()` 设置 `NetOurIP`、可选 `NetServerIP`、bootfile；`netboot_update_env()` 随后通过 `setenv()` 更新 ipaddr/serverip/gatewayip/netmask 等 RAM 环境变量，不调用 `saveenv()`。
- ACK 后调用 `net_auto_load()`。`net/net.c` 明确检查 `autoload`；当 `getenv_yesno("autoload")==0`（即 n）时只完成网络配置，不启动 TFTP GET。
- PA-220 板级默认环境包含 `autoload=n`，但可能被已有 SPI 环境覆盖，因此实测前必须 `printenv autoload` 确认；若非 n，只允许用 RAM-only `setenv autoload n` 修正，禁止 `saveenv`。

### 2.2 与 uBootKit 对照

- 两版均使用 BOOTP/DHCP Client 状态机并依赖各自 U-Boot NetLoop。
- PA-220 是旧接口风格（`BootpRequest`、`DhcpHandler`、`NetSetTimeout`、`NetSendPacket`）；uBootKit 2016 使用新接口/命名（`bootp_request`、`dhcp_handler`、`net_set_timeout_handler`、`net_send_packet` 等）。
- DHCP 协议状态机可作为行为参考，但不应复制另一代源码文件或混用全局变量/packet handler API。
- 适配结论：**直接复用 PA-220 现有 DHCP Client；无需移植 uBootKit bootp.c。** P4 的工作是确认最终配置、环境同步、autoload 关闭和网卡收发/退出行为。

## 3. Ping

### 3.1 PA-220 既有路径

- `cmd_net.c` 的 `do_ping()` 将目标 IPv4 解析为 `NetPingIP`，调用 `NetLoop(PING)`。
- `net/net.c` 的 PING 分支调用 `ping_start()`。
- `net/ping.c` 通过 ARP 获取目标 MAC，发送 ICMP Echo Request；匹配 Echo Reply 后将 NetLoop 置为成功。
- `ping_start()` 设置 10 秒超时；`ping_timeout()` 调用 `eth_halt()` 并将 NetLoop 置为失败。Ping 因而会初始化/停止网卡并改变瞬时网络状态，但不写持久化存储。
- `ping_receive()` 还实现 Echo Request 的回复路径；这不改变命令主要的主动 Echo 测试行为。

### 3.2 与 uBootKit 对照

- uBootKit 2016 Ping 主体同样使用 ARP + ICMP、10 秒 timeout 和 NetLoop 状态机。
- uBootKit 增加 HTTPD 运行时跳过 `eth_halt()` 的保护；PA-220 当前无 HTTPD/TCP 服务，不能把该分支原样移植。未来共享网络服务时需要统一网卡生命周期仲裁。
- 适配结论：**直接复用 PA-220 现有 Ping；无需移植 uBootKit ping.c。** 实测需覆盖成功、超时和接口恢复。

## 4. TFTP

### 4.1 PA-220 接收写入路径

- `net/tftp.c:160-197` 的 `store_block()` 计算 `offset = block * TftpBlkSize + TftpBlockWrapOffset`、`newsize = offset + len`。
- 未启用 `CONFIG_SYS_DIRECT_FLASH_TFTP` 时，直接执行 `memcpy((void *)(load_addr + offset), src, len)`；本函数没有检查乘法/加法溢出、目的地址回绕、RAM 总边界或 named block 边界。
- TFTP GET 的 DATA handler 在接收 block 后调用 `store_block()`；TFTP Server 的 WRQ 接收路径最终也进入同一 DATA/store 路径。
- `CONFIG_SYS_DIRECT_FLASH_TFTP` 若启用会在特定目标地址调用 `flash_write()`，属于持久化写入。板级头当前未定义该宏，但最终 `.config` 仍需确认关闭。
- 因此 TFTP GET 和 TFTP Server 接收功能均为 **BLOCKED**，不能通过指定某个猜测地址来规避 validator 缺失。

### 4.2 TFTP PUT

- PUT 路径通过 `load_block()` 从 `save_addr + offset` 读取 RAM，再向远端发送。
- 不写本机存储，但如果源地址/长度未经范围验证，可能越界读取 U-Boot 内存或向网络泄露数据；本批不启用/不实测。

### 4.3 uBootKit 对照

- uBootKit 新版 `net/tftp.c` 的接收路径使用 IPQ806X、`CONFIG_SYS_SDRAM_END`、FDT/TrustZone 常量作边界判断，并通过 `map_sysmem()` 取得映射指针。
- 这些地址常量属于 IPQ 平台内存布局，不能移植到 CN7130/PA-220。
- 即使新版检查了某个 end address，仍需按 PA-220 实际 cached/uncached/physical alias、整数溢出和 named-block 所有权设计通用 validator；不能把 IPQ 检查当作可复用安全方案。

## 5. 当前决策

| 功能 | 源码审计结论 | 当前状态 |
|---|---|---|
| DHCP Client | 复用 PA-220 原生实现；autoload=n 时 DHCP-only | 源码审计完成，实机 NOT RUN |
| Ping | 复用 PA-220 原生实现 | 源码审计完成，实机 NOT RUN |
| TFTP GET | 现有代码可用性不等于接收安全；逐 block validator 缺失 | BLOCKED |
| TFTP Server | 同一 RAM 写入缺陷；不启动 | BLOCKED |
| TFTP PUT | 源地址/长度有越界读取与信息泄漏风险 | NOT RUN |
| uBootKit DHCP/Ping/TFTP 源码 | 作为协议/行为参考，不直接复制 | 已对照 |

## 6. 本批安全边界

- 不执行 `saveenv`、EEPROM/TLV/CPLD 写入、SPI/eMMC/NAND/USB block 写入/擦除。
- 不执行 `bootloaderupdate`、`namedalloc`、`namedfree`、`mtest`。
- 不执行 `tftpboot`、`tftpsrv`、`tftpput` 的实机数据传输测试。
- 不把 `0x81000000` 当作默认安全上传地址；这是原厂 Stage3 镜像加载地址。
- 统一 `octeon_load_range_check(uint64_t addr, uint64_t len)` 未实现前，TFTP/HTTP/ELF 所有接收路径保持 BLOCKED。
- 本批仅完成源码审计与文档，不构成编译或硬件网络验证。
