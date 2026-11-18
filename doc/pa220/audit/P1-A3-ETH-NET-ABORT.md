# P1-A3：网络服务审计（第三批：eth/net 主循环、TCP 接入、net_abort）

- 状态：**本批源码静态审计完成；P1-A3 网络服务源码审计完成；未编译、未实机验证**
- 日期：2026-11-18
- uBootKit：chenxin527/uboot-qsdk12.5-build@9ce315735ac88fd0a7c7697bc527a8616ed0a972
- PA-220：sdzbzyh/pa220-openwrt@801717ba6a8a59ed2cfd53bf3e1b19841fecd2e8
- 前置：P1-A3-BOOTP-DHCP.md、P1-A3-TFTP-WGET-TELNET.md；P1-A1 已审计 TCP 核心协议实现。
- 范围：net/eth.c、net/net.c、net/net_abort.c、common/autoboot.c；对照 PA-220 net/eth.c、net/net.c、drivers/net/octeon_eth.c、board/octeon/pa220/pa220_board.c。

## 1. uBootKit eth.c：网络设备抽象

- 文件：u-boot-2016/net/eth.c（1158 行）。
- 同时包含 DM Ethernet 和旧式 eth_device 两套路径。
- DM 路径通过 uclass/device/eth_ops 调用驱动的 start/stop/send/recv；legacy 路径维护 eth_devices 环形链表和 eth_current。
- 通用入口：eth_initialize()、eth_set_current()、eth_init()、eth_halt()、eth_send()、eth_rx()、eth_get_name()。
- eth_init() 从 ethact/ethprime 选择设备，调用驱动 start/init；eth_rx() 调用当前设备 recv；eth_send() 调用 send。
- MAC 地址通过 eth_getenv_enetaddr_by_index() 读取环境；eth_write_hwaddr() 在需要时调用驱动 write_hwaddr()。这是运行时硬件寄存器配置，不等于写 EEPROM。
- uBootKit eth.c 中 IPQ/PPE/DM 与通用 legacy 部分并存；PA-220 不应整文件替换。

## 2. PA-220 Ethernet driver

### 2.1 配置和编入
- PA-220 octeon_pa220.h:73-89 配置 OCTEON SGMII/QSGMII/MGMT Ethernet，并定义 CONFIG_OCTEON_INTERNAL_ENET。
- octeon_network.h 定义 CONFIG_OCTEON_ETH、CONFIG_NET_MULTI、CONFIG_MII、CONFIG_UDP_CHECKSUM、CONFIG_PHYLIB、BOOTP 选项等。
- drivers/net/Makefile 以 CONFIG_OCTEON_ETH 编入 octeon_eth.o，以 CONFIG_OCTEON_MDIO 编入 octeon_mdio.o。
- eth.c 使用旧式 eth_device 链表；eth_initialize(bd_t *) 调用 board_eth_init 或 cpu_eth_init，再遍历注册设备并设置 MAC。
- octeon_eth.c 实现 octeon_eth_initialize()、octeon_eth_init()、octeon_eth_send()、octeon_eth_recv()、octeon_eth_halt()、octeon_eth_write_hwaddr()；底层依赖 CVMX IPD/PIP/PKO、QLM、MDIO/PHY 等 OCTEON 硬件 API。
- 目前确认驱动源码、CONFIG_OCTEON_ETH 配置和 Makefile 编入关系；本次未完全确认 PA-220 最终 board_eth_init/cpu_eth_init 的实际链接调用点，标记 NOT VERIFIED。不能将源码编入等同于已初始化或实机网卡工作。

### 2.2 PA-220 板级网络初始化和 CPLD 边界
- pa220_board.c:120-138 early_board_init() 读取 TLV EEPROM 板卡描述和 MAC base，并调用 octeon_board_get_mac_addr()。
- pa220_board.c:160-165 board_configure_qlms() 配置 DLM0 为 QSGMII/QSGMII，供前面板 88E1680 端口使用。
- pa220_board.c:169-187 board_prepare_os_handoff() 在离开 U-Boot 时通过 CPLD reg 5 bit2 将 88E1680 置于 reset，并清除 CPLD reg7 bit2 的 U-Boot ALM LED。
- 这些是 PA-220 既有板级硬件操作；本审计未提出或执行新增 CPLD 写操作。新增 Web 网络功能不得自行改变 QLM、PHY reset 或 CPLD flash-select。
- PA-220 前面板 QSGMII 和 MGT AGL 的 OCTEON driver 是现有底层网络基础；不得引入 IPQ PPE、QCA PHY 或 IPQ MAC 初始化代码。

## 3. uBootKit net.c：网络主循环与协议分发

- 文件：u-boot-2016/net/net.c（1695 行）。
- 全局网络状态包括 net_ip、net_server_ip、net_gateway、net_netmask、net_dns_server、net_ethaddr、UDP/ARP/ICMP handlers、timeout handler、net_state。
- `net_init()`（386-415）首次分配对齐 TX/RX packet buffers、初始化 ARP、清理 handlers；每次调用 net_init_loop() 同步当前 Ethernet MAC。
- `net_loop()`（417 起）初始化/选择网卡、执行协议 prerequisite 检查、设置协议初始状态并运行 eth_rx 主循环。
- 在 CONFIG_TCP 下，net_loop 支持 TCP 协议状态；循环中调用 tcp_periodic_check()，并由 TCP 状态改变 NETLOOP_SUCCESS/FAIL。
- `net_process_received_packet()`（1142 起）解析 Ethernet/VLAN/IP/ICMP/UDP；CONFIG_TCP 下遇到 IPPROTO_TCP 调用 receive_tcp()，否则 UDP 交给当前 udp_packet_handler。
- net_set_udp_handler()/net_get_udp_handler()、net_set_timeout_handler()、net_send_udp_packet() 提供可复用的基础 UDP handler/定时器/发送接口。
- net.c 同时包含 IPQ/PPE、MMC、SDHCI、SMEM/HTTPD/TCP 集成；不是纯粹可复制的通用网络层。

## 4. PA-220 net.c：旧 NetLoop 对照

- 文件：u-boot/net/net.c（1507 行）。
- 全局网络状态使用 NetOurIP、NetServerIP、NetOurGatewayIP、NetOurSubnetMask、NetOurDNSIP、NetOurEther、NetTxPacket、NetRxPackets。
- `NetInitLoop()`（258-280）通过 get_env_id() 检测环境变化；环境 ID 变化时从 ipaddr/gatewayip/netmask/serverip/dnsip 更新运行时网络状态。
- `net_init()`（294-320）分配对齐 packet buffer、初始化 ARP、清理 UDP/ARP/timeout handlers。
- `NetLoop()`（324 起）调用 eth_init(gd->bd)，再按协议分派 TFTPGET/TFTPPUT/TFTPSRV/DHCP/BOOTP/PING/NFS/DNS 等状态机，循环中调用 eth_rx()。
- PA-220 的 eth_init() 需要 bd_t *bis 参数；uBootKit 新版 eth_init() 不带参数，网络设备/状态接口属于不同代 U-Boot。
- PA-220 net_process_received_packet() 对 IP 层只处理 ICMP 和 UDP；没有 IPPROTO_TCP 分支、receive_tcp()、tcp_start() 或 tcp_periodic_check()。
- 因此 PA-220 具备成熟的 Ethernet/ARP/IP/ICMP/UDP/NetLoop 基础，但不具备 uBootKit 新增 TCP 子系统。HTTPD、WGET、TelnetD 均不能只移植 handler；必须先移植/重写 TCP transport 和 net.c TCP 收包/调度路径。
- 环境变量变更由 NetInitLoop() 在下一次同步时刷新运行时网络状态；这不意味着 Web handler 修改环境后会自动重新启动网卡或立即改变现有 TCP 会话。

## 5. net_abort：网络包中止 autoboot

### 5.1 构建和调用链
- 文件：u-boot-2016/net/net_abort.c（151 行），由 CONFIG_NET_ABORT 编入 net_abort.o；include/net_abort.h 提供接口。
- net/Kconfig 未定义 CONFIG_NET_ABORT；具体开关来源/目标配置需继续确认。
- 实际调用点：u-boot-2016/common/autoboot.c:233、276-295。
- abortboot_normal() 在 autoboot 倒计时开始时调用 net_abort_prepare()；若准备成功，倒计时期间循环调用 eth_rx()，检查 net_abort_detected()。
- 收到中止包后终止 autoboot，随后调用 net_abort_finish()。
- net_abort_prepare() 先检查 get_enable_state("net_abort_en", true)；默认 true。随后 eth_init()、net_init()、arp_probe()，安装 net_abort_udp_handler 和 100ms timeout handler。
- 监听 UDP 37541，payload 必须精确等于字符串 `UBOOT:ABORT`；未验证发送方 IP/MAC，也没有认证。
- net_abort_finish() 清理 UDP/timeout handler；若收到 magic 包，广播回复 `UBOOT:ABORTED`，执行 run_command("httpd", 0)，再进入 cli_loop()；否则 eth_halt()。
- 这不是普通 DHCP/TFTP 服务，而是远程中断 autoboot 并进入 Web failsafe/命令行的网络入口。

### 5.2 安全和 PA-220 适配
- 无认证 UDP magic packet 可被同网段设备触发；会阻止正常 autoboot，并进入 U-Boot 命令行/HTTP failsafe。
- net_abort_prepare() 只在 CONFIG_NET_ABORT 编译并由 autoboot 调用时生效；是否在目标 uBootKit defconfig 启用尚未确认。
- PA-220 没有 net_abort.o/CONFIG_NET_ABORT 对应构建入口，也没有 HTTPD/TCP 服务。
- 可以参考“autoboot 窗口内监听 UDP 维护触发包”的思路，但 PA-220 不应直接开放无认证远程 CLI；若需要恢复入口，应设计独立白名单/认证策略并限制到管理网口。
- net_abort 本身不执行 SPI/eMMC 写入；但它能进入可执行任意命令的 U-Boot CLI/HTTPD，因此会间接打开持久化写入攻击面。

## 6. P1-A3 网络服务最终适配矩阵

| 功能 | PA-220 源码/配置状态 | 结论 |
|---|---|---|
| Ethernet driver | OCTEON eth_device driver、CONFIG_OCTEON_ETH、QSGMII/AGL 板级配置存在 | 保留现有驱动；不移植 IPQ eth/PPE |
| ARP/IP/ICMP/UDP | 旧 net.c/NetLoop 已有 | 复用 PA-220 现有网络栈 |
| DHCP Client/BOOTP | 旧 bootp.c + CONFIG_CMD_DHCP | 已有等价实现 |
| DHCP Server | 无 dhcpd.o/服务端实现 | 参考协议逻辑后重写，统一 NetLoop 调度 |
| TFTP GET/PUT/Server | 旧 tftp.c + CONFIG_CMD_TFTPPUT/TFTPSRV | 已有等价实现；补充 load_addr 内存边界审查 |
| WGET | 无 wget.o/TCP client | 需独立实现，先解决 TCP |
| HTTPD/TCP | PA-220 无 TCP 收包/服务子系统 | 需移植/重写 TCP transport 和 net.c 调度 |
| TelnetD | 无服务端；uBootKit 无认证任意命令执行 | 不移植 |
| net_abort | 无对应模块 | 默认不移植无认证远程 CLI 入口 |

## 7. 本阶段边界

- P1-A3 完成网络服务源码静态审计；未改变 PA-220 网络驱动、QLM、PHY、CPLD 或网络配置。
- DHCPD、TFTP、WGET、TelnetD、net_abort 均未在 PA-220 编译/实机验证。
- 未执行任何 SPI/eMMC/NAND/EEPROM/CPLD 持久化写入。
- P1-A3 完成不代表 HTTPD/TCP 已移植；P1-A1 仍是源码审计结论。
