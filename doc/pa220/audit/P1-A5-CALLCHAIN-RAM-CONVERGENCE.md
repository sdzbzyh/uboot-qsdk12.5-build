# P1-A5：PA-220 硬件调用链与 RAM 安全区收敛补充

- 状态：**本补充批次完成源码调用链收敛；TLV EEPROM 实际容量仍 UNKNOWN；RAM 最终运行时地址仍需实机确认**
- 日期：2026-11-18
- PA-220 固定源码：sdzbzyh/pa220-openwrt@801717ba6a8a59ed2cfd53bf3e1b19841fecd2e8
- uBootKit 固定审计源码：chenxin527/uboot-qsdk12.5-build@9ce315735ac88fd0a7c7697bc527a8616ed0a972

## 1. 已确认的 P1-A5 启动调用链

PA-220 使用 OCTEON 专用 `arch/mips/lib/board_octeon.c`，而不是通用 `arch/mips/lib/board.c` 的初始化序列。

`board_init_f()` 的实际 `init_sequence[]` 顺序中：

1. `board_fdt_init_f()`
2. `init_twsi_bus()`
3. `board_early_init_f()`
4. `octeon_boot_bus_init()`
5. `early_board_init()`
6. `octeon_env_init()`
7. `octeon_import_ram_env()`
8. `init_baudrate()` / serial / console
9. `octeon_init_dram()`
10. `octeon_dram_test()`
11. `checkboard()` / `display_board_info()`

因此 PA-220 的 `early_board_init()` **确实进入正式启动链**，且发生在 DRAM 初始化之前。

`pa220_board.c:early_board_init()` 当前执行：

- `pa220_early_stat_led()`：重新映射 CPLD CS0，并写 reg7/reg6；
- GPIO2/GPIO3 拉低，保持 USB power off；
- `octeon_board_get_clock_info()`；
- `octeon_board_get_descriptor()`；
- `octeon_board_get_mac_addr()`；
- 根据 EEPROM/板级信息设置 `gd->cpu_clk`。

这意味着 TLV EEPROM 是早期板级初始化的实际输入，不是仅供 Web 展示的附属数据。

## 2. CPLD boot-watchdog 调用链已收敛

`early_board_init()` 之前已经由 `octeon_boot_bus_init()` 完成基本 boot-bus 初始化。

随后在 `board_init_r()`：

1. `octeon_boot_bus_moveable_init()`
2. `octeon_configure_load_memory()`
3. `octeon_configure_linux_reserved_memory()`
4. 启动可选硬件 watchdog
5. `octeon_boot_bus_late_init()`

而 `octeon_boot_bus_late_init()` 最终调用：

`octeon_boot_bus_board_post_init(gd->fdt_blob)`

PA-220 实现该 weak hook，并执行：

- 检查 boot-bus CS0 是否映射到 0x1b040000；
- 读取 CPLD reg d；
- reg d bit0 low -> high pulse；
- 每次间隔 10 ms；
- 最后只读打印 CPLD version/reg5/reg d。

因此此前“post-init hook 是否可达”的 UNKNOWN 已收敛为：

**CALLABLE / 当前正式 board_init_r() 启动链会执行。**

该 reg d 写操作属于已有板级初始化行为，不是本项目新增写操作，应保留并在实机测试时重点观察 watchdog/failsafe SPI 切换行为。

## 3. QLM 调用链已收敛

`board_init_r()` 在 boot-bus late init 后执行：

`board_configure_qlms()`

PA-220 实现：

`octeon_configure_qlm(0, 2500, CVMX_QLM_MODE_QSGMII_QSGMII, 0, 0, 0, 1)`

因此：

- DLM0/QSGMII 配置不是“孤立函数”；
- 它确实进入当前 board_init_r()；
- 调用发生在网络初始化之前；
- 不需要新增 QLM 调用；
- 不应在 Web/诊断路径重复调用。

当前结论：

**DIRECT REUSE / EXISTING BOARD INITIALIZATION**

后续只需实机确认 QSGMII link/PHY 状态，不再把“是否调用”作为 P1 阻塞项。

## 4. MDIO/PHY 调用链已收敛

`board_init_r()` 网络阶段：

1. `board_net_preinit()`
2. `eth_initialize(gd->bd)`
3. `board_mdio_init()`
4. `board_net_postinit()`

`eth_initialize()` 本身会进入 CPU-specific `cpu_eth_init()`；PA-220 CPU 路径先执行：

`octeon_mdio_initialize()`

随后初始化 OCTEON Ethernet ports。

然后再执行 PA-220 的：

`board_mdio_init() -> __board_mdio_init() -> octeon_board_phy_init()`

`octeon_board_phy_init()` 根据 FDT PHY compatible 和 `marvell,reg-init` 配置 88E1680/88E1512。

因此当前顺序为：

**OCTEON MDIO bus -> Ethernet device registration -> board_mdio_init -> FDT PHY register initialization**

结论：

- 不移植 IPQ switch/PPE PHY 初始化；
- 不重复初始化 MDIO；
- 不新增 Web MDIO arbitrary write；
- 后续实机只读验证重点是 PHY ID、MDIO 地址和 link 状态。

## 5. TLV EEPROM：地址已确认，容量仍 UNKNOWN

源码明确：

- I2C slave address = `0x57`；
- page write size = 64 bytes；
- page write delay = 5 ms；
- `CONFIG_SYS_I2C_EEPROM_ADDR_LEN = 1)。

`common/cmd_eeprom.c` 对 addr_len=1 的实现不是普通 24C256 的 16-bit internal address，而是：

- `offset & 0xff` 作为页内 offset；
- `offset >> 8` 作为 I2C device-address selector；
- 实际 I2C transfer address length = 1 byte。

因此：

**不能仅凭 `/* 24c256: 64 bytes */` 注释把实际器件认定为 24C256/32 KiB。**

目前源码能可靠确认：

- slave address：**0x57**
- addressing scheme：**8-bit internal/page offset + device-address block selector**
- page size：**64 bytes（配置值）**
- 实际 EEPROM 型号：**UNKNOWN**
- 实际有效容量：**UNKNOWN**
- 是否存在多个 bank/address selector：**需要硬件实测**
- TLV 最大 tuple length：**256 bytes**
- parser 没有独立的物理 EEPROM 容量参数

因此 P1 不再猜测“32 KiB”。

### 实机只读验证要求

下一阶段只读验证至少需要：

1. I2C bus 扫描确认 0x57；
2. 从 offset 0 开始分段读取；
3. 在可能的 bank selector 变化处观察是否存在镜像/连续数据；
4. 找到 END tuple；
5. 验证 MAC、board descriptor、clock tuple 的 offset；
6. 确认最后可响应地址，从而确定实际容量/有效 TLV 区间。

禁止使用 `eeprom write`、`tlv_eeprom set/delete` 做验证。

## 6. RAM 运行布局：已从“固定地址猜测”转为运行时公式

PA-220 OCTEON U-Boot 不把 U-Boot 固定放在 DRAM 起始地址。

`board_init_f_finish()`：

- 从 `gd->ram_size` 计算 U-Boot 顶部；
- CN63XX 及更新型号使用：

`u_boot_mem_top = 0x20000000 + (gd->ram_size - 0x10000000)`

- U-Boot 长度向上按 64 KiB 对齐；
- 从顶部向下预留 U-Boot；
- 再预留 `TOTAL_MALLOC_LEN`；
- 再预留 `STACK_SIZE`；
- 根据 TLB mapping size 对齐；
- 最终得到 `bd->bi_uboot_ram_addr`。

因此：

**最终 U-Boot/heap/stack 地址必须由运行时 `gd->ram_size`、实际链接长度和 TLB 对齐计算，不能在 P1 写死。**

已确认配置：

- `CONFIG_SYS_TEXT_BASE = 0xC0000000`
- `CONFIG_SYS_MALLOC_LEN = 12 MiB`
- `CONFIG_SYS_BOOTPARAMS_LEN = 128 KiB`
- `STACK_SIZE = 64 KiB + CONFIG_SYS_BOOTPARAMS_LEN = 192 KiB`

## 7. 低地址固定保留区

OCTEON 内存映射定义：

- 前 1 MiB 属于 bootloader 固定保留区；
- exception/FDT/bootmem descriptor 等位于该区域；
- `OCTEON_FDT_BASE = 512 KiB`
- `OCTEON_FDT_MAX_SIZE = 128 KiB`
- U-Boot exception handler 从 FDT 后按 4 KiB 对齐；
- `OCTEON_RESERVED_LOW_BOOT_MEM_SIZE = 1 MiB`
- idle-core loop 位于约 1 MiB 附近。

因此任何 RAM upload buffer 都不能落入物理 0..1 MiB。

## 8. 关键：0x81000000 不能直接视为安全上传地址

项目既有 D-005 记录了 0x81000000 作为既有 TFTP RAM 加载资料地址。

但当前 U-Boot 在 board_init_r() 中还会执行：

`octeon_configure_linux_reserved_memory()`

默认情况下，RAM >= 1 GiB 时：

- Linux reserved size = min(ram_size / 2, 240 MiB)；
- 自动 base = `OCTEON_LINUX_RESERVED_MEM`；
- 该 base 当前为 1 MiB。

对于 PA-220 的大内存配置，默认 Linux reserved 区会从约 1 MiB 开始覆盖到约 241 MiB 物理地址。

因此如果将 0x81000000 按 cached DRAM alias 解读为物理 DRAM 起始 + 16 MiB，那么它落在默认 Linux reserved 区内。

结论：

**0x81000000 不能在当前源码证据下继续作为 Web/TFTP 任意上传的“安全地址”。**

D-005 仍然保留“链接地址与加载地址必须区分”的决策，但 0x81000000 的安全性需要在 P2/P6 重新处理。

## 9. 当前已有的专用 load-memory 机制

`octeon_configure_load_memory()` 会创建 bootmem named block：

`OCTEON_BOOTLOADER_LOAD_MEM_NAME`

并设置：

- `loadaddr`
- `load_reserved_addr`
- `load_reserved_size`

对于大于 256 MiB 的 RAM，其默认 load reservation 最大为 256 MiB。

对于大内存 PA-220，自动算法最终会把默认 load block 放到约 512 MiB 物理地址附近，而不是低端 16 MiB。

这比固定写死 0x81000000 更接近正确的 RAM upload 设计。

但目前仍有一个关键问题：

**源码中的 `loadaddr` 是以 U-Boot 地址空间使用的值设置，而 OCTEON MIPS64/n32 的物理地址、TLB mapping、cached/uncached alias 之间仍需要实机确认。**

所以目前：

- load reservation：**可作为候选安全区**
- 固定 `0x81000000`：**否**
- 最终 Web upload 地址：**RUNTIME-COMPUTED**
- 最终上传长度：**必须受 `load_reserved_size` 限制**
- 上传校验：**必须先做边界检查，再写 RAM**
- 不允许用 `gd->ram_size` 单独判断安全性。

## 10. 当前 RAM 安全区结论

| 区域 | 当前结论 |
|---|---|
| 物理 0..1 MiB | **禁止覆盖** |
| U-Boot 实际 relocation 区 | **禁止覆盖；运行时计算** |
| U-Boot malloc arena | **禁止覆盖；运行时计算** |
| U-Boot stack/GD/bd/bootparams | **禁止覆盖；运行时计算** |
| Linux reserved named block | **禁止覆盖** |
| load-memory named block | **首选候选上传区** |
| 0x81000000 | **不能作为默认安全地址** |
| `CONFIG_SYS_LOAD_ADDR=0x20000000` | **仅配置候选，不等于安全上传区** |
| 最终 Web/TFTP buffer | **必须运行时从 reserved block 计算** |

## 11. P1 当前剩余阻塞项

本补充后，P1-A5 剩余主要不是“函数是否被调用”，而是：

1. TLV EEPROM 实际器件/容量/有效 TLV 边界：**UNKNOWN，需只读硬件验证**
2. RAM load-memory 的最终虚拟/物理地址映射：**UNKNOWN，需运行时确认**
3. 实际链接长度/最终 relocation 地址：**需要最终构建 map/实机启动日志**
4. 板级时序的硬件结果：PHY link、CPLD watchdog/failsafe 行为仍需实机验证

以上均不允许在 P1 中用猜测替代证据。
