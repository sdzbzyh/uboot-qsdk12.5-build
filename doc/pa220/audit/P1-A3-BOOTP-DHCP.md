# P1-A3：网络服务审计（第一批：网络构建配置、BOOTP/DHCP Client、DHCP Server）

- 状态：**本批源码静态审计完成；P1-A3 整体进行中；未编译、未实机验证**
- 日期：2026-11-18
- uBootKit：chenxin527/uboot-qsdk12.5-build@9ce315735ac88fd0a7c7697bc527a8616ed0a972
- PA-220：sdzbzyh/pa220-openwrt@801717ba6a8a59ed2cfd53bf3e1b19841fecd2e8
- 范围：net/Makefile、net/Kconfig、net/bootp.c、net/dhcpd.c、include/net/dhcpd.h；对照 PA-220 net/Makefile、net/bootp.c、net/net.c 和 octeon_cmd_conf.h。
- 后续 P1-A3 仍需审计 TFTP、WGET、TelnetD、eth/net 主循环及 net_abort。

## 1. 构建条件与 CONFIG

uBootKit 固定 commit：
- net/Makefile:11-18：
  - CONFIG_CMD_NET 编入 arp.o、bootp.o、eth.o、net.o、tftp.o。
  - CONFIG_CMD_DHCPD 编入 dhcpd.o（实际宏名为 CONFIG_DHCPD）。
  - CONFIG_HTTPD 编入 httpd.o。
  - CONFIG_TCP 编入 tcp.o。
  - CONFIG_TELNETD 编入 telnetd.o。
  - CONFIG_WGET 编入 wget.o。
  - CONFIG_NET_ABORT 编入 net_abort.o。
- net/Kconfig 仅定义 NET、NET_RANDOM_ETHADDR、NETCONSOLE、NET_TFTP_VARS；没有在该文件定义 CONFIG_DHCPD、CONFIG_TCP、CONFIG_TELNETD、CONFIG_WGET 或 CONFIG_NET_ABORT。它们的配置来源需在其他 Kconfig/defconfig/板级配置中继续确认。
- failsafe.c 在 CONFIG_DHCPD 下调用 dhcpd_start()/dhcpd_stop()；即 DHCP server 不仅编入，还由 Web failsafe 主循环启停。

PA-220 固定 commit：
- net/Makefile:12-23 使用旧式 COBJS-$(CONFIG_CMD_NET) 编入 bootp.o、eth.o、net.o、tftp.o；没有 dhcpd.o、tcp.o、httpd.o、telnetd.o、wget.o、net_abort.o。
- octeon_cmd_conf.h:12-25 在未定义 OCTEON_NO_NETWORK / CONFIG_OCTEON_DISABLE_NETWORKING 时定义 CONFIG_CMD_NET、CONFIG_CMD_DHCP、CONFIG_CMD_TFTPPUT、CONFIG_CMD_TFTPSRV、CONFIG_CMD_PING、CONFIG_CMD_DNS 等。
- 因而 PA-220 已配置 DHCP Client 和 TFTP client/server 命令，但没有 uBootKit 的 DHCP Server/TCP/HTTP/TelnetD/WGET 服务对象。
- 以上是源码/配置证据，不等于目标硬件网络功能已编译或实测。

## 2. BOOTP/DHCP Client

### 2.1 uBootKit 路径
- 文件：u-boot-2016/net/bootp.c。
- 由 CONFIG_CMD_NET 编译；若 CONFIG_CMD_DHCP 定义，则同一 BOOTP 文件中的 DHCP 状态机和 DHCP option 处理路径启用。
- `bootp_reset()` 初始化事务 ID 缓存、重试次数、起始时间和超时。
- `bootp_request()` 构造 BOOTP/DHCPDISCOVER 请求，填充客户端 MAC、transaction ID、DHCP option list，设置超时和 UDP handler，发送广播包。
- 接收路径 `bootp_handler()` / `dhcp_handler()` 检查回复、匹配 transaction ID、解析 BOOTP/DHCP options，将地址、server、网关、DNS、hostname 等信息写入网络运行时全局状态。
- DHCP option 编码支持 subnet mask、router、DNS、hostname、bootfile、bootpath、NIS/NTP、PXE 等按 CONFIG_BOOTP_* 条件编入的选项。
- 主要全局状态：bootp_ids[]、bootp_num_ids、bootp_try、bootp_start、bootp_timeout、dhcp_leasetime、dhcp_server_ip、dhcp_option_overload，以及 net_ip/net_server_ip/net_gateway/net_netmask/net_dns_server 等网络状态。
- 这是 DHCP Client；不等同于 DHCP Server。它主要修改 RAM 中网络状态，最终是否写入环境由上层网络命令/流程决定。

### 2.2 PA-220 对照
- PA-220 u-boot/net/bootp.c 同样实现 BOOTP/DHCP Client，包含 BootpRequest、BootpHandler、DhcpHandler、DHCP option 处理和重试机制。
- 两版协议逻辑高度相似，但 API/命名属于不同代 U-Boot：
  - uBootKit 使用 bootp_request、net_tx_packet、net_set_ether、net_set_udp_header、net_send_packet、net_set_timeout_handler 等；
  - PA-220 使用 BootpRequest、NetTxPacket、NetSetEther、NetSendPacket、NetSetTimeout 等旧接口，同时部分函数仍使用小写 net_*。
- PA-220 net/net.c 的 NetInitLoop()（258-284）从 getenv_IPaddr("ipaddr")、getenv_IPaddr("serverip") 等环境读取并初始化 NetOurIP、NetServerIP 等运行时状态。
- PA-220 NetLoop()（324 起）负责 net_init、eth_init、协议状态机、eth_rx、超时和 eth_halt；uBootKit 的 client 仍运行在 U-Boot 网络协议框架内。
- 适配结论：PA-220 DHCP Client 已有等价实现，无需移植 uBootKit bootp.c；需要的是核对现有配置、运行时环境同步和设备网卡状态。不要将两个版本的网络全局变量/API 混用。

### 2.3 状态
- 源码存在：uBootKit 与 PA-220 均有 BOOTP/DHCP Client。
- 配置启用：PA-220 octeon_cmd_conf.h 定义 CONFIG_CMD_NET、CONFIG_CMD_DHCP；实际最终 .config 仍需构建确认。
- 编译成功：NOT VERIFIED。
- 实机验证：NOT VERIFIED。

## 3. DHCP Server（dhcpd.c）

### 3.1 服务入口和调用链
- 文件：u-boot-2016/net/dhcpd.c（932 行），头文件 include/net/dhcpd.h。
- 构建开关：CONFIG_DHCPD；failsafe.c 在该宏下调用 dhcpd_start()，退出时若 dhcpd_is_running() 则 dhcpd_stop()。
- `dhcpd_start()`：若 net_ip/net_netmask/net_gateway/net_dns_server 未设置，填入默认值；计算地址池；清空 8 个静态租约槽；保存原 UDP handler 并安装 dhcpd_udp_handler。
- `dhcpd_udp_handler()`：先调用 dhcpd_handle_packet()，再调用之前的 UDP handler。
- `dhcpd_handle_packet()`：检查服务运行状态、UDP 67/68 端口、BOOTP 固定头长度、BOOTREQUEST、Ethernet htype/hlen、DHCP magic cookie 和 message type。
- DISCOVER：dhcpd_handle_discover() 查找或分配 IP，调用 dhcpd_send_reply() 发 DHCPOFFER。
- REQUEST：dhcpd_handle_request() 解析 Server Identifier 和 Requested IP，校验子网/地址池/冲突后发 DHCPACK 或 DHCPNAK。
- `dhcpd_stop()`：仅当当前 UDP handler 仍是 dhcpd_udp_handler 时恢复原 handler，清空 running 状态。

### 3.2 地址池与租约
- 默认服务器地址 192.168.1.1，默认掩码 255.255.255.0，默认池 192.168.1.100–192.168.1.200。
- 动态地址池按 net_ip/net_netmask 计算，起始偏移 DHCPD_POOL_OFFSET=99，目标池大小 DHCPD_POOL_SIZE=101。
- 最大客户端数 DHCPD_MAX_CLIENTS=8。
- 通过客户端 MAC 的 FNV-1a 哈希确定起始地址，线性探测冲突。
- DHCP OFFER/ACK 发送 lease time 3600 秒。
- lease 结构仅有 used、MAC、IP；没有到期时间字段，也没有 DHCPRELEASE/DECLINE 处理；租约不按 3600 秒到期回收，只在 dhcpd_start() 清空或运行时重新分配。该行为与通告租期不一致。
- 地址池耗尽时 dhcpd_alloc_ip() 的回退分支可能返回 pool_start_host，即使该地址已经被占用；所有租约槽耗尽时也会返回池首地址，存在重复分配风险。

### 3.3 网络状态、全局副作用和协议边界
- 不调用 setenv()/saveenv()；服务器地址池和租约只存在 RAM 全局状态。
- 会修改全局 UDP handler 指针，且链式调用旧 handler；需要防止与正在运行的 NetLoop/DHCP Client/其他 UDP 服务争用。
- dhcpd_send_reply() 通过 net_tx_packet、net_set_ether、net_set_udp_header、net_send_packet 直接构造广播 OFFER/ACK/NAK。
- 服务器没有持久化租约数据库；无 DNS 服务本体，只在 DHCP option 中通告 DNS 地址。
- 仅处理 DHCPDISCOVER 和 DHCPREQUEST；不处理 DECLINE、RELEASE、INFORM 等消息。
- 没有认证或防止恶意客户端耗尽租约槽的机制。

### 3.4 源码问题/待验证
- DHCPD_POOL_OFFSET=99 对不同子网掩码并不总合理；动态池无法计算时回退默认 192.168.1.100–200，可能与当前网段不匹配。
- 8 个租约槽与 101 个地址池不匹配；租约槽耗尽后回退可能产生重复地址。
- 租约时间只在报文中通告，内部没有过期回收。
- dhcpd_udp_handler() 处理后仍调用 prev_udp_handler；需检查旧 handler 是否也会处理同一 DHCP 报文。
- dhcpd_start() 只检查 net_ip 等运行时状态，不直接从 PA-220 环境变量读取；PA-220 端口/地址初始化时序必须重新设计。
- DHCP Server 在 Web failsafe 主循环中运行；不能直接在 PA-220 NetLoop() 正在执行另一个协议时简单挂接，否则 UDP handler 和 eth_rx 所属循环可能冲突。

### 3.5 PA-220 适配结论
- PA-220 没有 dhcpd.c/服务端对象，也没有 CONFIG_DHCPD 编入规则。
- 其旧网络栈具备 UDP handler 注册、UDP 包发送、NetTxPacket/NetRxPacket、eth_rx 等基础能力，但接口命名、packet 生命周期、定时器和主循环与 uBootKit 2016 有差异。
- 如需 DHCP Server，可参考 dhcpd.c 的报文状态机和 option 处理，针对 PA-220 旧网络栈重写；必须与 NetLoop/HTTP/TFTP 服务统一调度，不可独立抢占 UDP handler。
- 第一版是否需要 DHCP Server 取决于 MGT 维护网络方案；不能默认启动或改变现有管理口 IP/网关。
- 适配判断：需要重写；非直接复用。租约过期、地址冲突、池配置、请求合法性和多服务共存必须先修正/验证。

## 4. 本批范围与后续

已完成：网络 Makefile/Kconfig 边界、BOOTP/DHCP Client 对照、DHCP Server 主调用链、状态与租约模型、PA-220 对应接口初查。

尚未完成：TFTP、WGET、TelnetD、eth.c、net.c、net_abort.c 及网络驱动/HTTP/DHCP 服务调度整体关系。

未执行：构建、网口实测、DHCP 租约测试、任何持久化写入。
