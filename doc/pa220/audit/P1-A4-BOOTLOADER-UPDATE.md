# P1-A4：固件/Bootloader 管理审计（PA-220 OCTEON bootloaderupdate）

- 状态：**本批源码静态审计完成；P1-A4 存储/固件源码审计接近完成；未编译、未实机验证**
- 日期：2026-11-18
- PA-220 固定源码：sdzbzyh/pa220-openwrt@801717ba6a8a59ed2cfd53bf3e1b19841fecd2e8
- uBootKit 固定源码：chenxin527/uboot-qsdk12.5-build@9ce315735ac88fd0a7c7697bc527a8616ed0a972
- 范围：PA-220 arch/mips/cpu/octeon/commands/cmd_octeon_bootloader.c、对应 Makefile、PA-220 config 链；与 uBootKit QCA failsafe firmware writer 对照。

## 1. 构建和配置

- arch/mips/cpu/octeon/Makefile:52-53：CONFIG_CMD_OCTEON_BOOTLOADER_UPDATE 编入 commands/cmd_octeon_bootloader.o。
- octeon_cmd_conf.h:33 定义 CONFIG_CMD_OCTEON_BOOTLOADER_UPDATE。
- octeon_pa220.h:24 定义 CONFIG_SYS_NO_FLASH；35 定义 CONFIG_OCTEON_MMC。
- octeon_common.h:71-76 定义默认 CONFIG_OCTEON_SPI_STAGE2_START=0x00010000。
- 在 PA-220 固定板级头、octeon_common.h 和 board/octeon/pa220/config.mk 中未发现 CONFIG_OCTEON_SPI_BOOT_END 定义。
- cmd_octeon_bootloader.c:96-101 在 CONFIG_OCTEON_MMC 下默认 CONFIG_OCTEON_STAGE3_DEVNAME="mmc"、DEVNO=0。
- 因此：
  - command 本身编入；
  - 并行 NOR update 分支被 CONFIG_SYS_NO_FLASH 排除；
  - SPI bootloader update 分支由 CONFIG_OCTEON_SPI_BOOT_END 控制，在当前已检查的 PA-220 配置链中未启用；
  - eMMC bootloader update 分支因 CONFIG_OCTEON_MMC 编入。
- 最终 .config/链接 map 尚未读取，以上为固定配置源码推导，构建状态 NOT VERIFIED。

## 2. bootloaderupdate 命令入口

- 文件：arch/mips/cpu/octeon/commands/cmd_octeon_bootloader.c。
- do_bootloader_update()（1902-2163）解析 medium、image_addr、image_len、burn_addr、failsafe 标志。
- 支持参数 nand、spi、mmc/sd；SPI 参数处理仅在 CONFIG_OCTEON_SPI_BOOT_END 下编入。
- 默认镜像地址来自 fileaddr/load_addr；长度来自 filesize 或 bootloader header。
- 对非 force_mmc 路径调用 validate_header()/validate_data() 检查 header CRC32、data CRC32；再检查 board_type。
- CRC32 只检测传输/内容损坏，不是数字签名或来源认证。
- 若为 headerless legacy NOR image，会尝试 do_bootloader_update_nor()；但 PA-220 CONFIG_SYS_NO_FLASH，NOR parallel flash 分支不可用。
- Stage2/Stage3/Pre-UBOOT image 在 CONFIG_OCTEON_MMC 下可进入 do_bootloader_update_emmc()。
- U_BOOT_CMD(bootloaderupdate) 注册于 2284-2296；bootloadervalidate 注册于 2298-2302。

## 3. PA-220 eMMC bootloader update 写入路径

### 3.1 阶段识别和设备选择
- emmc_detect_stage()（约 1680-1717）：先读取上传缓冲区前 512 bytes，尝试识别 MBR Stage1；否则将缓冲区解释为 bootloader_header_t，调用 validate_header()/validate_data() 识别 Stage2/3。
- do_bootloader_update_emmc()（1745-1892）默认设备来自 octeon_stage3_devname/octeon_stage2_devname 环境变量，回退到 CONFIG_OCTEON_STAGE3_DEVNAME（默认 mmc）和 device 0。
- 使用 get_dev() 获取 block_dev_desc_t，再读取 LBA0 的 MBR。
- mmc_check_partitions() 检查 MBR 签名、分区表和分区起始 LBA；要求第一分区不能占用前 16 个 sectors 的 Stage1 区域，并检查 bootable partition 数量。

### 3.2 Stage1 写入
- 若输入被识别为 Stage1，复制固定 8192 bytes 到 stage1_buffer。
- 将现有 MBR 的 disk signature/partition table 区域复制到新 Stage1 buffer。
- 调用 dev_desc->block_write(dev_no, 0, 16, stage1_buffer)，覆盖 eMMC LBA0–15（8 KiB）。
- 这是直接 eMMC 持久化写入；未先备份原始 LBA0–15，也没有回滚。
- **边界问题：** emmc_detect_stage() 先读取 512 bytes，Stage1 分支再无条件 memcpy 8192 bytes；没有根据实际上传 filesize 检查输入至少 8192 bytes，短上传可能越界读取 RAM。

### 3.3 Failsafe Stage2 写入
- 对 Stage2/Stage3 header，读取 get_image_size(header)。
- 计算第一分区起始位置前的可用空间；failsafe 模式下检查 Stage2 image 是否能放在 Stage1 后。
- failsafe 模式调用 dev_desc->block_write(dev_no, 0x10, num_sectors, image_addr)，从 LBA0x10（sector 16）写入 Stage2 image。
- 这是 eMMC 原始块写入；没有额外 erase（eMMC block device），也没有写前备份/回滚。
- 非 failsafe 模式对 CN61XX/CNF71XX/CN70XX 只打印使用 fatwrite 将 octboot2.bin 和 u-boot-octeon_*.bin 放到 FAT 分区的说明，不在本函数直接写入最终 Stage3。

### 3.4 不适用于 PA-220 的假设
- 该 eMMC bootloader update 设计假定 Stage1/Stage2 从 eMMC MBR/sector 0x10 启动。
- PA-220 当前项目基线的 Stage3 U-Boot 是 SPI 启动链的一部分，由原厂前级加载，DRAM 已由前级初始化。
- 不能因 CONFIG_OCTEON_MMC 编入就认定 PA-220 应使用 eMMC Stage1/Stage2 布局；当前设备真实 eMMC boot chain/boot partition 角色仍需独立只读确认。
- 本命令对 PA-220 eMMC 的 LBA0–15 或 LBA0x10 写入必须视为高风险，不能作为 Web 更新入口。

## 4. SPI bootloader update 分支：源码存在但 PA-220 配置未启用

- do_bootloader_update_spi()（1554-1607）及 spi_build_map()/spi_reorder_bootloaders()/spi_write_image() 仅在 CONFIG_OCTEON_SPI_BOOT_END 下编译。
- 该分支使用 spi_flash_probe(0,0,CONFIG_SF_DEFAULT_SPEED,SPI_MODE_0)，扫描 CONFIG_OCTEON_SPI_STAGE2_START 到 CONFIG_OCTEON_SPI_BOOT_END 的区域，识别 Stage2/Stage3 header，重排镜像并擦写多个 SPI erase block。
- Stage1 更新会从 SPI offset 0 开始 erase + write；Stage2/Stage3 更新可能搬移现有镜像、擦除旧区并写入新布局。
- 在已检查的 PA-220 固定配置链中没有 CONFIG_OCTEON_SPI_BOOT_END，故不能把这段通用 OCTEON SPI updater 认定为 PA-220 当前可用功能。
- 另有源代码缺陷：do_bootloader_update_spi() 中局部指针 image_map 尚未初始化就执行 memset(image_map, 0, sizeof(image_map))；若该分支被编入，可能对不确定地址写内存。该问题属于未启用分支，不能据此宣称 PA-220 实际执行过。
- 即便将来定义 SPI boot end，该算法也假设 Octeon 通用 Stage1/2/3 连续布局；PA-220 SPI/CPLD 选择与 OEM 前级链必须先单独核实，不可直接开启。

## 5. 与 uBootKit QCA upgrade 的差异

- uBootKit upgrade.c/failsafe.c 使用 QCA flash 逻辑写 HLOS/rootfs/ART/CDT/MIBIB 等分区；这些不是 PA-220 的 OCTEON eMMC/SPI layout。
- PA-220 bootloaderupdate 是 OCTEON 专用命令，具有独立 header、board_type、CRC、Stage1/2/3 和 eMMC/SPI/NAND medium 分派。
- PA-220 没有 QCA SMEM/MIBIB/ART/CDT；PA-220 的 SPI Stage3、eMMC GPT 和 TLV EEPROM 需要独立映射。
- 不可移植 QCA flash 0:APPSBL、flash 0:ART、flash 0:CDT 或 QCA bootconfig 命令。

## 6. 安全结论和项目约束

- PA-220 已编入 bootloaderupdate 命令；这不是普通只读诊断命令。
- eMMC Stage1 路径写 LBA0–15；failsafe Stage2 路径写 LBA0x10 起始区域。
- 当前 PA-220 配置链未发现 SPI updater 所需 CONFIG_OCTEON_SPI_BOOT_END；不代表可用通用 sf write 替代。
- SPI NOR 仍按项目规则只读；eMMC bootloader 写入必须单独备份、审查、授权。
- P1 阶段不执行 bootloaderupdate、mmc write/erase、sf write/erase/update、saveenv。
- 本批未执行任何存储写入、CPLD 写入、设备探测或实机更新；编译和实机状态 NOT VERIFIED。
