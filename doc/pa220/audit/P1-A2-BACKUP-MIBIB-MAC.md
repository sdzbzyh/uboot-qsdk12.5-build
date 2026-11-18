# P1-A2：Web 业务模块审计（第三组：backup.c、mibib.c、mac.c）

- 状态：**本组源码静态审计完成；未移植、未编译、未实机验证**
- 日期：2026-11-18
- uBootKit：chenxin527/uboot-qsdk12.5-build@9ce315735ac88fd0a7c7697bc527a8616ed0a972
- PA-220：sdzbzyh/pa220-openwrt@801717ba6a8a59ed2cfd53bf3e1b19841fecd2e8
- 文档分支：feature/pa220-ubootkit-web
- 前置：P1-A2-READONLY-WEB-MODULES.md、P1-A2-MISC-WEBTERM.md
- 范围：backup.c、mibib.c、mac.c。upgrade.c 尚未审计，仍保留为 P1-A2 最后一项。

## 1. 构建与配置关系

- u-boot-2016/Makefile:704：CONFIG_HTTPD 控制 failsafe 目录进入构建。
- failsafe/Makefile：非 SPL 构建编入 modules/、fs.o、failsafe.o。
- failsafe/modules/Makefile：backup.o、mibib.o 为 obj-y；mac.o 由 CONFIG_FAILSAFE_MAC_MANAGEMENT 控制。
- failsafe/Kconfig：CONFIG_FAILSAFE_MAC_MANAGEMENT 默认 n；打开后提供 MAC_TYPE_RAW_HEX/MAC_TYPE_ASCII 选择，以及 MAC_PART_NAME、MAC_OFFSET_IN_PART、MAC_LABEL、MAC_COUNT 配置。
- 以上只确认源码和 Kconfig/Makefile 规则；未确认具体目标 .config 是否启用 MAC 管理，未编译。

## 2. backup.c：存储只读备份

### 2.1 URI 与参数
- `backup_register_uri_handlers()`（514-517）注册 POST /backup -> `backup_handler()`（214-512）。
- 前端 main.js 约 3031、3179 行请求 /backup，使用 FormData。
- 表单字段：mode、target；mode 为 full 或 range；range 额外使用 start、size。
- target 前缀：raw:、smem:、mmc:。raw 支持 spi/mmc/nand；smem 通过 IPQ 分区名解析；mmc 通过 GPT 分区名解析。
- 处理器未显式检查 request->method；依赖表单字段存在与否进行处理。

### 2.2 调用链和底层访问
- `parse_u64_len()`（107-138）：解析整数及 K/KB/KiB、M/MB/MiB 单位。
- `smem_read_data()`（140-183）：SPI 分支调用 spi_flash_probe()/spi_flash_read()；NAND 分支调用 nand_read_skip_bad()，必要时先读对齐缓冲再 memcpy。
- `mmc_read_data()`（185-212）：按 block size 计算块范围，分配临时缓冲，调用 block_read()，再复制目标区间。
- `backup_handler()` NEW：解析 target/mode/range，分配 64 KiB 数据缓冲，确定 SPI/NAND/MMC 设备或分区基址/大小，生成下载响应头。
- RESPONDING：每次读取最多 64 KiB，按 HTTP_RESP_CUSTOM 流式返回；CLOSED 释放缓冲和 session。
- 没有调用 spi_flash_erase/write、nand erase/write 或 block_write；本模块设计目标是读取/导出，不直接修改持久化存储。

### 2.3 全局状态与平台专有依赖
- 依赖全局 mmc_host（qca_mmc/sdhci_host）、nand_info、CONFIG_SF_DEFAULT_*。
- SMEM 路径依赖 getpart_offset_size()、qca_smem_flash_info、get_which_flash_param() 和 SMEM flash 类型。
- 依赖 U-Boot SPI/NAND/MMC 读 API、GPT 分区查询。
- 数据本身通过 HTTP 发送给客户端，具有完整存储内容外泄风险；无认证/授权检查的情况下不能暴露 raw 全盘导出。

### 2.4 边界和错误处理风险
- `parse_u64_len()` 的乘法换算未检查 u64 溢出。
- `start_offset + total_size > target_size`（约 365 行）未使用减法式边界检查，start+size 溢出可能绕过范围校验。
- `mmc_read_data()` 使用 `(offset + size - 1)`；size 为 0 时下溢，offset+size 也可能溢出；临时分配大小乘法未检查溢出。
- `mmc_read_data()` 在 block_read 返回块数不足时仍先 memcpy，再返回错误，可能复制未完整初始化的数据。
- `smem_read_data()` 的 SPI 分支未检查 spi_flash_probe() 返回指针；NAND 未对 writesize 为 0 做保护；未对所有目标做独立容量边界检查。
- NAND 未对齐读取分支在 nand_read_skip_bad() 返回错误后仍执行 memcpy，需先检查 ret。
- 响应头 snprintf() 返回长度未检查负值/截断。
- 必须将 range 验证、整数溢出、设备容量、读取失败与响应头长度作为移植重写项。

### 2.5 PA-220 对照
- PA-220 固定配置启用 OCTEON MMC/MMC、OCTEON SPI/SPI_FLASH、EFI/GPT；没有 Qualcomm SMEM/NAND 分区表机制。
- PA-220 eMMC GPT 分区读取可以研究复用其旧版 block device/GPT API，但必须核对 U-Boot 2013.07 的结构体、块读取返回值和缓存对齐要求。
- PA-220 SPI NOR 可作为只读备份对象研究；但需要先明确实际 SPI 芯片/片选/正常与备用 Flash 选择边界。不得在备份功能中写 CPLD flash-select，也不得假设 IPQ CONFIG_SF_DEFAULT_* 与 PA-220 一致。
- PA-220 没有 NAND，raw:nand 与 SMEM NAND 分支不适用。
- 结论：可重写 eMMC GPT 和经过硬件选择确认后的 SPI 只读备份；SMEM 分区路径不适用。第一版应默认禁用全盘原始导出，仅提供经过白名单确认的只读对象。

## 3. mibib.c：IPQ MIBIB 上传解析与运行时分区刷新

### 3.1 入口与执行路径
- `mibib_register_uri_handlers()`（145-148）注册 POST /mibib/reload -> `mibib_reload_handler()`（34-142）。
- 前端上传页面为 /mibib.html；main.js 上传流程对 mibib 特殊使用 /mibib/reload（main.js 约 1969 行）。
- 读取 multipart 字段 mibib，调用 check_fw_type() 判别 MIBIB_NAND 或 MIBIB_NOR。
- NAND 分支调用 has_nand()，更新 qca_smem_flash_info 的 flash_type/flash_block_size/flash_density。
- NOR 分支调用 has_spi()、spi_flash_probe()，更新同一 SMEM flash info。
- 随后调用 get_mibib_and_ptable_addr() 解析上传数据，调用 mibib_ptable_init() 更新运行时分区表，再调用 get_kernel_fs_part_details()。
- 通过 handle_fail_led_state()/handle_success_led_state() 修改 IPQ failsafe LED 状态。
- 本 handler 没有调用 SPI/NAND erase/write；从本文件可见的是上传文件解析和运行时全局分区状态刷新，不应把它误称为持久化 MIBIB 刷写。

### 3.2 全局状态/风险/平台边界
- 修改 qca_smem_flash_info 全局字段，并调用 Qualcomm MIBIB/SMEM 分区表函数。
- 依赖 is_9008_mode()、has_nand()/has_spi()、SMEM、MIBIB 固件解析和 IPQ LED 状态函数。
- SPI probe 结果未判空即读取 spi->size。
- 未显式检查 request method；上传长度由 check_fw_type/get_mibib_and_ptable_addr 内部处理，本文件未证明其全部边界行为。
- PA-220 无 Qualcomm 9008 模式、SMEM/MIBIB 分区机制；PA-220 使用 eMMC GPT 与板级 SPI 布局。不能移植此模块或将 MIBIB 当成 PA-220 分区格式。
- 适配价值仅限于“上传文件先解析、只更新 RAM 中解析结果、安装前预览”的工作流思想；具体解析器必须替换为 PA-220 GPT/镜像格式识别器。
- 本模块不应写入 PA-220 SPI/eMMC，也不应被映射为 CPLD/Flash 切换操作。

## 4. mac.c：MAC 地址读取与持久化修改

### 4.1 注册、配置和参数
- `mac_register_uri_handlers()`（548-552）注册 GET /mac/info、POST /mac/set。
- 模块仅在 CONFIG_FAILSAFE_MAC_MANAGEMENT 下编入。
- Kconfig 默认关闭；若开启，MAC 类型默认 RAW_HEX，也可选择 ASCII。
- 配置项：MAC_PART_NAME（默认 0:ART）、MAC_OFFSET_IN_PART、MAC_LABEL、MAC_COUNT。
- 前端 main.js 约 7194 行 POST /mac/set，字段 mac_data；约 7250 行 GET /mac/info。

### 4.2 MAC 定位和读写调用链
- `get_mac_part_offset_size()`（73-111）根据 qca_smem_flash_info.flash_type 决定使用 SMEM 分区偏移或 MMC GPT 分区查询。
- `get_mac_data_offset_size()`（113-200）：
  - RAW_HEX：partition offset + CONFIG_MAC_OFFSET_IN_PART，长度为 MAC_COUNT * 6。
  - ASCII：要求 MAC_COUNT==1，读取整个分区并搜索 CONFIG_MAC_LABEL，定位标签后的 MAC 字符串。
- `read_write_mac_data()`（202-253）按 flash_type 分派：
  - SPI -> read_data_from_spi()/write_data_to_spi()
  - NAND -> read_data_from_nand()/write_data_to_nand()
  - MMC -> read_data_from_mmc()/write_data_to_mmc()
- `mac_info_handler()`（452-509）读取 MAC 数据并生成 JSON。
- `mac_set_handler()`（511-546）解析用户提交的 MAC 列表/ASCII 字符串，再调用 WRITE_MAC_DATA 分支。
- MAC 修改不是环境变量 setenv，也不是只改运行时地址；它会直接修改持久化存储。

### 4.3 底层持久化写入证据
uBootKit `lib/flashrw.c`：
- `write_data_to_spi()`（178-247）：读出覆盖扇区，修改内存副本，调用 spi_flash_erase() 后 spi_flash_write() 写回整个扇区。
- `write_data_to_nand()`（249-323）：读出覆盖 NAND erase block，调用 nand_erase_opts()，再 nand_write_skip_bad(..., WITH_WR_VERIFY)。
- `write_data_to_mmc()`（325-383）：block_read() 读块，修改缓冲，再 block_write() 写回。
- 这些路径是明确的 SPI NOR/NAND/eMMC 持久化写入；MAC 页面不能移植为默认开放的写接口。
- helper 的 offset+size、size=0、扇区/块数量乘法等仍需审查溢出和边界；不能仅凭存在读改写逻辑就认定对 PA-220 安全。

### 4.4 PA-220 MAC/EEPROM 对照
- PA-220 octeon_pa220.h:50-61 配置板载 TLV EEPROM 地址 0x57，并将 EEPROM 地址长度设为 1 byte；注释说明 EEPROM 保存 board type、serial、MAC base。
- octeon_pa220.h:98 启用 CONFIG_CMD_OCTEON_TLVEEPROM；octeon_cmd_conf.h 启用 CONFIG_CMD_EEPROM 和 CONFIG_CMD_I2C。
- PA-220 board/octeon/pa220/pa220_board.c:130-136 的 early_board_init() 调用 octeon_board_get_descriptor() 与 octeon_board_get_mac_addr()，从 TLV EEPROM 获取板卡描述和 MAC 地址。
- PA-220 的 MAC 来源是 TLV EEPROM/板级 OCTEON API，不是 IPQ ART/MIBIB/SMEM 分区布局；不能照搬 MAC_PART_NAME、MAC_OFFSET_IN_PART、SMEM flash_type 或 QCA 写入路径。
- 通用 EEPROM 驱动存在 eeprom_read()/eeprom_write()，且 CONFIG_CMD_EEPROM 已启用；这说明源码具备 EEPROM 读写命令路径，不代表已验证板载 TLV EEPROM 可任意写入，也不构成写入授权。
- 可适配方向：先实现 PA-220 TLV EEPROM/MAC base 的只读展示，复用 octeon_board_get_mac_addr() 或经确认的 TLV parser；任何 EEPROM 写入必须独立审查、备份和授权。不得通过 Web 暴露通用 eeprom write。

### 4.5 MAC 模块健壮性问题
- `mac_set_handler()` 未显式检查 HTTP method。
- RAW_HEX 分支的 offset+size 未在本文件中与分区边界完整校验。
- ASCII label 搜索循环对剩余长度的检查只保证 label_len，不保证后续分隔符和 MAC 字符串的完整读取范围。
- `mac_info_handler()` 使用固定 1,024 字节 JSON 缓冲，snprintf 累计长度没有统一边界检查。
- flashrw 写 helper 会对覆盖扇区/块做 read-modify-erase/write；断电或写失败可能破坏原数据，不能作为无风险操作。

## 5. 本组适配结论

| 模块 | uBootKit 功能 | PA-220 对照 | 适配结论 |
|---|---|---|---|
| backup | SPI/NAND/MMC/SMEM 分区备份 | eMMC GPT、SPI NOR；无 NAND/SMEM | eMMC GPT 与确认后的 SPI 只读备份可重写；先做边界/设备选择审查 |
| mibib | Qualcomm MIBIB 解析和 SMEM 分区刷新 | PA-220 使用 GPT/板级布局 | MIBIB 机制不适用；只借鉴上传解析/dry-run 工作流 |
| mac | ART/SMEM/SPI/NAND/MMC MAC 读写 | TLV EEPROM，OCTEON board MAC API | 只读 MAC 信息可适配；QCA MAC 写入模块不移植 |

## 6. 安全与未完成状态

- 本批未执行任何 SPI/eMMC/NAND/EEPROM 写入，也未调用 CPLD 写操作。
- backup 源码路径只读，但具备全盘/分区内容导出能力，需控制可访问对象。
- mibib 本文件执行上传解析和 RAM 分区状态刷新，不是持久化写入实现。
- mac 的写路径明确包含 SPI erase/write、NAND erase/write、MMC block_write；PA-220 的 EEPROM 写入也必须视为持久化操作。
- CONFIG_FAILSAFE_MAC_MANAGEMENT 默认 n；具体目标配置启用状态 UNKNOWN。
- 编译成功、RAM 启动和实机验证均 NOT VERIFIED。
- P1-A2 最后一项 upgrade.c 尚未开始；其写入路径审计完成前，不能宣称 P1-A2 完成。
