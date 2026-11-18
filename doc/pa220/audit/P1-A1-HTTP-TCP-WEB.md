# P1-A1：HTTP/TCP/Web 核心链路审计（第一批）

- 状态：**本批源码审计完成；未适配、未编译、未实机验证**
- 日期：2026-11-18
- uBootKit 审计对象：`chenxin527/uboot-qsdk12.5-build@9ce315735ac88fd0a7c7697bc527a8616ed0a972`
- PA-220 对照基线：`sdzbzyh/pa220-openwrt@801717ba6a8a59ed2cfd53bf3e1b19841fecd2e8`
- 文档分支：`feature/pa220-ubootkit-web`
- 本记录只记录源码证据和适配分析，不代表功能已经移植。

## 1. 本批审计范围

已逐文件读取并追踪：

- uBootKit：`u-boot-2016/net/httpd.c`
- uBootKit：`u-boot-2016/include/net/httpd.h`
- uBootKit：`u-boot-2016/net/tcp.c`
- uBootKit：`u-boot-2016/include/net/tcp.h`
- uBootKit：`u-boot-2016/failsafe/failsafe.c`
- uBootKit：`u-boot-2016/failsafe/Kconfig`
- uBootKit：`u-boot-2016/failsafe/Makefile`
- uBootKit：`u-boot-2016/failsafe/fs.c`
- uBootKit：`u-boot-2016/failsafe/fs.h`
- uBootKit：`u-boot-2016/failsafe/modules/modules.h`
- uBootKit：`u-boot-2016/net/Makefile`
- uBootKit：`u-boot-2016/net/Kconfig`
- PA-220：`u-boot/net/net.c`
- PA-220：`u-boot/net/Makefile`
- PA-220：`u-boot/include/configs/octeon_cmd_conf.h`
- PA-220：`u-boot/include/configs/octeon_pa220.h`

另外对 PA-220 固定树做了文件存在性检索：没有 `u-boot/net/tcp.c`、`u-boot/net/httpd.c`、`u-boot/net/dhcpd.c` 或 `u-boot/failsafe/`。

## 2. uBootKit HTTP/TCP 实现边界

### 2.1 HTTPD 是建立在新增 TCP 子系统之上的

证据：

- `net/httpd.c:150-180` 的 `httpd_create_instance()` 调用 `tcp_listen(htons(port), httpd_tcp_callback)`。
- `net/httpd.c:209-230` 的 URI 注册使用动态分配的 handler 链表。
- `include/net/httpd.h` 暴露 `httpd_create_instance()`、`httpd_register_uri_handler()`、`httpd_request_find_value()` 等接口。
- `include/net/tcp.h` 暴露 `tcp_start()`、`tcp_listen()`、`tcp_connect()`、`tcp_send_data()`、`tcp_periodic_check()` 等接口。
- `net/Makefile:23-29` 明确：`CONFIG_HTTPD` 编入 `httpd.o`，`CONFIG_TCP` 编入 `tcp.o`。

因此 Web UI 不是一个可以只复制 HTML/URI handler 的小模块；其最低核心依赖至少包括 HTTPD + TCP + 现有 U-Boot 网络收发框架。

### 2.2 HTTPD 自己维护连接状态机

`net/httpd.c` 定义了：

- NEW
- HEADER_RECVING
- PAYLOAD_RECVING
- FULL_RCVD
- HEADER_SENT
- RESPONDING
- CLOSING

并通过 `httpd_tcp_callback()` 接收 TCP 层事件，再分派到 `httpd_rx()`、`httpd_tx()`、`httpd_cleanup()`。

这意味着移植时必须同时解决旧 PA-220 U-Boot 网络栈的 callback/packet 生命周期差异，不能假定 2016 版 `tcp_cb_data` 与 2013.07 ABI 相同。

## 3. HTTP 上传路径及内存依赖

这是本批中最重要的适配风险之一。

### 3.1 上传数据不是只存在 HTTP 接收缓冲区

`net/httpd.c:108-120` 的 `httpd_get_upload_buffer_ptr()`：

- DRAM >= 512 MiB 时使用 `CONFIG_SYS_SDRAM_BASE + SZ_256M`
- 否则使用 `IPQ_TFTP_MIN_ADDR`
- 调用 `is_memory_region_available(load_addr, size)`

这三个名字都需要在 PA-220 上重新定义/实现：

1. PA-220 的 DRAM 地址空间与 U-Boot 当前内存布局；
2. 上传区与 U-Boot、stack、heap、kernel load 地址之间的隔离；
3. `is_memory_region_available()` 对旧版 U-Boot 是否存在。

因此不能直接复制该函数。

### 3.2 Content-Length 驱动的连续内存上传

`net/httpd.c:417-625`：

- 解析 `Content-Length`
- 对 `/upload`、`/console/upload` 使用独立上传区
- 单次只允许一个大上传
- 使用 `upload_id` 标记上传
- 接收完成后 `flush_cache()`
- 最后在上传区末尾追加 NUL

`httpd_recv_payload()` 使用：

`size_recv = min(payload_size - upload_size, cbd->datalen)`

然后连续写入 `upload_ptr + upload_size`。

这对 PA-220 的 big-endian 本身不是主要障碍；真正的障碍是旧网络栈的数据缓冲区、缓存一致性接口和可安全占用的 DRAM 区域。

### 3.3 multipart 解析有额外 ABI/内存要求

`net/httpd.c:720-810`：

- 根据 boundary 在上传区中拆分 multipart/form-data；
- `MAX_HTTP_FORM_VALUE_ITEMS=5`；
- 为 boundary 动态分配内存；
- 将表单数据调整到 4-byte 对齐地址；
- 必要时 `memmove()`；
- URI handler 最终直接访问表单数据指针。

这部分原则上可作为通用业务层参考，但必须在 PA-220 旧版编译器、n32 ABI 和旧内存 API 下重新确认。

## 4. HTTPD 与 failsafe 主循环的耦合

`failsafe/failsafe.c:100-180` 的核心流程：

1. `net_init()`
2. 根据 `eth_is_on_demand_init()` 初始化网卡
3. `tcp_reset_all_conn()`
4. `tcp_start()`
5. 可选 `dhcpd_start()`
6. 可选 `telnetd_start()`
7. 循环执行：
   - `eth_rx()`
   - `tcp_periodic_check()`
   - Ctrl-C 检查
8. 退出时停止 DHCPD/TelnetD、`tcp_reset_all_conn()`、`eth_halt()`

因此 Web failsafe 并没有自己独立的以太网驱动；它依赖 U-Boot 网络设备框架。

PA-220 现有 `net/net.c:324-580` 的 `NetLoop()` 也采用 `eth_init()`、`eth_rx()`、超时处理和状态机，但它是旧版 UDP/协议状态机接口。uBootKit 的 TCP 层是独立新增的连接对象/回调模型。

**初步判断：**
- 以太网底层可以优先复用 PA-220 现有 OCTEON 网络驱动；
- HTTP/TCP 上层不能直接二进制/源码无修改搬运；
- 最可能的工作方式是把 uBootKit 的 TCP/HTTPD 逻辑作为参考，适配到 PA-220 2013.07 的网络 packet/定时器/内存 API。

## 5. DHCP/Telnet/网络服务的配置边界

uBootKit：

`net/Makefile:23-29` 明确存在：

- `CONFIG_DHCPD -> dhcpd.o`
- `CONFIG_HTTPD -> httpd.o`
- `CONFIG_TCP -> tcp.o`
- `CONFIG_TELNETD -> telnetd.o`
- `CONFIG_WGET -> wget.o`
- `CONFIG_NET_ABORT -> net_abort.o`

但目前不能仅据此说这些功能在某个 IPQ defconfig 中全部启用；这一项留到后续配置审计。

PA-220：

`u-boot/include/configs/octeon_cmd_conf.h:12-25` 已明确：

- `CONFIG_CMD_NET`
- `CONFIG_CMD_PING`
- `CONFIG_CMD_DHCP`
- `CONFIG_CMD_TFTPPUT`
- `CONFIG_CMD_TFTPSRV`
- `CONFIG_CMD_DNS`
- `CONFIG_CMD_CDP`

PA-220 `u-boot/net/Makefile` 只有旧版通用网络对象，没有 HTTPD/TCP/DHCPD/TelnetD/WGET 对象。

因此 PA-220 已具备 DHCP/TFTP 网络基础，但**没有 uBootKit 所需的 TCP/HTTP 服务实现**。

## 6. 页面资源与 URI handler

uBootKit `failsafe/Makefile` 在非 SPL 构建中：

- 编译 `modules/`
- 编译 `fs.o`
- 编译 `failsafe.o`
- 通过 `makefsdata` 将 `failsafe/fsdata/*` 转换为 `httpd-fsdata.c`

`failsafe/failsafe.c:48-62` 集中注册：

- index
- html
- backup
- env
- mac（受 `CONFIG_FAILSAFE_MAC_MANAGEMENT`）
- mibib
- misc
- network
- sysinfo
- syslog
- upgrade
- webterm

因此 Web 前端本身与后端 handler 是清晰分离的。PA-220 若采用同一 Web UI，可以保留页面资源/URI 组织思路，但所有涉及设备、存储、环境变量、升级和硬件信息的 handler 仍需逐项重写。

## 7. 与 PA-220 现有启动/内存设计的关键关系

PA-220 `u-boot/include/configs/octeon_pa220.h`：

- 注释明确 U-Boot 作为 SPI stage 3 或 RAM chain-load 使用，不负责初始化 DRAM。
- `CONFIG_NO_CLEAR_DDR=1`。
- `CONFIG_SYS_MEMTEST_START=0x22000000`
- `CONFIG_SYS_MEMTEST_END=0x2a000000`
- `linux_ram` 使用 `tftpboot` 将 initramfs 加载到 `loadaddr` 后从 RAM 启动。

这意味着 Web 上传区的地址选择必须避开：

- U-Boot 自身运行区；
- heap/stack；
- mtest 区域；
- TFTP/RAM boot 的 `loadaddr`；
- 未来可能用于 Web 上传固件的其他内存区。

尤其不能直接采用 uBootKit 的 `CONFIG_SYS_SDRAM_BASE + 256MiB` 规则。

## 8. HTTPD 移植可行性初步分类

| 子模块 | 源码存在 | PA-220 现状 | 初步适配方式 | 当前结论 |
|---|---|---|---|---|
| HTML/CSS/JS 静态资源 | 是 | 无 | 可直接作为资源重新生成 | 高可行 |
| URI handler 框架 | 是 | 无 | 可移植接口思想，需新建 | 高可行 |
| HTTP parser | 是 | 无 | 可移植，但需适配旧 U-Boot API | 中高 |
| HTTP response/gzip | 是 | 无 | 可移植；需核对 gzip/lib/内存 | 中 |
| TCP server | 是 | 无 | 新增/移植 uBootKit TCP 层 | 中 |
| failsafe 主循环 | 是 | 有旧 `NetLoop()` | 重写为 PA-220 网络驱动模型 | 中 |
| DHCP server | 是 | 只有 DHCP client | 新增 `dhcpd`，需确认端口/ARP/UDP API | 中 |
| TelnetD | 是 | 无 | 可选新增 | 中 |
| Web upload | 是 | 无 | 可实现，但必须重新设计安全 DRAM 区 | 中/高风险 |
| IPQ-specific PPE/SMEM/MIBIB | 是 | 无 | 不直接移植 | 不适用 |

## 9. 当前阶段的安全边界

本批只做源码读取。

没有：

- 修改 PA-220 功能代码；
- 修改 uBootKit 固定源码；
- 执行 eMMC/SPI NOR 写入；
- 执行固件升级；
- 进行硬件写测试；
- 声称 Web/TCP 已编译或实机可用。

PA-220 当前配置中存在 `CONFIG_ENV_IS_IN_SPI_FLASH`，并且定义了 `CONFIG_ENV_OFFSET=0xe00000` 与冗余环境区；这属于现有配置事实。本 P1 审计阶段仍遵守“RAM 测试优先、持久化写入不作为默认操作”的规则。

## 10. 下一步

P1-A1 后续不再重复 HTTPD/TCP 源码结构盘点，转入：

1. 页面 URL 与每个 URI handler 的实际调用链；
2. `failsafe/modules/sysinfo.c`、`network.c`、`env.c`、`syslog.c`；
3. 逐项确认 handler 使用的 Qualcomm/IPQ 专用 API；
4. 对照 PA-220 的 CPU、网络、EEPROM、CPLD、SPI/eMMC、环境变量接口；
5. 然后再审计 `backup.c`、`mibib.c`、`mac.c`；
6. `upgrade.c` 放到最后，并逐条标注所有可能的持久化写操作。

## 11. 状态矩阵

- **源码存在**：已确认 uBootKit HTTPD/TCP/failsafe 代码存在。
- **配置启用**：本批仅确认 PA-220 网络基础配置；uBootKit 各 IPQ defconfig 的具体启用状态尚未完成。
- **编译成功**：未验证。
- **实机验证**：未验证。
