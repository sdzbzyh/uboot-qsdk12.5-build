# P1-A4：存储后端审计（第一批：eMMC 与 SPI NOR 核心）

- 状态：**本批源码静态审计完成；P1-A4 整体进行中；未编译、未实机验证**
- 日期：2026-11-18
- uBootKit：chenxin527/uboot-qsdk12.5-build@9ce315735ac88fd0a7c7697bc527a8616ed0a972
- PA-220：sdzbzyh/pa220-openwrt@801717ba6a8a59ed2cfd53bf3e1b19841fecd2e8
- 范围：MMC/SPI Flash Makefile、核心结构和命令入口；对照 PA-220 OCTEON MMC driver、SPI NOR driver、板级配置。
- 后续 P1-A4 仍需审计 GPT/分区 API、文件系统、环境后端和固件分区映射。

## 1. eMMC 构建与 API

### 1.1 uBootKit
- drivers/mmc/Makefile：CONFIG_GENERIC_MMC 编入 mmc.o/mmc_write.o；CONFIG_QCA_MMC 编入 qca_mmc.o；CONFIG_SDHCI 编入 sdhci.o。
- include/mmc.h 定义 struct mmc、mmc_config、mmc_ops、block_dev_desc_t block_dev。
- 公共接口包括 find_mmc_device()、mmc_get_dev()、mmc_init()、mmc_read()、mmc_get_life_info()、mmc_hwpart_config()、RPMB 操作和 EXT_CSD boot configuration。
- mmc_get_dev() 返回 block_dev_desc_t*；块接口通过 block_read/block_write/block_erase 访问。
- common/cmd_mmc.c 提供 mmc read/write/erase、rescan、part、dev、list、protect、hwpartition、bootbus、bootpart-resize、partconf、rst-function、RPMB 等命令。

### 1.2 PA-220
- octeon_pa220.h:37 启用 CONFIG_OCTEON_MMC、CONFIG_CMD_MMC、CONFIG_MMC；drivers/mmc/Makefile 通过 CONFIG_OCTEON_MMC 编入 octeon_mmc.o。
- PA-220 的 drivers/mmc/octeon_mmc.c（4,776 行）不仅是 host driver，还实现 mmc_initialize/find_mmc_device/mmc_init/mmc_read/mmc_write/block_read/block_write/block_erase 等 MMC 核心路径。
- 底层通过 CVMX MIO EMM/NDF DMA CSR、cvmx_ptr_to_phys() 和 OCTEON watchdog 访问控制器；这是 CN7130 专用实现，不能用 QCA MMC/SDHCI host 代码替换。
- PA-220 include/mmc.h 仍使用 struct mmc + block_dev_desc_t；find_mmc_device()/mmc_get_dev()/mmc_init()/mmc_read() 等基础 API 与 uBootKit 名称相同或近似。
- 但 PA-220 头文件未发现 uBootKit 的 mmc_get_life_info()、mmc_hwpart_config() 等所有扩展接口；逐项使用前必须核对声明/实现，不可根据同名 struct 推断完全 ABI 兼容。
- uBootKit 的 generic mmc core 与 QCA host 驱动、PA-220 的 OCTEON 单体 MMC 实现属于不同实现边界。适配应保留 PA-220 octeon_mmc.c，仅对上层通用只读块接口做兼容封装。

## 2. PA-220 eMMC 读写路径和风险

### 2.1 块读取
- octeon_mmc.c:1389-1452 的 mmc_bread() 查找设备、检查 blkcnt!=0、检查 start+blkcnt 是否超过 block_dev.lba、检查卡存在，然后调用 mmc_read()；非 8-byte 对齐缓冲使用 4 KiB bounce buffer。
- 这是只读路径；仍需注意 start+blkcnt 整数溢出和目标 RAM 缓冲范围由调用者负责。

### 2.2 块写入
- octeon_mmc.c:1454-1525 的 mmc_bwrite() 检查设备、blkcnt、start+blkcnt 与 block_dev.lba、卡存在和 write-protect；之后调用 mmc_write()，按 cfg->b_max 分批 DMA。
- mmc_write()（1112 起）通过 CVMX EMM/NDF DMA 写入 eMMC；失败时重试/停止 DMA，返回写入块数。
- mmc_bwrite() 有容量边界检查，但 start+blkcnt 未用减法式防溢出；RAM 源缓冲区大小/地址由上层命令保证，driver 本身不校验物理内存范围。
- 持久化风险：任何调用 block_write()/mmc_write() 的路径都会直接写 eMMC 数据区。

### 2.3 块擦除
- octeon_mmc.c:1321-1385 的 mmc_erase_t() 计算 end=start+blkcnt-1，发送 ERASE_GROUP_START/END 和 MMC_CMD_ERASE。
- octeon_mmc.c:1528-1550 的 mmc_berase() 直接调用 mmc_erase_t()；与 mmc_bwrite() 不同，mmc_berase() 本身没有 blkcnt==0 检查，也没有 start+blkcnt 与 mmc->block_dev.lba 的容量检查。
- blkcnt=0 会导致 end=start-1；start+blkcnt 也可能溢出。此处是必须单独修正/封装的风险点。
- mmc_berase() 对 erase group 不对齐只打印 caution，但仍按原始 start/blkcnt 发擦除命令；不应误以为会自动按组安全调整范围。

### 2.4 PA-220 eMMC 命令
- common/cmd_mmc.c:167-439：支持 rescan、part、list、dev、read、write、erase；在 CONFIG_SUPPORT_EMMC_BOOT 下还可能支持 open/close/bootpart。
- read 使用 block_read；write 使用 block_write；erase 使用 block_erase。
- open/close 调用 boot_part_access() 改变 boot partition 访问/配置；bootpart 调用 mmc_boot_partition_size_change() 改变 boot/RPMB 分区大小。它们不是普通数据读取，可能修改 eMMC EXT_CSD 配置，需独立确认可逆性和一次性限制。
- PA-220 octeon_pa220.h 设置 CONFIG_OCTEON_MMC_NO_RST_N=1，明确禁止写入 EXT_CSD RST_n_FUNCTION OTP 字段；不得移除该保护。
- 结论：eMMC 只读查询/块读取可研究复用现有 OCTEON driver；写/擦除和 EXT_CSD 配置命令均属于持久化高风险操作，P1 不执行。

## 3. SPI NOR 构建和设备探测

### 3.1 PA-220 配置
- octeon_pa220.h:109-115 启用 CONFIG_SPI_FLASH、CONFIG_SPI_FLASH_WINBOND、CONFIG_OCTEON_SPI、CONFIG_SPI；默认 bus=0、CS=0、speed=16 MHz。
- 同一配置明确注释：SPI NOR（Winbond W25Q128）“read access only is intended”。
- octeon_pa220.h:112 启用 CONFIG_CMD_SF；drivers/mtd/spi/Makefile 在 CONFIG_CMD_SF 下编入 sf.o，在 CONFIG_SPI_FLASH 下编入 sf_probe.o、sf_ops.o。
- 这表示代码中存在 SPI 写命令，不表示项目允许执行；项目 D-004/D-006 和 PROJECT 安全规则仍禁止 sf erase/write/update。
- 当前默认 bus/CS 配置不能单独证明它对应正常 SPI、备用 SPI 或实际 CPLD flash-select 状态；芯片映射仍需设备只读证据，禁止自动写 CPLD reg d。

### 3.2 PA-220 SPI probe/read/write
- drivers/mtd/spi/sf_probe.c:309-387 的 spi_flash_probe() 创建 spi_slave、claim bus、发送 CMD_READ_ID、通过 spi_flash_validate_params() 识别芯片并返回 spi_flash 对象；这是探测/读取 ID，不执行 flash erase/write。
- drivers/mtd/spi/sf_ops.c 的 spi_flash_cmd_read_ops() 实现读取；spi_flash_cmd_erase_ops() 检查 erase_size 对齐后发送擦除命令；spi_flash_cmd_write_ops() 按 page_size 分页发送 page program。
- spi_flash_write_common() 会执行 write enable、写命令和 ready polling；spi_flash_cmd_erase_ops() 调用 write_common 发送 erase opcode。
- 这些 API 可用于未来受控只读探测/备份设计；任何 write/erase 调用都属于 SPI NOR 持久化修改。

### 3.3 PA-220 sf 命令
- common/cmd_sf.c:89-142：sf probe 默认读取 CONFIG_SF_DEFAULT_BUS/CS/SPEED/MODE，调用 spi_flash_probe() 并保存全局 flash 指针。
- common/cmd_sf.c:243-296：sf read/write/update 解析 RAM 地址、flash offset、长度；write 调用 spi_flash_write()，update 调用 spi_flash_update()。
- common/cmd_sf.c:299-329：sf erase 调用 spi_flash_erase()。
- PA-220 sf read/write/update 路径未见 uBootKit 新版 cmd_sf.c 中对 RAM 地址区间的同等完整检查；offset+len 也存在整数溢出审查需求。
- sf update 会按扇区读出、比较、擦除整个扇区、写入新数据并恢复未覆盖尾部；这是读改擦写，不是只写变化字节。
- SPI NOR 是项目只读维护对象；即使命令可用，也不能在 P1/P2 的 RAM-only 验证中执行 sf write/update/erase。

### 3.4 uBootKit sf 命令差异
- uBootKit common/cmd_sf.c:269-370 使用 mtd_arg_off_size() 解析设备/分区偏移和长度，并检查 flash offset+len、RAM 地址是否在 CONFIG_SYS_SDRAM_BASE 到 gd->ram_size 之内。
- PA-220 旧版 common/cmd_sf.c 仍使用简单的 simple_strtoul()，没有同等的 RAM region 检查；两者不能直接假定安全性相同。
- 两版本均最终调用 spi_flash_read/write/erase/update；底层语义仍是 SPI NOR 命令。

## 4. 本批适配结论

| 子系统 | PA-220 源码/配置 | 可适配判断 | 风险 |
|---|---|---|---|
| eMMC 查询/只读块访问 | OCTEON MMC driver、block_dev_desc_t、mmc_get_dev/find_mmc_device | 复用 PA-220 现有驱动；适配上层查询接口 | 未验证容量/内存边界 |
| eMMC block write/erase | mmc_write/block_write/mmc_erase_t/block_erase | 不在 P1 执行；未来单独授权 | 持久化写，erase range 缺少完整边界校验 |
| eMMC boot/RPMB 配置 | 条件命令/EXT_CSD API | 只读展示可研究；写配置禁止默认开放 | 可能修改启动分区配置/OTP |
| SPI probe/JEDEC/read | OCTEON SPI + Winbond SPI_FLASH | 可作为只读探测基础，需确认实际 CS/芯片 | 设备映射未实测 |
| SPI write/erase/update | CONFIG_CMD_SF 与 sf_ops 写接口存在 | 不移植为可执行 Web 功能 | 持久化破坏 SPI 启动链/环境区 |

## 5. 本批未完成范围

- 未执行任何 mmc write/erase、mmc bootpart/open/close、sf write/erase/update、saveenv。
- 未执行 SPI probe、eMMC rescan 或设备读写实测。
- 尚未完成 GPT/分区表、ext4/FAT/UBIFS 文件系统和环境后端完整审计。
- 构建成功、RAM 启动、eMMC/SPI 实机功能均 NOT VERIFIED。
