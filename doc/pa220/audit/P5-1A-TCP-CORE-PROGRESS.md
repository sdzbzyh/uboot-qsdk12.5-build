# P5-1A TCP 核心实现进度：报文基础层

- 日期：2026-11-20
- 集成仓库分支：`feature/pa220-ubootkit-web`
- 固定 PA-220 源码：`IronDemiGod/pa220-openwrt@801717ba6a8a59ed2cfd53bf3e1b19841fecd2e8`
- 本阶段性质：P5-1A 的第一个独立代码单元；**不是完整 TCP 实现**。

## 1. 目录布局核实

固定源码采用旧版 U-Boot 常见布局：网络协议对象位于 `u-boot/net/`，公共网络头位于 `u-boot/include/net.h`。当前没有 `u-boot/net/tcp.c` 或 TCP 公共头文件，因此新增独立目录头 `u-boot/include/net/tcp.h` 与实现 `u-boot/net/tcp.c`，不改动既有 `net.h`、`net.c` 或 `net/Makefile`。

## 2. 本提交实现

- TCP 固定报文头长度、最大头长度和 MSS/RX/TX/重传策略常量。
- 无对齐假设的网络序字段读取。
- TCP data offset 与头/载荷边界检查。
- TCP options TLV 边界检查；支持识别 MSS，拒绝非法长度、零 MSS 和重复 MSS；未知 option 安全跳过，EOL/NOP 正确终止/推进。
- IPv4 TCP 伪首部校验和计算与校验入口，支持奇数长度载荷。
- RFC 9293 风格 32 位序列号模运算比较。
- 明确声明 RX payload 指针仅为当前报文视图，后续连接层必须复制数据，不得跨收包周期保存。

## 3. 明确未实现

本提交**没有**实现 TCP 连接状态机、监听、SYN/SYN-ACK/ACK 交换、序列空间推进、接收窗口/缓冲区操作、发送队列、ACK 管理、FIN/RST 状态处理、重传调度、超时轮询或以太网/IP 发包。头文件中的容量和重传常量只是下一阶段策略约束，不代表相应功能已经存在。

未接入 `NetReceive()`，未修改 UDP/ARP handler、全局 timeout、网络 Makefile 或 CLI；未实现 HTTP parser/Web 服务。

## 4. 验证状态

- 目录/接口静态核对：完成。
- 代码静态审查：已进行边界与整数宽度检查，仍需编译器和测试向量验证。
- 编译/链接：未执行（当前仅通过 GitHub 仓库接口读取/提交源码，未取得可运行的 PA-220 构建环境）。
- 单元测试/校验和向量：未执行。
- RAM 启动/实机网口：未执行。

## 5. 下一步入口（P5-1A.2）

在不接入 NetReceive 的前提下实现单连接 TCP 状态机和可注入收发/时钟接口：SYN 接入与 SYN-ACK 重传、ACK/序号校验、有限接收缓冲区与乱序策略、RST/FIN、发送确认与重传上限、超时清理。实现前先针对本提交做真实工具链编译和校验和测试；若编译环境仍不可用，保持 NOT VERIFIED，不得宣称 TCP 核心完成。
