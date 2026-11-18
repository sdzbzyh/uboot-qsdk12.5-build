# P1-A4：存储后端审计（第三批：环境变量与 SPI 冗余环境后端）

- 状态：**本批源码静态审计完成；P1-A4 整体进行中；未编译、未实机验证**
- 日期：2026-11-18
- uBootKit：chenxin527/uboot-qsdk12.5-build@9ce315735ac88fd0a7c7697bc527a8616ed0a972
- PA-220：sdzbzyh/pa220-openwrt@801717ba6a8a59ed2cfd53bf3e1b19841fecd2e8
- 前置：P1-A4-EMMC-SPI-CORE.md、P1-A4-GPT-FILESYSTEM.md
- 范围：common/env_sf.c、env_common.c、cmd_nvedit.c、environment.h、common/Makefile、PA-220 环境配置。

## 1. PA-220 环境存储配置

固定文件：u-boot/include/configs/octeon_pa220.h。

| 配置 | 实际值 |
|---|---|
| CONFIG_ENV_IS_IN_SPI_FLASH | 已定义 |
| CONFIG_SYS_REDUNDAND_ENVIRONMENT | 已定义 |
| CONFIG_ENV_SPI_BUS | 0 |
| CONFIG_ENV_SPI_CS | 0 |
| CONFIG_ENV_SPI_MAX_HZ | 16,000,000 |
| CONFIG_ENV_SPI_MODE | 0 |
| CONFIG_ENV_SIZE | 8 KiB |
| CONFIG_ENV_SECT_SIZE | 64 KiB |
| CONFIG_ENV_OFFSET | 0xE00000 |
| CONFIG_ENV_OFFSET_REDUND | 0xE10000 |

两份环境位于两个相邻、64 KiB 对齐的 SPI NOR sector；环境数据本体为 8 KiB，剩余 sector 空间在保存时先读出再恢复。

common/Makefile:49 在 CONFIG_ENV_IS_IN_SPI_FLASH 下编入 env_sf.o；common/Makefile:222/224 编入 cmd_nvedit.o 和 env_common.o。配置不是 RAM-only 环境。

## 2. saveenv() 完整写入路径

PA-220 实际使用 common/env_sf.c，且因 CONFIG_ENV_OFFSET_REDUND 定义，编译其冗余环境 saveenv()（47-139），不是后面的非冗余分支。

### 2.1 每次 saveenv() 的执行顺序
1. 若 env_flash 尚未探测，调用 spi_flash_probe(CONFIG_ENV_SPI_BUS, CONFIG_ENV_SPI_CS, CONFIG_ENV_SPI_MAX_HZ, CONFIG_ENV_SPI_MODE)。
2. 调用 hexport_r() 将 env_htab 导出为 NUL 分隔数据。
3. 计算 CRC32，设置新副本 flags=ACTIVE_FLAG。
4. 根据 gd->env_valid 选择 inactive copy：
   - 当前 primary 有效：新写 CONFIG_ENV_OFFSET_REDUND；
   - 当前 redundant 有效：新写 CONFIG_ENV_OFFSET。
5. 因 CONFIG_ENV_SECT_SIZE(64 KiB) 大于 CONFIG_ENV_SIZE(8 KiB)，先从目标 sector 的 env data 尾部偏移读取剩余 56 KiB。
6. 调用 spi_flash_erase(env_flash, env_new_offset, 64 KiB) 擦除目标 sector。
7. 调用 spi_flash_write() 写入 8 KiB 新 env_t。
8. 再调用 spi_flash_write() 恢复目标 sector 剩余 56 KiB 原数据。
9. 调用 spi_flash_write() 将旧副本 env_t.flags 改为 OBSOLETE_FLAG。
10. 更新 gd->env_valid，完成返回。

### 2.2 真实持久化影响
- 每次 saveenv() 至少擦除一个完整 64 KiB SPI NOR sector。
- 随后写入 8 KiB 环境结构和最多 56 KiB sector 尾部恢复数据。
- 还会写旧副本 flags 字节。
- 写入使用 spi_flash_erase()/spi_flash_write()，不依赖是否执行过 sf 命令；即使 Web handler 直接调用 saveenv()，也会发生 SPI NOR 持久化写入。
- 双副本机制可以在部分写失败时保留旧副本，但不等于无写入或无风险；没有在写入后完整读回验证新副本 CRC 的步骤。
- 写入过程断电可能导致新副本无效；旧副本 flags 更新失败可能出现两个副本状态不一致。启动时 env_relocate_spec() 依据 CRC/flags 选择有效副本。

## 3. 启动时环境恢复路径

- env_init()（328-335）只将 gd->env_addr 指向 default_environment，并设置初始 env_valid；此时 SPI 尚不可用，不执行写入。
- env_relocate_spec()（141-222）分配两个 env_t 缓冲，probe SPI，分别读取 CONFIG_ENV_OFFSET 和 CONFIG_ENV_OFFSET_REDUND。
- 分别校验 CRC32：
  - 两份 CRC 均无效：set_default_env("!bad CRC")；
  - 仅 primary 有效：gd->env_valid=1；
  - 仅 redundant 有效：gd->env_valid=2；
  - 两份都有效：依据 ACTIVE_FLAG/OBSOLETE_FLAG 选择；flags 不一致/异常时走默认选择分支。
- 选中副本后调用 env_import(ep, 0) 导入运行时 env_htab；没有在此函数中执行 SPI erase/write。
- 这说明正常启动时的环境恢复是 SPI 读取；saveenv 才是持久化写入口。

## 4. 环境变量 API 和命令边界

### 4.1 RAM-only 操作
- setenv()/env_set() 修改运行时 env_htab。
- env_import() 将 buffer 导入运行时 env_htab。
- env export/import 命令本身负责在内存与文件/缓冲之间转换；不等同于 saveenv。
- 只调用 setenv()/env_import() 而不调用 saveenv()，不会通过 env_sf.c 写入 SPI 环境区。

### 4.2 持久化操作
- common/cmd_nvedit.c:702-712 的 saveenv 命令调用 saveenv()。
- octeon_cmd_conf.h 启用 CONFIG_CMD_SAVEENV。
- octeon_pa220.h 启用 CONFIG_CMD_EXPORTENV、CONFIG_CMD_IMPORTENV。
- uBootKit 的 network.c/env.c 修改 handler 会在 setenv/env_import 后调用 saveenv()；若按此逻辑移植到 PA-220，将直接擦写上述 SPI 环境 sector。
- PA-220 include/environment.h 暴露 env_import()，但本次没有发现与 uBootKit 同名的公开 env_export() 声明；PA 旧版导出命令在 cmd_nvedit.c 内通过 hexport_r() 实现。只读导出接口需要单独封装。

## 5. uBootKit 对照与接口差异

- uBootKit 固定 commit 的 common/env_sf.c 同样提供冗余 SPI env 写入逻辑，但其函数命名/封装不同（sf_saveenv/sf_env_relocate_spec），且 export 使用 env_export() API。
- PA-220 2013.07 的环境实现通过 hexport_r() 导出、env_import() 导入；不能直接把 uBootKit env.c 中 env_export() 调用复制到 PA-220。
- 两边都依赖 spi_flash_probe/erase/write，但目标板 bus/CS、环境偏移、sector size 和 SPI driver 必须按 PA-220 固定配置，不可继承 IPQ 默认值。

## 6. 对 PA-220 Web 设计的硬性约束

1. Web 第一版仅允许读取环境白名单；不能全量导出所有环境变量。
2. 只读展示不得调用 setenv()/env_import()，也不得调用 saveenv()。
3. 若未来允许 RAM-only 临时设置，必须明确只改变当前运行时状态，不自动保存；网络参数还需同步 NetInitLoop/NetOurIP。
4. 任何 saveenv() 必须作为单独的持久化动作，明确提示将擦写 SPI NOR 环境主/冗余 sector，并要求用户独立授权。
5. Web Terminal、远程命令、任意 shell、升级 handler 均不得绕过此约束。
6. 本项目的 TFTP/RAM chain-load 不会改变 CONFIG_ENV_OFFSET/REDUND，也不会使 saveenv() 变成 RAM-only。

## 7. 本批状态

- 已确认：PA-220 SPI redundant env 配置、common/Makefile 编入、saveenv() erase/write 顺序、启动 CRC/flags 选择逻辑。
- 未确认：实际设备 SPI bus0/CS0 对应物理 SPI 芯片与 CPLD flash-select 状态；未进行设备探测或读写。
- 未执行：saveenv、sf erase/write/update、任何 SPI/eMMC/EEPROM/CPLD 写操作。
- 编译成功、实机验证均 NOT VERIFIED。
