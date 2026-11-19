# PA-220 P4 网络实机验证续接检查点（2026-11-19）

## 当前 GitHub 状态

- 正式集成仓库：`sdzbzyh/uboot-qsdk12.5-build`
- 本次工作分支：`feature/pa220-ubootkit-web`
- 本次开始时该分支 HEAD：`473e721d56dbe918a3130af3bc2133253ba00771`
- 默认分支 `main` HEAD：`f9784439e935a32953738b58444c5954ee1888e9`；P4 工作不在 main 上，本次不修改 main。
- 固定 PA-220 源码：`IronDemiGod/pa220-openwrt@801717ba6a8a59ed2cfd53bf3e1b19841fecd2e8`
- 固定 uBootKit 审计基线：`chenxin527/uboot-qsdk12.5-build@9ce315735ac88fd0a7c7697bc527a8616ed0a972`

当前分支已有 P4-PLAN、P4-1 两批网络源码审计、网络基线审计和测试计划。旧文档中 P4 PAUSED 是此前会话的历史控制状态；本检查点按用户 2026-11-19 最新明确指令恢复 P4，不重新开展 P3 历史审计。

## 已有证据（不重复索取）

### 用户已提供的有效配置头

```c
#define CONFIG_OCTEON_PA220       1
#define CONFIG_OCTEON_PCI_HOST    0
#define CONFIG_OCTEON_CN70XX      1
#define CONFIG_BOARDDIR board/octeon/pa220
```

该证据确认有效配置头选择 PA-220/CN70XX 板级配置；它本身不能单独证明最终编译时 `CONFIG_CMD_NET`、`CONFIG_CMD_DHCP`、`CONFIG_CMD_PING` 或 `CONFIG_SYS_DIRECT_FLASH_TFTP` 的最终值。

### 用户已提供的运行时环境

```ini
autoload=n
ethact=octrgmii0
ethprime=octrgmii0
```

以下变量在此前串口检查中未定义：`disable_networking`、`ipaddr`、`serverip`、`gatewayip`、`netmask`、`dnsip`、`bootfile`。不得再次要求用户重复提供这些值。

## 已完成的静态审计

- 固定源码已有 DHCP Client、Ping 命令及 NetLoop 路径；无需移植另一代 uBootKit DHCP/Ping 实现。
- 板级配置链静态定义网络相关 CONFIG 宏；但源码宏定义不等同于最终构建产物的有效编译配置。
- PA-220 MGT AGL 设备的静态注册路径使用 `octrgmii%d` 命名；专用 management-port 路径命名为 `octmgmt%d`，二者不可混淆。
- `ethact` / `ethprime` 当前值与 `octrgmii0` 相符，但变量值本身不能独立证明驱动已成功注册，也不能证明 PHY carrier/link-up。
- DHCP ACK 在 `autoload=n` 时不进入 TFTP 自动下载；DHCP 会通过 RAM 环境 `setenv` 更新协商参数，不调用 `saveenv`。
- Ping 成功和 10 秒超时路径都会 halt Ethernet；超时后是否能重新初始化必须实机验证。
- TFTP GET/Server 接收路径尚无统一 RAM 范围 validator；TFTP 接收、Server、PUT 实机操作继续 BLOCKED。

## 当前 P4 工作分解（以用户本次指令为准）

| 工作项 | 当前状态 | 说明 |
|---|---|---|
| P4-1 最终构建配置证据 | PARTIAL | 已有有效 PA-220 配置头和固定源码 CONFIG 链；最终网络 CONFIG 编译值尚未由构建产物/日志闭环 |
| P4-2 Ethernet 实际注册证据 | PARTIAL | 驱动注册和命名链已静态审计；运行时设备枚举/注册日志尚未取得；ethact 值已由用户提供 |
| P4-3 DHCP-only | NOT RUN | 先闭环 P4-1/P4-2；保留 autoload=n；不执行任何 TFTP 命令 |
| P4-4 Ping | NOT RUN | DHCP 后再执行成功、超时及恢复测试 |
| P4-5 最终归档 | NOT RUN | 实机证据完成后更新阶段报告和日志 |

## 下一步最小证据要求

1. 只读提取已知本地构建日志 `/home/debian/PA220_P3_build_20261119.log` 中网络对象编译/链接记录，以及最终 CONFIG 值（若日志确有记录）；不重新构建、不搜索源码树。
2. 取得当前镜像启动时 Ethernet enumeration / `[PRIME]` 输出，或通过该 U-Boot 版本实际支持的只读 eth 命令列出设备。不得假定支持现代 `eth list`。
3. 若现有构建日志没有最终 CONFIG 快照，则如实保留 EVIDENCE MISSING，不为此重新构建。

## 安全边界

- 不执行 `saveenv`。
- 不写 Flash、EEPROM、SPI、eMMC、NAND、CPLD 或其他持久化介质。
- 不执行 `tftpboot`、`tftpsrv`、`tftpput`；TFTP 接收/上传保持 BLOCKED。
- DHCP 前再次确认 `autoload=n`；仅允许 DHCP Client，不允许自动下载。
- 本检查点没有执行构建、网络命令或实机操作，也没有声称任何 P4 网络实测通过。
