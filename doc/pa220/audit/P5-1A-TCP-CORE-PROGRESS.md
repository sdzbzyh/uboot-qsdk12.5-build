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
