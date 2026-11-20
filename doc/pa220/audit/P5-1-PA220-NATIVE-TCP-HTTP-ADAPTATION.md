# P5-1：PA-220 原生 TCP 接收与只读 HTTP 服务适配审计

- 日期：2026-11-20
- 状态：**源码静态审计完成；实现未开始；未编译、未实机验证**
- 集成分支：`feature/pa220-ubootkit-web`
- 本次审计时分支 HEAD：`0ff842868fdd05628dcb81391d83a2765bddf801`
- PA-220 固定源码：`IronDemiGod/pa220-openwrt@801717ba6a8a59ed2cfd53bf3e1b19841fecd2e8`
- uBootKit 固定审计源码：`9ce315735ac88fd0a7c7697bc527a8616ed0a972`

本报告是 P5 专项适配审计，不重做 P1/P2/P3/P4。它把 P1-A1 已完成的 uBootKit HTTPD/TCP 架构审计与 PA-220 固定源码对照，收敛为 P5-1 的实现入口和安全边界。

## 1. PA-220 原生网络栈现状

审计文件：PA-220 固定源码 `u-boot/net/net.c`、`u-boot/net/eth.c`、`u-boot/include/net.h`、`u-boot/net/Makefile`。

- `eth.c` 提供设备注册/选择、`eth_init()`、`eth_rx()`、`eth_send()`、`eth_halt()`。驱动的接收回调最终进入 U-Boot 网络接收处理。
- `net.c:NetLoop()` 是阻塞式协议主循环：初始化网络和 Ethernet，反复调用 `eth_rx()`，检查 Ctrl-C、ARP 超时和一个全局 timeout handler，并根据 `net_state` 结束。它不是可供 HTTPD 直接注册的通用 TCP 服务循环。
- `NetReceive()` 解析 Ethernet/VLAN/IPv4，检查 IPv4 头和校验和、目的地址并处理分片；当前 IPv4 传输层只分派 ICMP 与 UDP。其他 IP protocol（包括 TCP protocol 6）被丢弃。
- UDP/ARP handler 是单一全局回调；`NetSetTimeout()` 也是单一全局 timeout callback。不能让 TCP 服务与现有 UDP 命令并行抢占这些状态。
- `NetRxPacket` 指向当前接收缓冲区；驱动继续收包后该缓冲区可被复用。TCP 层不得长期保存 RX packet 内部指针，必须在回调中校验并复制所需字段/数据。
- `NetSendPacket()`（`net.h` inline）可通过 `eth_send()` 发送原始 Ethernet frame；`NetSetEther()`、`net_set_ip_header()`、`NetCksum()` 可作为组帧基础，但当前没有 TCP header/checksum/state/retransmission 实现。
- `net/Makefile` 只编入旧网络对象，没有 TCP 或 HTTPD 对象；固定 PA-220 树不存在 uBootKit HTTPD/TCP 服务模块。

## 2. TCP 接收处理适配入口

建议采用最小、显式的网络层扩展，而不是把新 TCP 服务塞入 UDP handler：

1. 在 `include/net.h` 定义 TCP 输入回调/注册接口（或等价内部接口），参数至少包含 Ethernet frame、IPv4 header、TCP segment 的只读视图及长度。
2. 在 `net.c:NetReceive()` 的 IPv4 分派处识别 `IPPROTO_TCP (6)`，完成已有 IP 校验后，将边界受检的 TCP segment 分派给 TCP 层；不得改变 ARP、ICMP、UDP 的既有路径。
3. TCP 回调只在当前 `eth_rx()` 调用栈内访问 RX packet；需要跨轮询保存的数据必须复制进固定容量的连接状态/接收缓冲区。
4. TCP 输出由 TCP 层构造 IPv4/TCP 头和伪首部校验和，再通过原生 `NetSendPacket()` 发出。连接初期保存对端 MAC；首版服务限制为同一 MGT 二层维护网段，不能假定具备通用路由/邻居缓存。
5. 单连接、静态内存、明确最大 MSS/窗口/请求长度是首版较稳妥的范围。必须实现并审查 SYN/SYN-ACK/ACK、数据序号/确认号、FIN/RST、超时和重传；不能把“能响应 SYN”视为可用 TCP。

TCP 层不得依赖 IPQ PPE/GMAC、IPQ cache flush、固定 SDRAM 地址、TrustZone/FDT 内存边界或 Qualcomm 寄存器。

## 3. HTTP 服务生命周期

首版采用显式 CLI 生命周期：`pa220 web` 启动，在前台运行，Ctrl-C 退出；不自动进入 Web、不挂接 autoboot、不在后台常驻。

建议生命周期：

1. **前置检查**：确认当前 Ethernet 设备存在、MAC 有效、`ipaddr` 非零且可用；服务仅绑定 PA-220 当前 MGT 接口和配置 IP。缺少前置条件时不启动。
2. **启动**：初始化 TCP/Web 静态状态；调用旧网络栈初始化入口；按 `NetLoop()` 的既有顺序选择当前 Ethernet 并初始化设备。不得同时运行另一个 NetLoop 协议命令。
3. **运行**：前台轮询 `eth_rx()`、TCP 定时/重传处理和 watchdog；不使用全局 `NetSetTimeout()` 作为 TCP 私有定时器，以免覆盖现有网络状态。Ctrl-C 是正常退出入口。
4. **退出/异常清理**：清除 TCP listener/连接状态和接收缓冲区；清除新增回调；停止 Ethernet；确保 UDP/ARP handler、NetLoop 状态及 CLI 可以被下一条命令重新使用。所有失败分支走同一个幂等 cleanup。
5. **autoboot 兼容**：首版只能由用户在 CLI 显式启动；服务退出后返回 CLI，随后原有 autoboot 逻辑不被改写。不得自动修改环境变量或调用 `saveenv`。

需要特别审查旧版 `NetLoop()` 与 Web 前台循环的初始化/停止顺序，尤其是 Ethernet 当前设备、ARP pending 状态、单例回调和 Ctrl-C 的交互。

## 4. HTTP parser 与只读 handler 的最小范围

- 固定静态接收缓冲区；请求行、header 总长、header 数量、URI 长度均设硬上限。
- 第一版只接受 `GET`（可选 `HEAD`）；拒绝 POST/PUT/PATCH/DELETE、任何 request body、chunked transfer、multipart、文件上传和未知方法。
- 只支持明确 allowlist URI；拒绝绝对 URI、路径穿越、重复/冲突 Content-Length、畸形 header 和超限请求。
- 每个响应固定 Content-Length、Content-Type、Connection: close；每连接完成一次请求后关闭，避免首版引入 keep-alive 状态复杂度。
- 只读 handler 通过静态注册表绑定函数，不提供任意命令字符串执行、shell/CLI bridge 或可写 handler。
- 可先实现静态首页和有限 JSON 状态查询。板卡/内存/网络信息应调用已审查的只读数据接口；不直接把 CLI 输出重定向到 HTTP。
- 不暴露环境变量全量导出、MAC 修改、网络配置修改、日志清除或其他带副作用的业务模块。

## 5. uBootKit 参考代码的可复用边界

P1-A1 已审计 uBootKit 固定源码 `net/tcp.c`、`net/httpd.c`、`failsafe/failsafe.c` 及头文件：

- 可参考 TCP 状态机、HTTP 请求/响应解析、URI handler 注册和页面资源组织。
- 不能直接假设其 TCP callback ABI、RX buffer 生命周期、定时器、分配器、网络主循环与 U-Boot 2013.07 一致。
- 不移植 Qualcomm/IPQ 专有网络驱动、PPE/GMAC、SoC 寄存器、固定 IPQ 地址、TrustZone/FDT 内存判定、SMEM/QCA MMC host、IPQ cache API。
- HTTP 上传区、RAM 接收和文件系统业务均属于 P6 或之后阶段，不进入本次 P5-1。

## 6. 第一版只读安全边界

明确禁止：任意 U-Boot 命令执行；环境变量 set/unset/reset/restore；`saveenv`；SPI/eMMC/NAND/EEPROM/CPLD 写入；文件上传；固件安装；重启/复位/看门狗触发；网络配置修改；任何通过 HTTP 引发持久化或其他不可逆副作用的路径。

允许：经过逐项审查的只读板卡、内存、当前网络配置摘要和有限存储识别状态。即使是状态查询，也必须确认底层 API 不会初始化/改变硬件状态或清除状态位。

P6 的 RAM 文件接收、统一 `__tmp_load` validator、上传缓冲区和校验功能全部不在 P5 范围内。

## 7. 尚未验证

- 本报告只完成固定源码静态审计；没有 TCP/HTTP 代码、补丁或配置变更。
- 没有执行编译、链接 map/ELF 核查、RAM 启动或网口实测。
- 未验证 PA-220 Ethernet 驱动对高频轮询、持续连接和异常 TCP 流量的表现。
- 未验证单连接 TCP 状态机的重传、乱序、分段、校验和、超时和断线清理。
- 未验证 HTTP parser 的边界、畸形请求、重复连接和资源耗尽防护。
- 未验证 Web 启停后 DHCP/Ping/TFTP 既有命令、CLI 和 autoboot 恢复。
- MGT 同网段访问约束和绑定策略仍需实现时核实；不宣称已有网络隔离或访问控制。

## 8. P5-1 下一步

按小提交推进，避免与并行 P4 对 `net.c` / `net.h` / `net/Makefile` 的修改冲突：

1. 先新增独立 `tcp.c` / `include/net/tcp.h`，实现静态单连接 TCP 状态机和单元级可审查的包解析/校验；暂不接入 `NetReceive()`。
2. 再单独提交最小 IPv4 TCP 分派 hook；合并前必须重新读取分支 HEAD 和 P4 当前 diff/提交，手工整合，不覆盖 P4。
3. 增加 `pa220 web` 前台服务生命周期和统一 cleanup。
4. 在独立 HTTP 文件中实现 bounded parser、静态 URI handler registry 和纯只读页面/状态 handler。
5. 编译验证、map/ELF 对象核查、RAM 启动与隔离 MGT 网口测试分别记录；实机通过前状态保持 NOT VERIFIED。

**本报告不表示 P5-1 实现完成。** 当前可确认的收敛结论是：PA-220 原生网络栈缺少 TCP 分派和 TCP 服务循环；最小接入点是 `NetReceive()` 的 IPv4 protocol 分派，服务生命周期应由显式前台 `pa220 web` 命令管理。
