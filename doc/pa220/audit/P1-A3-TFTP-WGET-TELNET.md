# P1-A3：网络服务审计（第二批：TFTP、WGET、TelnetD）

- 状态：**本批源码静态审计完成；P1-A3 整体进行中；未编译、未实机验证**
- 日期：2026-11-18
- uBootKit：chenxin527/uboot-qsdk12.5-build@9ce315735ac88fd0a7c7697bc527a8616ed0a972
- PA-220：sdzbzyh/pa220-openwrt@801717ba6a8a59ed2cfd53bf3e1b19841fecd2e8
- 前置：P1-A3-BOOTP-DHCP.md
- 范围：net/tftp.c、net/wget.c、net/telnetd.c、include/net/telnetd.h；对照 PA-220 net/tftp.c、net/Makefile 和网络命令配置。

## 1. 构建条件

uBootKit net/Makefile：
- CONFIG_CMD_NET -> tftp.o。
- CONFIG_CMD_TFTPPUT -> tftp.c 内的 TFTP PUT 客户端状态机。
- CONFIG_CMD_TFTPSRV -> tftp.c 内的 TFTP Server WRQ 接收路径。
- CONFIG_WGET -> wget.o。
- CONFIG_TCP -> tcp.o；WGET/TelnetD 都依赖 TCP 子系统。
- CONFIG_TELNETD -> telnetd.o。
- CONFIG_SYS_DIRECT_FLASH_TFTP -> tftp.c store_block() 中可选的直接 Flash 写入路径。
- net/Kconfig 没有定义 CONFIG_WGET、CONFIG_TCP、CONFIG_TELNETD、CONFIG_SYS_DIRECT_FLASH_TFTP 等开关，需继续追踪板级 defconfig/Kconfig 来源。
PA-220：
- net/Makefile 在 CONFIG_CMD_NET 下编入 tftp.o。
- octeon_cmd_conf.h 启用 CONFIG_CMD_TFTPPUT、CONFIG_CMD_TFTPSRV；因此旧版 TFTP client/server 已有构建入口。
- PA-220 net/Makefile 不编入 wget.o、telnetd.o、tcp.o；没有对应 TCP 服务端/WGET对象。
- PA-220 octeon_pa220.h 未发现 CONFIG_SYS_DIRECT_FLASH_TFTP；本次按该固定板级配置未启用处理，最终 .config 仍需构建确认。

## 2. TFTP

### 2.1 uBootKit 实现
文件：u-boot-2016/net/tftp.c（1104 行）。
- 由 CONFIG_CMD_NET 编译；TFTP PUT 和 TFTP Server 分别由 CONFIG_CMD_TFTPPUT/CONFIG_CMD_TFTPSRV 条件编入。
- client GET：构造 RRQ、设置 UDP handler、接收 DATA、校验 block 序号、ACK 并写入 load_addr。
- client PUT：读取 save_addr 内存并发送 WRQ/DATA；只读取本地内存，不在本机写存储。
- TFTP Server：`tftp_start_server()`（约 970 行）设置 STATE_RECV_WRQ、UDP 端口 69 和 tftp_handler；收到 WRQ 后应答 ACK，后续 DATA 通过 store_block() 写入 load_addr。
- `store_block()`（175-222）默认写入 RAM；若 CONFIG_SYS_DIRECT_FLASH_TFTP，则根据 flash_info 判断目标是否为并行 NOR，命中后调用 flash_write()，这是持久化写路径。
- 下载地址与文件末端检查依赖 IPQ_TFTP_MIN_ADDR/IPQ_TFTP_MAX_ADDR、CONFIG_SYS_SDRAM_END、CONFIG_IPQ_FDT_HIGH、CONFIG_TZ_END_ADDR 等 IPQ/板级内存边界。
- TFTP 的数据缓冲区、block wrap、超时、重试、multicast 等状态大量使用文件级静态变量；同时只支持一个活动 TFTP 传输状态机。

### 2.2 PA-220 对照
- PA-220 net/tftp.c（950 行）同样支持 TFTP GET、CONFIG_CMD_TFTPPUT 和 CONFIG_CMD_TFTPSRV。
- PA-220 TFTP Server 入口为旧接口 `TftpStartServer()`；设置 STATE_RECV_WRQ、UDP handler，收到 WRQ 后将 DATA 写入 load_addr。
- 协议逻辑与 uBootKit 高度相似，变量/API 命名和 U-Boot 网络栈代际不同：TftpStartServer/TftpHandler/TftpBlock/TftpOurPort/NetTxPacket vs tftp_start_server/tftp_handler/tftp_cur_block/tftp_our_port/net_tx_packet。
- PA-220 `store_block()`（159-194）在默认路径直接 memcpy 到 load_addr；仅 CONFIG_SYS_DIRECT_FLASH_TFTP 时调用 flash_write()。该函数本身没有 uBootKit 新版那组 IPQ_TFTP_MIN/MAX 和 FDT/TrustZone 区域限制。
- PA-220 Stage 3 的 DRAM 已由原厂前级初始化；TFTP 将数据写入 RAM 仍必须避开 U-Boot、heap、stack、保留区和用户指定的 TFTP/RAM 启动加载地址。
- 适配判断：PA-220 已有 TFTP client/server，不需要移植 uBootKit tftp.c；应先补齐 PA-220 load_addr/文件末端边界保护，并确认最终配置未打开 direct-flash TFTP。TFTP PUT/Server 不应在未经授权时写 SPI/eMMC。

### 2.3 风险与状态
- TFTP Server 没有认证，任何可达的客户端都可能向 load_addr 写入任意内容；其作用是 RAM 上传，不是安全文件服务器。
- 若 CONFIG_SYS_DIRECT_FLASH_TFTP 打开，store_block() 可直接调用 flash_write()；这必须被视为持久化写路径。PA-220 固定板级头未发现该宏，但最终构建配置未核实。
- TFTP PUT 读取 save_addr 内存并向远端发送，属于数据外传能力；不等于本地持久化写。
- TFTP GET/Server 的 load_addr 和数据长度必须做 PA-220 专用内存范围验证。

## 3. WGET（HTTP 下载客户端）

### 3.1 入口和调用链
文件：u-boot-2016/net/wget.c（701 行）。
- 由 CONFIG_WGET 编入；命令入口 `do_wget()`（661-695），注册 U_BOOT_CMD(wget)。
- `start_wget()` 解析 URL，只支持 http://，不支持 HTTPS/IPv6；域名路径可在 CONFIG_CMD_DNS 下调用 DNS net_loop。
- `wget_loop()` 初始化或复用网卡和 TCP 状态，调用 tcp_connect()、tcp_start()，轮询 eth_rx()/tcp_periodic_check()。
- TCP 回调 wget_tcp_callback() 分派 NEW_CONN/DATA_RCVD/REMOTE_CLOSED/CLOSED。
- 收到响应头后 wget_parse_response() 要求 HTTP 状态码 200，解析 Content-Length；之后 wget_recv_data() 将数据复制到用户指定 load_addr。
- 完成时调用 flush_cache()、set_file_info_env()；后者会更新上传文件信息环境变量。未发现 saveenv()，所以此路径的环境变化是 RAM-only，但会改变当前全局环境。
- `wget_loop()` 还会设置 net_server_ip 并 setenv("serverip", ...)，不调用 saveenv()。

### 3.2 内存和协议边界
- 默认 load_addr：优先环境变量 loadaddr；否则根据 gd->ram_size 选择 SDRAM_BASE+256 MiB 或 IPQ_TFTP_MIN_ADDR。
- HTTP Content-Length 存在时调用 is_memory_region_available() 检查目标范围；缺少 Content-Length 时按 gd->ram_top-load_addr 或 SIZE_MAX 估算。
- 不支持 HTTPS/TLS、HTTP chunked transfer encoding、重定向和除状态码 200 外的响应。
- 响应头缓冲区按接收数据不断 realloc，未见固定最大头长度限制，恶意服务器可导致内存消耗。
- Content-Length 解析较简单且大小写敏感；长度、整数溢出、服务器实际发送长度必须继续审查。
- 只在接收完成时 set_file_info_env()，没有本地 SPI/eMMC 写操作；下载目标为 RAM。
- 依赖 TCP 子系统和 DNS（可选），不是单纯复用现有 UDP 网络栈即可获得的功能。

### 3.3 PA-220 对照
- PA-220 固定源码 net/Makefile 没有 wget.o，未发现 CONFIG_WGET 对应构建入口。
- PA-220 没有 uBootKit 新增 TCP 子系统；net/net.c 是旧 NetLoop UDP/ARP/ICMP/BOOTP/TFTP 框架。
- 因此 WGET 需要先有 TCP client 或独立 HTTP client；不能直接编译 uBootKit wget.c。
- PA-220 可借鉴 HTTP/1.0 GET、Content-Length、RAM 接收和内存范围检查思路；域名解析可研究已有 DNS client，但 DNS 结果环境变量、NetLoop 状态和 TCP 连接需适配。
- 适配结论：可作为后续独立功能重写，当前不属于可直接复用功能。初版只在 TFTP 可满足需求时不必增加 WGET。

## 4. TelnetD（TCP 远程命令行服务器）

### 4.1 构建和自动启动
- 文件：u-boot-2016/net/telnetd.c（880 行），头文件 include/net/telnetd.h。
- CONFIG_TELNETD 编入 telnetd.o；依赖 CONFIG_TCP 和 capture.h/call_func_capture。
- failsafe.c:136-139 在 CONFIG_TELNETD 下调用 `get_enable_state("telnet_en", true)`；若环境变量不存在，默认 true，即自动启动 TelnetD，默认端口由 telnetd_get_port_from_env() 解析，默认 23。
- 注意：telnetd.c 的 U_BOOT_CMD help 描述变量为 telnetd_enable，但 failsafe.c 实际检查的是 telnet_en，存在文档/实现不一致。

### 4.2 连接和命令执行
- `telnetd_start()` 调用 tcp_listen() 监听指定 TCP 端口；`telnetd_stop()` 调用 tcp_listen_stop()。
- 每个连接分配 telnetd_pdata，处理 Telnet IAC 协商、输入缓冲、命令编辑、输出队列。
- `telnetd_process_input()` 解析用户输入；`telnetd_execute()` 分配输出缓冲并调用 call_func_capture()。
- `telnetd_run_command()`（305-319）直接调用 run_command(p->cmd, 0)，然后输出 U-Boot prompt。
- 没有用户名/密码认证、没有 TLS、没有命令白名单；任何可连接者获得任意 U-Boot 命令执行能力。
- 由于 PA-220 命令环境包含 saveenv、sf、mmc、EEPROM、文件系统写入及 OCTEON bootloader update 命令，TelnetD 会成为绕过 RAM-only 和持久化写入审查的远程入口。

### 4.3 PA-220 对照与结论
- PA-220 net/Makefile 不编入 telnetd.o，也没有此 TCP server。
- PA-220 没有 uBootKit TCP 子系统/capture.h 适配实现。
- 不直接移植 TelnetD；PA-220 Web/网络维护服务初版不提供远程 shell。
- 如确有远程诊断需求，只设计白名单只读命令 RPC，并对每个命令逐项验证副作用；不能使用 run_command() 作为通用入口。

## 5. 本批结论

| 功能 | PA-220 源码状态 | 适配结论 | 持久化/安全 |
|---|---|---|---|
| TFTP GET | 已有旧版 client | 复用现有实现；补内存边界检查 | RAM 接收 |
| TFTP PUT | 已有旧版 client | 复用现有实现 | 向远端发送 RAM 数据 |
| TFTP Server | 已有旧版 WRQ server | 复用现有实现；审查内存范围和启用条件 | 默认 RAM 写；direct flash 宏可改为 Flash 写 |
| WGET | 无对应实现/TCP client | 需重写 | 下载到 RAM；会改运行时 env |
| TelnetD | 无对应 TCP server | 不移植任意命令远程执行 | 高风险远程持久化写入入口 |

本批未编译、未实机测试、未执行 TFTP direct flash、SPI/eMMC/NAND/EEPROM/CPLD 写入。
