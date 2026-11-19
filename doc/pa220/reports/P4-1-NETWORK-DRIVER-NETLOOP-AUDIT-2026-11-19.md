# PA-220 P4-1 第二批网络配置、MGT 注册与 NetLoop 审计（2026-11-19）

- 审计使用的集成分支 HEAD：`66c136daa97e47612d19c10f03042cadc39e7e1b`
- PA-220 固定源码：`IronDemiGod/pa220-openwrt@801717ba6a8a59ed2cfd53bf3e1b19841fecd2e8`
- 范围：最终配置可见性、DHCP/Ping 命令编入条件、MGT 设备注册、DHCP/Ping NetLoop 生命周期。
- 结论：**固定源码静态审计完成；最终构建 .config / include/autoconf.mk 快照未在 GitHub 源码仓库提供，当前标记 EVIDENCE MISSING；未执行构建或网口实机测试。**

## 1. 配置项与命令编入

### 1.1 板级配置链

- `u-boot/include/configs/octeon_pa220.h` 定义 `CONFIG_OCTEON_SGMII_ENET`、`CONFIG_OCTEON_QSGMII_ENET`、`CONFIG_OCTEON_MGMT_ENET`，随后 include `octeon_cmd_conf.h`。
- `octeon_cmd_conf.h` 在未定义 `OCTEON_NO_NETWORK` 和 `CONFIG_OCTEON_DISABLE_NETWORKING` 时定义 `CONFIG_CMD_NET`、`CONFIG_CMD_PING`、`CONFIG_CMD_DHCP` 等网络命令。
- `octeon_pa220.h` 通过 `CONFIG_EXTRA_ENV_SETTINGS` 提供 `autoload=n`、`ethact=octrgmii0`、`ethprime=octrgmii0` 默认值。已存在的持久化环境可以覆盖这些默认值。

### 1.2 编译对象与命令入口

- `u-boot/net/Makefile`：`CONFIG_CMD_NET` 编入 `bootp.o`、`eth.o`、`net.o`、`tftp.o`；`CONFIG_CMD_PING` 编入 `ping.o`。
- `u-boot/common/cmd_net.c)：`CONFIG_CMD_DHCP` 条件编入 `dhcp` 命令；`CONFIG_CMD_PING` 条件编入 `ping` 命令。
- DHCP 命令实际调用 `netboot_common(DHCP,...)`；Ping 命令调用 `NetLoop(PING)`。
- 这些配置由板级头文件静态定义链支撑，但本仓库当前没有最终构建目录中的 `.config` 或 `include/autoconf.mk` 快照。因此不能将“源代码定义存在”升级为“最终构建配置已实证”。状态：**源码条件成立；最终配置快照 EVIDENCE MISSING**。

构建入口 `build-pa220.sh` 调用固定源码的 `u-boot/build.sh`；后者执行 `make octeon_pa220_config` 和 make。后续若复用现有构建目录，应只读取已经生成的 `include/config.h`、`include/autoconf.mk` 或构建日志，不为获取配置证据而重复构建。

## 2. MGT 设备注册路径

### 2.1 CPU 网络初始化

`u-boot/arch/mips/cpu/octeon/cpu.c:cpu_eth_init()` 在 `CONFIG_CMD_NET` 下编译：
1. 若 RAM 环境存在 `disable_networking`，直接跳过网络初始化。
2. 在网络/MII/MDIO 条件满足时初始化 OCTEON MDIO。
3. 若定义 `CONFIG_OCTEON_MGMT_ENET` 且芯片具有 `OCTEON_FEATURE_MGMT_PORT` 或 `OCTEON_FEATURE_BGX_MIX`，调用 `octeon_mgmt_eth_initialize()`。
4. 若定义 RGMII/XAUI/SGMII 相关宏，调用 `octeon_eth_initialize()`；PA-220 定义了 `CONFIG_OCTEON_SGMII_ENET`，因此该调用点具备编译条件。

### 2.2 octrgmii0 的真实来源

- `octeon_mgmt_eth_initialize()` 注册的专用 management port 名称是 `octmgmt%d`，不是 `octrgmii0`。
- `octeon_eth_initialize()` 扫描 OCTEON interfaces，包含 `CVMX_HELPER_INTERFACE_MODE_AGL`。
- `octeon_eth_initialize_port()` 对 AGL 模式使用 AGL GMX base，并命名为 `octrgmii%d`；随后设置 init/halt/send/recv/write_hwaddr 回调并调用 `eth_register()`。
- PA-220 板级注释标注 MGT 为 AGL (`octrgmii0`)；板级环境默认 `ethact/ethprime=octrgmii0`。因此这两个默认名称对应的是 AGL 内置以太网扫描路径，不应误认成 `octmgmt0` 管理端口。
- 静态源码只证明注册条件和命名规则；最终设备列表、实际 ethact 选择及 PHY/link 状态仍需串口实测确认。不得把 ethact 名称当作 carrier/link 证明。

## 3. NetLoop 与 DHCP-only 生命周期

- `NetLoop()` 先执行 `net_init()`；对 DHCP/Ping 等非 NETCONS 协议，执行 `eth_halt()`、`eth_set_current()`、`eth_init(bd)`，再检查网络前置条件。
- DHCP 分支清 `NetOurIP` 并调用 `DhcpRequest()`；`DhcpRequest()` 进入 `BootpRequest()`，设置 DHCP 状态机、UDP handler 和超时后发送 DHCP Discover。
- `DhcpHandler()` 收到 ACK 后解析选项、调用 `BootpCopyNetParams()`，将网络参数放入 Net* 运行时变量，然后调用 `net_auto_load()`。
- `net_auto_load()` 先处理显式 autoload=NFS；否则当 `getenv_yesno("autoload") == 0` 时设置 `NETLOOP_SUCCESS` 并返回，不启动 TFTP。若 autoload 未设置或不是 n，则可能进入 TFTP GET。因此测试前必须读取并确认值恰为 n。
- DHCP 成功后 `NetLoop` 清理 UDP/ARP/timeout handlers、停止网卡、返回传输大小 0。随后 `netboot_common()` 调用 `netboot_update_env()`，通过 `setenv()` 更新 RAM 环境中的 ipaddr/serverip/gatewayip/netmask/hostname/rootpath/dnsip 等字段；size 为 0 时直接返回，不 flush 下载缓存、不 boot、不执行 saveenv。
- `bootfile` 环境 callback 会同步更新 RAM 全局 `BootFile`。服务端未提供的字段不会被强制清空；测试报告必须记录实际字段差异，而非假设所有字段均被覆盖。

## 4. Ping 生命周期

- `do_ping()` 将输入解析为 `NetPingIP` 并调用 `NetLoop(PING)`。
- PING 分支调用 `ping_start()`，设置 10000 ms timeout，先 ARP 再发送 ICMP Echo Request。
- 匹配目标 IP 的 Echo Reply 后设置 `NETLOOP_SUCCESS`；成功路径清 handlers 并执行 `eth_halt()`。
- `ping_timeout()` 自身也调用 `eth_halt()`，然后设置 `NETLOOP_FAIL`；失败路径清 handlers 后返回失败。
- 因而 Ping 成功和超时都会停止接口；下一次网络命令会重新走初始化路径。是否恢复必须实机验证，静态代码不能证明物理链路恢复。

## 5. 当前状态与下一步

| 项目 | 状态 |
|---|---|
| DHCP/Ping 源码配置条件、命令入口 | STATIC AUDIT COMPLETE |
| MGT/AGL 注册命名链 | STATIC AUDIT COMPLETE |
| DHCP-only autoload 保护及 RAM 环境更新 | STATIC AUDIT COMPLETE |
| Ping timeout / eth_halt 路径 | STATIC AUDIT COMPLETE |
| 最终构建 `.config` / `include/autoconf.mk` | EVIDENCE MISSING |
| 运行时 ethact/autoload/device list | NOT RUN |
| DHCP 实机获取地址 | NOT RUN |
| Ping 成功/超时/接口恢复 | NOT RUN |

下一步：
1. 从现有 PA-220 构建目录读取 `include/autoconf.mk` 或 `include/config.h`，不重复构建；若构建目录不存在，记录 EVIDENCE MISSING。
2. 串口只读确认 `printenv autoload ethact ethprime disable_networking`、`printenv ipaddr serverip gatewayip netmask dnsip bootfile`、`printenv` 中相关网络字段，以及 `eth list` / `eth current`（若该版本命令支持）。
3. 仅在确认 `autoload=n` 且设备为预期 MGT AGL 设备后，执行 DHCP；记录前后环境差异。禁止 `tftpboot`、`tftpsrv`、`tftpput`。
4. DHCP-only 完成后再测试 Ping 成功、超时和后续接口重新初始化。
5. TFTP 接收仍受统一 RAM validator 门控，不能因本轮源码审计完成而开放。

本批没有构建、没有执行网络命令、没有写入持久化环境或任何存储。
