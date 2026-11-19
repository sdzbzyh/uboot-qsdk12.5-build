# PA-220 P4 计划：网络维护基础

- 状态：**IN PROGRESS**
- 启动日期：2026-11-19
- 集成分支：`feature/pa220-ubootkit-web`
- PA-220 固定源码：`IronDemiGod/pa220-openwrt@801717ba6a8a59ed2cfd53bf3e1b19841fecd2e8`
- uBootKit 固定审计基线：`chenxin527/uboot-qsdk12.5-build@9ce315735ac88fd0a7c7697bc527a8616ed0a972`
- 前置阶段：P3 CLI/只读诊断 **COMPLETE**

## 1. 目标

在 PA-220 MGT 网口建立可复现的基础网络维护能力，优先复用 PA-220 自带的 DHCP Client 和 Ping；确认 TFTP 既有实现、配置与接收边界，避免将 IPQ 平台代码或未经验证的任意 RAM 写入路径带入 PA-220。

## 2. 首批源码审计结论

| 功能 | PA-220 固定源码状态 | 决策 |
|---|---|---|
| DHCP Client | `CONFIG_CMD_DHCP`、`DhcpRequest/DhcpHandler`、`dhcp` 命令已存在 | 复用旧实现；验证配置、autoload 与运行时环境副作用 |
| Ping | `CONFIG_CMD_PING`、`ping.o`、`ping` 命令已存在 | 复用旧实现；验证 NetLoop、ARP/ICMP、超时退出行为 |
| TFTP GET | 已存在；接收 block 经 `store_block()` 写入 RAM | 保留代码但禁止接收实测，直到 validator 接入 |
| TFTP Server | 已存在 WRQ 接收服务；DATA 经 `store_block()` 写入 RAM | 禁止启动/开放，直到 validator 接入 |
| TFTP PUT | 已存在；读取指定 RAM 源地址并向远端发送 | 暂不测试；后续另审源地址范围/信息泄漏风险 |
| uBootKit TFTP 新实现 | 含 IPQ806X/SDRAM/FDT/TrustZone 边界假设 | 不直接移植；只借鉴协议/健壮性设计 |

## 3. 工作批次

### P4-1：现有 DHCP Client / Ping 收敛
- 核实最终 PA-220 `.config` 对 `CONFIG_CMD_NET`、`CONFIG_CMD_DHCP`、`CONFIG_CMD_PING` 的实际启用状态。
- 核实 MGT 设备 `octrgmii0` 的驱动注册、ethact/ethprime 选择与 NetLoop 初始化/退出路径。
- DHCP 仅做客户端地址获取；测试前确认 `autoload=n`，避免 DHCP ACK 后自动启动 TFTP 下载。
- 记录 DHCP 对 RAM 环境的更新（ipaddr/serverip/gatewayip/netmask/dnsip/bootfile）；不执行 `saveenv`。
- Ping 使用已知可达的本地网关/维护主机；记录成功、失败和超时行为。Ping timeout 当前会调用 `eth_halt()`，需要观察接口是否正常恢复。
- 构建状态、网口实机状态分开记录；不得将配置存在写成实机通过。

### P4-2：TFTP 发送/接收路径审计与安全门
- 审计 TFTP GET、TFTP Server 的每个 DATA block 写入点及终止 block/重传/block-wrap 路径。
- 先设计并实现统一 `octeon_load_range_check(uint64_t addr, uint64_t len)`，再考虑接收启用。
- validator 必须先做 checked-add/checked-multiply，拒绝零长度/溢出/回绕/越界；统一地址域并规范化 cached/uncached/physical alias。
- `__tmp_load` named block descriptor 是权威边界；必须证明 descriptor 的物理范围、可写 alias 和 U-Boot/stack/heap/exception/FDT 等保留区不重叠。
- 每个 TFTP block 在 memcpy 前验证完整目标区间；不能只在开始时检查 base 或只在最后检查 newsize。
- `CONFIG_SYS_DIRECT_FLASH_TFTP` 必须在最终配置中确认关闭；任何 flash_write 路径不属于本阶段允许范围。
- 同一 validator 设计需为后续 HTTP payload reservation/TCP chunk、ELF PT_LOAD 提供一致接口。

### P4-3：TFTP 安全接收验证（仅在 P4-2 完成后）
- 仅使用明确批准的 RAM 测试区域和短小测试文件。
- 覆盖 block 边界、最后短 block、超长文件、block wrap、超时/重传和目标区间拒绝。
- 只允许 RAM 写入；禁止 saveenv、SPI/eMMC/NAND/CPLD/EEPROM 写入。

## 4. 运行时验证安全前提

- DHCP 运行前确认 `autoload=n`；若不是，先在 RAM 环境设为 n，绝不保存环境。
- DHCP/Ping 会初始化/停止网卡并改变网络运行时状态；在串口维护窗口执行，不与其他网络服务并行。
- 先只测 DHCP Client 和 Ping；不要运行 `tftpboot`、`tftpsrv`、`tftpput`。
- 不把 `0x81000000` 当作默认安全上传缓冲区；它是原厂 Stage3 镜像加载地址。
- 未证明 RAM validator 前，所有 TFTP/HTTP/ELF 接收均保持 BLOCKED。

## 5. P4 完成标准

1. DHCP Client 与 Ping 的 PA-220 编译配置、命令入口、网络主循环及退出行为有源码证据。
2. DHCP 在 `autoload=n` 下完成客户端地址获取，不触发 TFTP；RAM 环境变化可解释且未保存。
3. Ping 对可达/不可达目标的成功/超时路径有串口证据，接口状态可恢复。
4. TFTP GET/Server 接收路径完成逐 block 边界审计；validator 先于接收功能启用。
5. TFTP PUT 的 RAM 源地址边界与数据外传风险有独立结论。
6. 每批更新测试计划、源码审计报告和串口日志；提交后从远端回读核验。

## 6. 当前未完成

- 本计划创建时仅完成 P4-1/P4-2 的首轮源码审计；未构建新代码、未执行 DHCP/Ping 网口实测。
- TFTP validator 未实现；任何 TFTP 接收/上传测试均禁止。
