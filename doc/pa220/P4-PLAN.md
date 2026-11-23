# PA-220 P4 计划：网络维护基础

- 状态：**IN PROGRESS — DHCP/Ping 实机验证未完成；TFTP 接收安全门未完成（2026-11-23）**
- 启动日期：2026-11-19
- 集成分支：`feature/pa220-ubootkit-web`
- PA-220 固定源码：`IronDemiGod/pa220-openwrt@801717ba6a8a59ed2cfd53bf3e1b19841fecd2e8`
- uBootKit 固定审计基线：`chenxin527/uboot-qsdk12.5-build@9ce315735ac88fd0a7c7697bc527a8616ed0a972`
- 前置阶段：P1/P2/P3 按用户本次明确指令视为已完成；本次不重做 P3 历史证据审计，直接推进 P4。

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
- 注意 `eth_initialize()` 会按 `ethprime` 选择设备并同步 `ethact`；若指定名称未注册，`ethact` 可能被改写为实际当前设备。旧版命令是否支持 `eth list` 不预设，优先读取启动枚举日志及 `printenv`。
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


## 7. P4-1 第二批源码审计（2026-11-19）

第二批静态审计已完成，详细证据见 `reports/P4-1-NETWORK-DRIVER-NETLOOP-AUDIT-2026-11-19.md`。

已确认：
- 板级头文件启用 `CONFIG_OCTEON_SGMII_ENET`、`CONFIG_OCTEON_QSGMII_ENET`、`CONFIG_OCTEON_MGMT_ENET`；`octeon_cmd_conf.h` 在未定义网络禁用宏时启用 `CONFIG_CMD_NET`、`CONFIG_CMD_DHCP`、`CONFIG_CMD_PING`。
- `cpu_eth_init()` 的 AGL/SGMII 内置接口注册链和 `octrgmii0` 命名条件已定位。专用 management-port 初始化路径创建的是 `octmgmt0`，不可与 AGL 的 `octrgmii0` 混淆。
- DHCP ACK 在 `autoload=n` 时只完成客户端配置；NetLoop 结束时停止网卡，命令层将协商参数写入 RAM 环境，不调用 `saveenv`。
- Ping 成功路径和 10 秒超时路径都会停止网卡；超时显式调用 `eth_halt()`。后续实测必须验证下一次命令能否重新初始化接口。

尚缺证据：
- GitHub 固定源码中没有最终构建目录的 `.config` / `include/autoconf.mk` 快照，故最终编译配置状态标记 **EVIDENCE MISSING**，不能仅凭头文件宏声称最终构建配置已实证。
- 运行时 `autoload`、`ethact`、`ethprime`、`disable_networking` 和实际设备列表尚未串口只读确认。
- DHCP/Ping 尚未在网口实机验证。

下一步从已有构建目录只读提取配置快照（不重新构建），随后串口读取网络环境和设备列表；只有明确确认 `autoload=n` 后才进行 DHCP-only 测试。TFTP GET/Server/PUT 仍不执行。


## 8. 当前执行暂停（2026-11-19 最新用户指令）

此前 10:19–10:22Z 的提交重新将 P4 标记 IN PROGRESS，并记录两批 P4-1 固定源码静态审计；这些文件保留为历史文档，但不构成 P4 实际功能移植/构建/网口实测。

当前任务最新明确要求：P3 全历史会话审计、证据补录、仓库对照和最终归档完成之前，不得宣布 P3 COMPLETE，也不得开展 P4 实际开发。故本计划当前状态重新设为 **PAUSED**。P4-1 既有静态审计结果不删除、不扩展；不执行读取本地构建目录、DHCP/Ping 或任何 P4 后续步骤，除非未来用户明确重新授权。


## 9. P4 执行恢复与当前工作分解（2026-11-19）

本节按用户 2026-11-19 最新明确指令更新，优先于本文件第 8 节此前的 PAUSED 状态。用户已明确决定不重做 P3 历史审计，直接按 P4 既有进度继续；因此当前 P4 状态恢复为 **IN PROGRESS**。这不修改 P3 历史记录，也不代表 P4 实测已通过。

当前执行顺序：
1. **P4-1 最终构建配置证据**：复用已有构建目录/日志，确认最终网络 CONFIG 选项及 Ethernet、DHCP、Ping、TFTP 编译条件；不重新构建。
2. **P4-2 Ethernet 实际注册证据**：区分驱动源码存在、对象/命令实际编入、运行时设备注册成功；确认设备名称、ethact/ethprime 选择、多个设备及初始化错误/降级。
3. **P4-3 DHCP-only 实机验证**：仅在 P4-1/P4-2 前置条件满足且再次确认 autoload=n 后执行；记录 RAM 环境前后差异，不执行 TFTP。
4. **P4-4 Ping 实机验证**：依次覆盖成功、超时及超时后接口恢复；保留完整串口输出。
5. **P4-5 最终归档**：更新阶段报告、实机日志、证据等级、未解决问题和下一阶段入口。

已由用户提供、不得重复索取的证据：
- 有效配置头：CONFIG_OCTEON_PA220=1、CONFIG_OCTEON_PCI_HOST=0、CONFIG_OCTEON_CN70XX=1、CONFIG_BOARDDIR=board/octeon/pa220。
- RAM 环境：autoload=n、ethact=octrgmii0、ethprime=octrgmii0。
- 未定义变量：disable_networking、ipaddr、serverip、gatewayip、netmask、dnsip、bootfile。

当前仍缺：
- 最终构建产物/日志中 CONFIG_CMD_NET、CONFIG_CMD_DHCP、CONFIG_CMD_PING、CONFIG_SYS_DIRECT_FLASH_TFTP 等实际编译值的证据；板级头文件宏不能替代最终配置证据。
- Ethernet 启动枚举/注册日志或该版本确实支持的只读设备列表。ethact 的环境值不能单独证明设备注册成功，也不能证明物理 link-up。

当前工作检查点：`reports/P4-CONTINUATION-2026-11-19.md`。

安全边界继续有效：禁止 saveenv 和任何持久化写入；禁止 tftpboot/tftpsrv/tftpput；TFTP 接收/上传保持 BLOCKED。除非发现阻断性问题并获得用户明确同意，不重新构建镜像。

注：本计划原第 3 节的 TFTP 安全审计/validator 内容仍是独立安全门，不因当前 P4-1 至 P4-5 编号调整而解除；未完成 validator 前不得开放任何 TFTP 接收路径。


## 10. P4/P6 共用 RAM 写入前 validator 当前状态（2026-11-23）

本节覆盖此前 P4 工作分解中的历史下一步描述；不代表 P4 已完成。

### 已知状态

- P5-2C 已完成 PA-220 RAM 启动后的实机 HTTP/TCP 验收，但这只证明固定静态地址下的 Web 服务，不证明 DHCP Client/Ping 功能通过。
- DHCP Client、Ping 成功/超时/超时后接口恢复：仍未完成本阶段实机验收。
- TFTP GET/Server 的接收路径仍然 BLOCKED；不得执行接收测试。
- 当前补丁序列中尚无统一 `octeon_load_range_check()` 实现；当前代码树/补丁清单未发现 P4-2/P6 共用 RAM 写入前 validator。

### 下一工作项

在重新开放任何 TFTP/HTTP 文件接收或 ELF segment 写入之前，先实现并验证统一 validator：

1. 唯一权威边界来自运行时 `cvmx_bootmem_find_named_block("__tmp_load")` descriptor；不得仅依赖 `loadaddr`、历史 globals 或固定地址。
2. 处理 descriptor 缺失/变化、cached/uncached/physical alias、checked add、零长度、整数溢出、回绕和半开区间边界。
3. 证明允许写入范围不覆盖 U-Boot image、relocation、stack、GD、malloc、exception/bootmap/FDT 等保留区。
4. 在 Host Harness 中覆盖允许与拒绝路径，再使用 Cavium GCC 4.7.0 / OCTEON SDK 5.1.0 完成目标对象编译、完整链接和 map/symbol 检查。
5. 设备重新上电后只进行必要的只读 descriptor 核对；不得为了本工作项刷写镜像或执行任何持久化写入。

Validator 通过之前，TFTP GET/Server 接收、HTTP upload/chunk 和 bootelf PT_LOAD/section 写入均保持 BLOCKED。Validator 通过后才可进入 P6 RAM-only 文件接收及 CRC/SHA/格式识别。

### P4 其他遗留项

DHCP-only 测试必须在确认 `autoload=n` 后进行；Ping 成功、超时和超时后 Ethernet 接口恢复仍需单独验证。不得因 P5-2C HTTP 通过而关闭这些遗留项。P4 的实际完成状态必须分别记录，不得以 validator 编译通过替代 DHCP/Ping 实机验证。
