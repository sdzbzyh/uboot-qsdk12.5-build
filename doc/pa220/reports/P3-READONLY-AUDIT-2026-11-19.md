# PA-220 P3 只读诊断源码审计（2026-11-19）

- 集成分支：`feature/pa220-ubootkit-web`
- 审计时 HEAD：`5a7e78bcb067f570d44d7816bf9e98bb1f6dc941`
- PA-220 固定源码：`IronDemiGod/pa220-openwrt`
- 固定源码 commit：`801717ba6a8a59ed2cfd53bf3e1b19841fecd2e8`
- 范围：P3 板级、时钟、网络、存储只读诊断源码审计
- 本记录不代表新增命令已构建或实机通过；本轮没有执行设备命令、构建或持久化介质操作。

## 1. 板级信息

### 1.1 可直接读取的缓存数据

PA-220 的 `early_board_init()` 已调用：

- `octeon_board_get_clock_info(PA220_DEF_DRAM_FREQ)`
- `octeon_board_get_descriptor(CVMX_BOARD_TYPE_PA220, 1, 0)`
- `octeon_board_get_mac_addr()`

可在运行时只读 global data，不必再次访问 EEPROM：

- `gd->board_type`
- `gd->arch.board_desc.rev_major`
- `gd->arch.board_desc.rev_minor`
- `gd->arch.board_desc.serial_str`
- `gd->arch.mac_desc.mac_addr_base[6]`
- `gd->arch.mac_desc.count`
- `gd->flags & GD_FLG_BOARD_DESC_MISSING`

Board descriptor 与 MAC descriptor 的填充逻辑位于 `board/octeon/common/octeon_board_common.c`。如果 MAC tuple 不存在，MAC descriptor 会由 `octeon_board_create_random_mac_addr()` 填充，因此输出 MAC base 时不能无条件声称其来自 EEPROM。

### 1.2 初始化副作用边界

`board/octeon/pa220/pa220_board.c` 中的 `early_board_init()` 及其他板级初始化函数不是只读诊断 API：

- `pa220_early_stat_led()` 对 CPLD reg 6/reg 7 执行 read-modify-write；
- GPIO 2/3 被配置为输出低电平，保持 USB 电源关闭；
- `board_configure_qlms()` 配置 QLM；
- `board_prepare_os_handoff()` 修改 CPLD 状态。

运行时诊断不得重新调用这些初始化函数。现有 `pa220 status` 中的 CPLD 读取路径只读 CPLD 映射寄存器；不要把启动初始化中的写操作混入 status 命令。

## 2. 时钟信息

### 2.1 现有 API 的真实语义

`cvmx_clock_get_rate()` 在此 U-Boot 构建中走 `CVMX_BUILD_FOR_UBOOT` 分支，返回 global data：

- `CVMX_CLOCK_CORE` -> `gd->cpu_clk`
- `CVMX_CLOCK_SCLK` / `CVMX_CLOCK_TIM` / `CVMX_CLOCK_IPD` -> `gd->bus_clk`
- `CVMX_CLOCK_DDR` -> `gd->mem_clk * 1000000`

因此它是已初始化的时钟值，不是独立频率计数器测量值。

### 2.2 PA-220 值的来源

- CPU：`early_board_init()` 以 `octeon_get_cpu_multiplier()` 读取 CN70xx `CVMX_RST_BOOT.c_mul`，乘以 `DEFAULT_CPU_REF_FREQUENCY_MHZ` 后写入 `gd->cpu_clk`。
- IO：`gd->bus_clk` 由 U-Boot/Stage2 初始化路径提供；本轮尚未取得该字段在本次 Stage2 handoff 下的独立原始串口值。
- DDR：`gd->mem_clk` 优先来自 TLV DDR clock descriptor；未找到有效 tuple 时使用 `PA220_DEF_DRAM_FREQ` 默认值。板级显示代码以 `gd->mem_clk` 作为 DDR 时钟，并显示双倍数据率。

后续 `pa220 clock` 若新增，应明确标注 core/io 为 global-data 计算/传入值，DDR 为 EEPROM descriptor 或 fallback 配置值；不得称为独立实测值。DDR 实际 PLL/DFM 频率若要输出，需要单独审计对应 CSR 的型号语义。

## 3. 网络诊断

### 3.1 当前 status 的边界

现有 `pa220 status` 显示 `ethact`、`ethprime`、`ipaddr`、`serverip`、`gatewayip`。其中 `ethact` 是当前选择的 Ethernet 设备环境变量，不是 PHY carrier/link-up 状态。现有 status 不构成实时链路检测。

### 3.2 为什么暂不调用通用 link API

`cvmx_helper_link_get()` 除调用接口 link_get 外，在 U-Boot 分支还会调用：

- `__cvmx_update_link_led()`
- `cvmx_sfp_check_mod_abs()`（满足条件时）

因此它不是严格无副作用的只读诊断 API。以太网驱动的链路更新路径还可能调用 `cvmx_helper_link_set()`、`board_net_set_link()` 并修改 MAC/PCS 配置。

本轮不把 `cvmx_helper_link_get()` 或 `cvm_oct_configure_rgmii_speed()` 纳入只读命令。PHY status 寄存器还需审查 latch-low/read-clear 语义；在完成该审计前，不运行 `mii`/MDIO link 探测来声称物理链路状态。

可安全保留的现有网络信息是环境配置与已选择接口；物理链路状态标记为 **NOT VERIFIED**。

## 4. 存储诊断

### 4.1 SPI NOR

PA-220 配置及 DTS 指明：

- Winbond W25Q128；
- SPI bus 0 / chip-select 0；
- 配置最大 SPI 时钟 16 MHz；
- DTS 容量 0x01000000（16 MiB）；
- `CONFIG_SPI_FLASH_WINBOND`、`CONFIG_CMD_SF` 已定义。

`sf probe` 通过 SPI 读取 JEDEC ID 并识别 flash 参数。当前配置限定 Winbond；`sf_probe.c` 中的 probe-time status-register 写操作仅在 ATMEL、MACRONIX、SST 对应编译选项分支启用，PA-220 的 Winbond 配置不启用这些分支。16 MiB 容量也不触发大于 16 MiB 的 BAR bank-register 路径。

因此，源码审计认为 `sf probe 0:0` 可作为后续受控只读识别候选；尚未执行，状态 **NOT RUN**。禁止 `sf write`、`sf erase`、`sf update`。

注意：PA-220 配置实际定义 `CONFIG_ENV_IS_IN_SPI_FLASH`，环境区位于 SPI NOR 0xe00000/0xe10000。不得依据配置文件开头“环境仅在 RAM”注释推断 `saveenv` 安全；现有 `pa220 env` 的警告必须继续保留，禁止 `saveenv`。

### 4.2 eMMC

PA-220 DTS 声明 eMMC non-removable、8-bit bus；板级配置最大频率 26 MHz，且定义 `CONFIG_OCTEON_MMC_NO_RST_N` 防止写入 OTP EXT_CSD RST_n_FUNCTION。

但通用 `mmc_init()` / `mmc_startup()` 并非纯读流程：源码会通过 MMC SWITCH/CMD6 修改 EXT_CSD 的 HS_TIMING、ERASE_GROUP_DEF、BUS_WIDTH 等配置字段。虽然这些操作不是用户数据区 block write，仍属于向 eMMC 发送配置写命令。

在本项目“禁止 eMMC 写入”的约束下，不执行可能触发初始化的 `mmc info`、`mmc rescan`、`mmc part`、`ext2ls`、`ext2load` 等命令。不得假设 `mmc_init()` 一定已完成；只有获得明确的既有初始化状态且不触发重初始化的只读 API 后，才考虑显示运行时容量/分区。

eMMC 运行时容量、分区及文件系统状态目前 **NOT VERIFIED**。

### 4.3 USB Mass Storage

PA-220 定义 `CONFIG_OCTEON_USB_OCTEON3`；公共配置由此启用 `CONFIG_CMD_USB` 和 `CONFIG_USB_STORAGE`。但板级早期初始化将 GPIO 2/3 拉低以保持 USB 电源关闭。

本轮不执行 `usb start` 或块设备扫描。USB 控制器/存储枚举状态 **NOT VERIFIED**；后续必须先审查 USB 初始化及电源 GPIO 的副作用。

## 5. 下一步实现建议

1. 新增只读 `pa220 board`：仅显示已缓存 board descriptor、serial、revision、MAC base/count 和 descriptor-missing 标志；不重新访问 EEPROM。
2. 新增只读 `pa220 clock`：显示 CORE、SCLK、DDR global-data 值及其来源/限制；不做初始化或 PLL 写操作。
3. 存储先记录配置与安全边界；可在后续 RAM 实机窗口单独执行 `sf probe 0:0`，不执行任何写命令。eMMC/USB 暂不主动初始化。
4. 网络保留 ethact/ethprime/IP 配置显示；实时 link 状态暂不实现，直到找到无 LED、无 SFP、无 MAC/PCS 修改且已确认无 read-clear 副作用的查询路径。
5. 不重复已通过的 `pa220 status/env/memory`、`bdinfo`、named/free bootmem 测试。

## 6. 本轮结果状态

| 项目 | 状态 | 说明 |
|---|---|---|
| 板级缓存信息 API | PASS（源码审计） | global data 可直接读取 |
| 板级初始化副作用边界 | PASS（源码审计） | 已识别 CPLD/GPIO/QLM 写路径 |
| CORE/IO/DDR 时钟 API | PASS（源码审计） | 返回 global data，不是独立实测 |
| 网络环境状态 | PASS（既有实机测试） | ethact 等环境项已显示 |
| 网络物理 link 状态 | NOT VERIFIED | 通用 link_get 有 LED/SFP 副作用 |
| SPI NOR probe | NOT RUN | 仅列为后续候选 |
| eMMC 只读初始化 | BLOCKED | mmc_init/startup 会发 EXT_CSD CMD6 写命令 |
| USB 枚举 | NOT RUN | USB 电源 GPIO 默认拉低，初始化副作用未审 |
| 新增 board/clock 命令 | NOT BUILT | 待形成下一补丁 |
| 本轮构建/实机测试 | NOT RUN | 本轮仅源码审计与文档更新 |

## 7. 持续安全边界

不执行 `saveenv`、EEPROM/TLV 写入、CPLD 写入、SPI NOR/eMMC/NAND/USB block 写入、擦除、分区、格式化或固件安装；不执行 `bootloaderupdate`、`namedalloc`、`namedfree`、`mtest`。不开放任意地址 RAM 上传。统一 cached/uncached/physical alias 及 pre-write validator 尚未完成。


## 8. 第二个只读诊断补丁（源码变更候选）

新增补丁：`patches/pa220/0002-pa220-board-clock-readonly-diagnostics.patch`。

新增：
- `pa220 board`：读取早期初始化后缓存的 board descriptor、revision、serial、MAC base/count 和 descriptor-missing 标志；不重扫 EEPROM。
- `pa220 clock`：读取 `cvmx_clock_get_rate(CORE/SCLK/DDR)`，并明确显示这些值来自 global data/初始化配置，不是独立频率测量。

补丁顺序：
- `build-pa220.sh` 按 `patches/pa220/*.patch` 字典序应用补丁；
- 因而 0001 先应用，0002 再应用；
- 已对固定源码文件按 0001 -> 0002 顺序执行 unified-hunk 上下文和行数核对，所有 hunk 均可顺序匹配。

尚未在 Debian 构建目录实际执行 `git apply --check`、构建或设备测试；因此 0002 状态仍为 **NOT BUILT / NOT VERIFIED**。下一步由用户在原固定 checkout 执行正式构建前的 patch check，再决定构建。


## 9. 最终状态更正与阶段交接（2026-11-19）

本文件第 6、8 节中的 `NOT BUILT / NOT VERIFIED` 是 board/clock 补丁实施前的历史源码审计状态，已被后续构建和实机证据取代，不再代表当前状态。

- **P3 CLI/只读诊断：COMPLETE。**
- 0002 已构建并通过原厂 Stage2/TFTP RAM 启动；`help pa220`、`pa220 board`、`pa220 clock` 已实机验证。
- 完整 board/clock 串口日志：集成仓库根目录 `reports/P3-BOARD-CLOCK-2026-11-19-serial.log`。
- board/clock 实机结果汇总：集成仓库根目录 `reports/P3-READONLY-AUDIT-2026-11-19.md`。
- P3 全阶段收尾表及 P4 交接见 `doc/pa220/TEST-PLAN.md` 第 7、8 节。

P3 COMPLETE 不改变以下状态：MAC descriptor 来源未单独证明；时钟是 global-data/configuration 值而非独立测量；Ethernet 物理 link NOT VERIFIED；eMMC 初始化 BLOCKED；USB 枚举 NOT VERIFIED；统一 RAM 写入 validator 未实现，TFTP/HTTP/ELF 任意接收保持 BLOCKED。P1 非阻断风险继续保留。

本文件早期关于通用 link API、eMMC/USB 初始化副作用和只读探测边界的审计结论仍有效；本次只更正历史阶段状态，不撤销风险约束。


## 10. SPI probe 历史时间来源补充（2026-11-19 审计）

本报告中该次源码审计的 `sf probe` 状态为 NOT RUN，含义是当时未执行新的 probe。历史用户消息中另有 2026-11-18 10:10:39Z 的先前只读输出：`SF: Detected W25Q128BV with page size 256 Bytes, erase size 4 KiB, total 16 MiB`。该观察早于 P3 开始，作为继承证据供 P3 汇总引用；不应描述为 P3 当日实测。详见 `../test-logs/2026-11-18_SPI-NOR-probe.session-transcribed.txt`。
