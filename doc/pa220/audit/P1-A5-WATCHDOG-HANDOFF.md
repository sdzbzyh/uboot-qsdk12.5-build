# P1-A5：OCTEON 硬件 Watchdog 与启动交接审计

- 状态：**本批源码静态审计完成；P1-A5 整体进行中；未实机验证**
- 日期：2026-11-18
- PA-220 固定源码：sdzbzyh/pa220-openwrt@801717ba6a8a59ed2cfd53bf3e1b19841fecd2e8
- 范围：octeon_wd.c、cmd_octeon_wd.c、octeon_env.c、octeon_boot.c、octeon_pa220.h、octeon_common.h。

## 1. 构建状态

- octeon_pa220.h:199 定义 CONFIG_HW_WATCHDOG。
- arch/mips/cpu/octeon/Makefile:71-72 在 CONFIG_HW_WATCHDOG 下编入 octeon_wd.o 和 commands/cmd_octeon_wd.o。
- octeon_common.h:481-482 定义 CONFIG_OCTEON_WD_TIMEOUT=5 seconds。
- octeon_common.h:236-237 将 watchdog_enable/watchdog_timeout 加入 CONFIG_ENV_CALLBACK_LIST_STATIC。
- 因此 PA-220 具备 OCTEON CIU hardware watchdog 和 octwd 命令源码入口。

## 2. 启动/停止机制

### 2.1 环境变量 callback
- arch/mips/cpu/octeon/octeon_env.c:235-260 定义 on_watchdog_enable()，并将其注册为 watchdog_enable 和 watchdog_timeout 两个 env callback。
- env_op_create/env_op_overwrite：读取 watchdog_timeout（默认 CONFIG_OCTEON_WD_TIMEOUT*1000，即 5000 ms），调用 hw_watchdog_start(msecs)。
- env_op_delete：调用 hw_watchdog_disable()。
- callback 不检查 value 内容；创建/覆盖 watchdog_enable 或 watchdog_timeout 即启动 watchdog，即使字符串值是 "0" 也会触发 start。
- 删除变量才触发 disable。watchdog_timeout 本身也注册同一个 callback，因此设置/修改 timeout 会立即启动 watchdog。
- 这不是单纯的环境文本修改；Web 环境编辑器若允许修改这两个变量，会立即改变硬件 watchdog 状态。

### 2.2 硬件实现
- arch/mips/cpu/octeon/octeon_wd.c:44-82 octeon_watchdog_get_period() 根据 OCTEON IO clock 计算 CIU watchdog timeout，硬件限制最大计数。
- hw_watchdog_start()（约 90-135）：
  - 初始化 boot vector 中的 NMI exception handler；
  - 若 msecs=0，读取 watchdog_timeout，默认 5000 ms；
  - 写 CVMX_CIU_PP_POKEX(0)；
  - 设置 CVMX_CIU_WDOGX(0).len 和 mode=3（Interrupt + NMI + soft-reset）；
  - 再 poke watchdog。
- hw_watchdog_disable()（约 140-155）清零 CVMX_CIU_WDOGX(0)，停止 core0 watchdog。
- octeon_watchdog_disable_all()（约 160-190）遍历可用 coremask，关闭所有 node/core watchdog。
- hw_watchdog_reset()（约 195-205）通过 CVMX_CIU_PP_POKEX(0) 喂狗。
- octwd 命令（arch/mips/cpu/octeon/commands/cmd_octeon_wd.c）：
  - octwd start [timeout_ms]：启动 watchdog；
  - octwd stop：关闭 watchdog；
  - octwd test [timeout_ms]：用 cvmx_wait_usec() 故意不喂狗并等待 NMI；可能触发异常/soft reset。
- octwd test 是破坏性诊断操作，不得开放 Web/远程 RPC。

## 3. OS handoff
- arch/mips/cpu/octeon/octeon_boot.c:2274-2290 的 octeon_bootloader_shutdown()：
  1. 调用 board_prepare_os_handoff()；
  2. 在 CONFIG_HW_WATCHDOG 下调用 hw_watchdog_disable()；
  3. 根据 api_enable 决定是否关闭网络/管理口；
  4. 关闭 USB。
- 当前函数调用的是 hw_watchdog_disable()（core0），不是 octeon_watchdog_disable_all()；多核 watchdog 状态需要结合实际启动 core 状态确认。
- board_prepare_os_handoff() 的 PA-220 实现会设置 CPLD reg5 bit2（88E1680 PHY reset）和 reg7 bit2（ALM LED off）。

## 4. 与 CPLD boot watchdog 的区别

- OCTEON CIU watchdog：由 CVMX_CIU_WDOGX/PP_POKEX 控制，超时产生 interrupt/NMI/soft reset。
- PA-220 CPLD reg d bit0：板级 boot watchdog/flash failover 控制，normal bootloader 通过 pulse 告知 CPLD 当前启动链存活；超时可能切换到 failsafe SPI flash。
- 两者是不同硬件、不同寄存器、不同恢复路径，不能用 octwd stop 替代 CPLD keepalive，也不能把 CPLD reg d 当作 OCTEON CIU watchdog。
- CPLD reg d 的现有 board callback 可达性仍按 P1-A5-QLM-MDIO-PHY-BOOTBUS.md 标记 NOT VERIFIED。

## 5. PA-220 Web/维护适配约束

1. Web 初版不开放 octwd start/stop/test。
2. 通用环境变量编辑器必须将 watchdog_enable/watchdog_timeout 列为受保护变量；不要允许普通 setenv/import 触发 watchdog callback。
3. 只读页面可显示 watchdog 配置变量，但“变量存在”不等于硬件 watchdog 当前已启动，必须有只读 CSR/状态证据才可显示运行状态。
4. OS handoff 的现有 hw_watchdog_disable() 保持原样；不得擅自改为 disable_all 或删除。
5. CPLD flash failover watchdog 与 CIU watchdog 分别审计，不能合并抽象。
6. 本批未执行 octwd 命令、未修改 watchdog env、未写 CVMX CSR/CPLD 寄存器。

## 6. 本批状态

- 源码存在：已确认。
- 配置启用：CONFIG_HW_WATCHDOG 已在 PA-220 固定头文件定义；octwd 命令与驱动在 Makefile 条件下编入。
- 编译成功：NOT VERIFIED。
- 实机运行状态：NOT VERIFIED。
