# P5-1A TCP 核心实现进度：单连接状态机（P5-1A.2）

- 日期：2026-11-20
- 分支：`feature/pa220-ubootkit-web`
- PA-220 固定源码：`IronDemiGod/pa220-openwrt@801717ba6a8a59ed2cfd53bf3e1b19841fecd2e8`
- 本提交性质：独立、静态单连接 TCP 核心；尚未接入网络分派。

## 本提交实现

- 单连接静态结构，无动态内存分配；LISTEN、SYN-RECEIVED、ESTABLISHED、FIN-WAIT-1/2、CLOSE-WAIT、CLOSING、LAST-ACK、TIME-WAIT、CLOSED 状态。
- 主动监听配置、本地/远端 IPv4 和端口绑定；被动 SYN、带 MSS 的 SYN-ACK、最终 ACK 验证。
- 32 位序列号/确认号管理，拒绝超前 ACK 和旧 ACK；仅接受严格按序报文序列检查，拒绝超出本地 MSS 或剩余接收窗口的数据；未协商的 URG/ECN/保留位报文拒绝处理。
- 固定 4096-byte 接收缓存；`tcp_core_read()` 消费缓存并释放窗口。缓存空间不会动态扩展。
- ACK、PSH 数据发送接口（单个 outstanding 发送范围）、累计 ACK 处理及部分 ACK 后剩余数据重传；发送长度受协商 MSS、1460-byte TX 上限及对端通告窗口约束。
- FIN、RST 处理；RST 仅在连接四元组匹配且序列号等于 RCV.NXT 时清理连接。
- 独立调用方时钟：`tcp_core_poll(now_ms)`；SYN-ACK/数据/FIN 使用单一待确认范围，固定 1000ms 重传间隔，最多 5 次重传；连接空闲 30s 清理，TIME-WAIT 60s 清理；清理后回到 LISTEN，并推进本地 ISS。
- 输出回调同步消费 TCP segment；报文由连接自身静态 wire buffer 构造；重传数据复制到连接私有缓存，不保留 RX packet 指针。

## API 与所有权

- `tcp_core_listen()` 初始化 listener 和初始发送序列号。
- `tcp_core_input()` 处理一段经过 IPv4 层交付的 TCP segment；调用方仍须保证 IPv4 目的地址与 segment 的校验和输入正确。
- `tcp_core_poll()` 由未来前台服务循环提供单调毫秒计数（允许 u32 回绕）。
- `tcp_core_read()` 将已接收字节复制给调用方并释放接收窗口。
- `tcp_core_write()` 复制发送数据，不保留调用方指针；返回发送字节数、0 表示对端窗口暂不可用、负值表示拒绝/发送失败。
- `tcp_core_close()` 发起 FIN。
- emit 回调参数 segment 仅在回调期间有效；回调必须同步复制/提交，不能保存指针。

## 限制与待处理

- 本提交未接入 `NetReceive()`、`NetLoop()`、Ethernet 驱动或 Makefile。
- 只实现单一 outstanding 发送范围，不支持发送队列、SACK、窗口缩放、时间戳、拥塞控制、零窗口探测和延迟 ACK；不支持乱序重组。
- 应用读取数据后不会主动发送独立 window-update ACK，窗口更新在下一次协议响应中通告。
- 被动关闭路径已实现；TIME-WAIT 的 2MSL 在当前策略中固定为 60s。
- 当前代码仍需针对 RFC 边界场景补充单元测试；未执行目标交叉编译、链接、RAM 启动或实机测试。不得标记为编译通过或 TCP 核心完成。
- 本阶段不开放任何 HTTP 命令执行、环境写入、持久化存储写入、CPLD 写入、固件安装或重启接口。

## P5-1A.3 验证进度（2026-11-20）

### 仓库恢复与基线

- 重新读取远程分支 `feature/pa220-ubootkit-web`；当前 HEAD 为 `5c33f6c4496cdc08a1cab66facf2c03b3bcc6754`。
- 与 P5-1A.2 检查点比较结果为 identical，ahead=0、behind=0；检查点之后没有远程提交。未发现本共享分支上的 P4 新提交，不执行 reset/rebase/force push。
- 已重新读取 `u-boot/include/net/tcp.h`、`u-boot/net/tcp.c`、本进度文档和 `doc/pa220/FEATURE-MATRIX.md`。

### P5-1A.3.1 目标交叉编译

- 用户的实际构建主机确实存在：Debian GNU/Linux 13.7 (trixie)、x86_64；历史工具链已在该主机确认存在，路径 `/home/debian/OCTEON-SDK/tools-gcc-4.7`，前缀 `mips64-octeon-linux-gnu-`，Cavium GCC 4.7.0 / SDK Build 52，Binutils 2.23.52。此前 P2 已完成最小交叉编译/链接验证，P3 于 2026-11-19 在该主机成功完整构建并生成 `u-boot-octeon_pa220.bin`（1,358,312 bytes）。
- 上述 Debian 主机是用户自己的构建环境；历史会话证据显示助手通过给出命令、由用户在该主机终端执行并回传输出的方式协作，并无证据表明当前 ChatGPT 临时容器可直接访问该主机。当前 ChatGPT 临时容器与用户 Debian 构建主机是不同环境；在临时容器中找不到 `/home/debian/...` 不能据此断言用户主机或工具链不存在。
- 本轮尚未在用户 Debian 主机执行 TCP 对象编译或完整 U-Boot 构建。下一步应沿用此前工作方式，给出针对已存在 Debian 构建 checkout/工具链的精确检查和编译命令，由用户执行并回传输出；不得要求用户重复提供已记录的工具链位置或版本。

### P5-1A.3.2 协议测试

- 已对仓库中的实际 `tcp.c` 进行源码路径审查，覆盖报文解析/校验和、握手、ACK、接收、重传和关闭逻辑；这属于静态审查，不是运行测试。
- 尚未建立并运行宿主机测试桩；协议测试通过数为 **0（未执行）**，不是测试失败，也不是测试通过。
- 两个待优先复现的状态边界：
  1. 进入 CLOSE-WAIT 后，当前通用接收路径仍可能接受 FIN 之后到达的数据；需要测试 FIN 后数据的序列号和状态处理。
  2. TIME-WAIT 没有独立的重复 FIN 处理分支；重复 FIN 的 ACK 与 2MSL 计时器重启行为需要测试。
- 上述两项是源码审查发现的待验证风险，尚未以测试复现；暂不在缺少复现证据时修改 TCP 状态机。

### 下一步入口

- 首先恢复可访问的 PA-220 固定源码 checkout 与 Cavium GCC 4.7.0 SDK，确认真实编译器前缀、编译参数、配置头文件和 include 路径，再编译 `u-boot/net/tcp.c`。
- 并行准备宿主机测试桩，直接编译实际 `tcp.c`；逐项执行 P5-1A.3.2 测试矩阵，优先验证上述 FIN/CLOSE-WAIT/TIME-WAIT 边界。
- 任何修复必须先有失败用例，再做最小代码修改并重跑回归。目标编译、协议测试和链接均通过前，不进入 P5-1B。


## 环境记录纠正（2026-11-20）

此前本节将 ChatGPT 当前临时容器的文件系统检查错误地表述为“构建环境不存在”。该结论不成立，现予纠正：用户的 Debian 13.7 构建主机和 Cavium GCC 4.7.0 工具链已有历史实证；本轮只是当前助手容器不能直接访问用户主机。后续按既有协作方式在用户主机运行命令并依据回传结果验证。
