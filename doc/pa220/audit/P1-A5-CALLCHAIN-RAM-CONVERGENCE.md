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


## 12. 第二轮源码收敛：TLV EEPROM 地址语义与有效访问窗口

本轮继续直接核对 PA-220 固定源码树 `sdzbzyh/pa220-openwrt@801717ba6a8a59ed2cfd53bf3e1b19841fecd2e8` 中实际随项目保存的 U-Boot 源码，而不是仅依赖前一轮摘要。

### 12.1 FDT/板级描述只能确认“配置意图”，不能确认物理容量

`u-boot/board/octeon/pa220/pa220.dts` 的 I2C0 节点明确：

- I2C 控制器：`cavium,octeon-3860-twsi`
- CSR：`0x1180000001000`
- U-Boot bus 编号映射为 bus 0；
- EEPROM slave：`0x57`
- FDT compatible：`atmel,24c256`
- page size：64 bytes；
- 注释同时明确写着“Answers 1-byte offsets only”。

因此 `24c256` 只能作为 FDT 的设备描述，不能作为本项目已经确认的物理 BOM 证据。

`u-boot/include/configs/octeon_pa220.h` 又明确：

- `CONFIG_SYS_I2C_EEPROM_ADDR = 0x57`
- `CONFIG_SYS_I2C_EEPROM_ADDR_LEN = 1`
- page write bits = 6，即配置 page size 64 bytes。

**物理 EEPROM 型号/物理容量仍为 UNKNOWN。**

### 12.2 但“当前 U-Boot 实际能访问多少”已经可以源码确定

`u-boot/common/cmd_eeprom.c:eeprom_read()` 在 `CONFIG_SYS_I2C_EEPROM_ADDR_LEN == 1` 时构造：

- `addr[0] = offset >> 8`
- `addr[1] = offset & 0xff`
- 随后调用 `i2c_read(addr[0] | dev_addr, offset, 1, ...)`

也就是说这里的“1-byte address length”实际语义是：

**I2C slave address 的 bank selector + 1-byte EEPROM internal offset。**

随后 `u-boot/drivers/i2c/octeon_i2c.c` 的 OCTEON I2C 驱动只使用目标地址的低 7 bit 发出 slave address。因此在固定 `dev_addr = 0x57` 的情况下，offset 的高位不能产生一个新的可用 slave address；内部 EEPROM offset 最终仍只有低 8 bit。

所以当前这套 U-Boot EEPROM 访问路径的**有效软件访问窗口为 256 bytes，并会以 0x100 为周期重复访问同一低 8-bit 地址窗口**。

这不是“物理 EEPROM 容量 = 256 bytes”的证明；它只证明：

> 即使板上实际器件大于 256 bytes，当前这套 U-Boot 配置/驱动也无法通过现有 `eeprom_read()` 正常访问其 0x100 以上的独立存储空间。

因此此前“实际容量 UNKNOWN”仍然成立，但现在增加一个更强的源码结论：

- **physical capacity：UNKNOWN**
- **current U-Boot effective readable window：256 bytes**
- **software cannot use current path to prove/read physical capacity >256 bytes**

### 12.3 TLV parser 的边界行为

`arch/mips/cpu/octeon/lib_octeon_shared.c`：

- header 大小为 8 bytes；
- `EEPROM_END_TYPE (0xffff)` 直接结束扫描；
- tuple length 必须满足 `sizeof(header) <= length <= 256`；
- `buf_len` 必须不小于 tuple length；
- checksum 会验证 header + payload；
- `octeon_tlv_get_tuple_addr()` 从 offset 0（除非定义了 `CONFIG_EEPROM_TLV_BASE_ADDRESS`）连续累加 tuple length。

但 parser **没有 EEPROM physical capacity 参数，也没有独立的总扫描上限**。

此外，在 U-Boot 路径中，读取 tuple payload 的第二次 `eeprom_read()` 调用没有检查其返回值；因此底层读取失败后仍可能继续对目标缓冲区做 checksum 计算。

结论：

**parser 对 tuple 自身长度有边界检查，但不能保证 tuple 扫描不会因为错误 END/损坏数据而超出真实 EEPROM；真实容量边界必须由硬件/运行时验证。**

这不阻塞当前 P1 构建，因为我们没有必要为了“证明物理容量”修改构建逻辑；最终 Web/CLI 只读展示可以把物理容量标为 UNKNOWN，并在实机测试阶段补齐证据。

### 12.4 TLV 实机只读验证：本项目后续测试必须执行

以下测试全部只读，不执行任何 EEPROM 写命令：

1. `i2c dev 0`
2. `i2c probe 57`
3. 确认 `0x57` 存在；
4. 在已经确认的 RAM load 区内读取 0x100 bytes：
   `eeprom read <safe_ram_addr> 0 100`
5. `md.b <safe_ram_addr> 100`，记录原始 TLV；
6. 再读取 offset `0x100`：
   `eeprom read <safe_ram_addr> 100 100`
7. 再读取 offset `0x200`：
   `eeprom read <safe_ram_addr> 200 100`
8. 比较三个窗口是否发生 0x100 周期重复；
9. 记录 TLV END tuple、board descriptor、MAC tuple、clock tuple 的实际位置；
10. 将上述输出完整反馈给本项目，以便最终判断：
   - 实际有效 TLV 区域；
   - 当前 U-Boot 访问窗口是否足够；
   - 是否需要以后增加正确的 16-bit EEPROM addressing 支持。

如果后续 Linux/OpenWrt 已经启动并提供 i2c-tools，则可以再做纯只读的 I2C 总线/EEPROM 检查；但不要为了 P1 阻塞构建去追这个问题。

**禁止：`eeprom write`、`tlv_eeprom set`、`tlv_eeprom delete`、任何 I2C EEPROM 写操作。**

## 13. 第二轮源码收敛：RAM load-memory

### 13.1 默认 PA-220 大内存情况下的 load block 已可进一步精确

`octeon_configure_load_memory()` 的源码公式为：

- 自动 size：OCTEON II/III、RAM > 256 MiB 时：
  `min(256 MiB, (ram_size - 256 MiB) / 3 向下按 1 MiB 对齐)`
- 对 PA-220 默认 8 GiB RAM，这个值最终为 **256 MiB**；
- 自动 base 计算经过 boot-bus hole 修正后最终落在 **0x20000000**；
- 随后：
  - `loadaddr = 0x20000000`
  - `load_reserved_addr = 0x20000000`
  - `load_reserved_size = 0x10000000`

即默认 8 GiB 配置下的候选 load block 为：

`[0x20000000, 0x30000000)`

这是**物理地址区间**。

### 13.2 Linux reserved 区与 load block 不重叠

`octeon_configure_linux_reserved_memory()` 在默认自动模式下：

- RAM >= 1 GiB：`size = min(ram_size / 2, 240 MiB)`
- 默认 base = `OCTEON_LINUX_RESERVED_MEM`
- PA-220 固定低地址布局下，该 base 为约 1 MiB。

因此默认 8 GiB 情况下：

- Linux reserved：约 `0x00100000 .. 0x0F100000`
- load block：`0x20000000 .. 0x30000000`

两者没有重叠。

### 13.3 U-Boot relocation 区也与 load block 分离

`board_init_f_finish()` 的实际公式：

1. CN63XX/CN70XX 及更新型号：
   `u_boot_mem_top = 0x20000000 + (gd->ram_size - 0x10000000)`
2. U-Boot 长度：
   `len = ALIGN_UP(&uboot_end - CONFIG_SYS_MONITOR_BASE, 64 KiB)`
3. 向下扣除：
   - U-Boot code/data/bss；
   - `STACK_SIZE = 64 KiB + CONFIG_SYS_BOOTPARAMS_LEN`；
   - `TOTAL_MALLOC_LEN`；
4. 再根据 TLB mapping size 向下对齐；
5. 得到 `u_boot_base_phys`；
6. 最终：
   `bd->bi_uboot_ram_addr = u_boot_base_phys`
   `bd->bi_uboot_ram_used_size = u_boot_mem_top - u_boot_base_phys`。

因此最终 relocation 地址不能在源码审计阶段写死；但对 PA-220 默认 8 GiB 情况，可以确认它位于 DRAM 映射的高端区域，而不是 0x20000000..0x30000000 的 load block。

### 13.4 0x81000000 的结论进一步明确

`CONFIG_SYS_SDRAM_BASE = 0x80000000` 是 cached address。

因此：

`0x81000000` 对应低端 DRAM physical address **0x01000000 (16 MiB)**。

该地址落在默认 Linux reserved 区内部，因此：

**0x81000000 = UNSAFE 作为默认 Web/TFTP 上传地址。**

这不是因为它“看起来危险”，而是可以由 OCTEON MIPS cached alias + Linux reserved memory 公式直接推出。

### 13.5 load block 是当前最合理的候选安全区

当前源码证据已经足以把结论从“候选”提高到：

**CONDITIONALLY SAFE / 首选运行时上传区**

条件是：

1. `octeon_configure_load_memory()` 成功；
2. `load_reserved_addr/load_reserved_size` 非零；
3. 上传地址从运行时 `loadaddr` / reserved block 获取；
4. 上传长度不超过 `load_reserved_size`；
5. 不把 `CONFIG_SYS_LOAD_ADDR` 单独当作安全性证明；
6. 不在 P1/P6 中重新使用固定 `0x81000000`。

当前 TFTP 接收路径本身仍然是直接：

`memcpy(load_addr + offset, src, len)`

没有在接收每个 block 前做 reserved-range 硬限制。

源码另有 `octeon_check_mem_load_range()`，它只能依据 `fileaddr/filesize` **事后发出 warning**，不能阻止越界写入。

因此最终 Web/TFTP 安全实现必须在写 RAM **之前**检查：

`addr >= load_reserved_addr`

且：

`addr + size <= load_reserved_addr + load_reserved_size`

并处理整数溢出。

### 13.6 P1 不阻塞构建的运行时验证项目

上述 RAM 映射已经足够指导当前构建，不需要为了 P1 再增加“猜测型”补丁。

**构建完成、RAM 启动后请务必反馈以下结果：**

1. 完整启动日志中：
   - `Base DRAM address used by u-boot:`
   - `DRAM:`
   - `Stack Pointer at:`
   - `Using TLB mapping size:`（若 DEBUG 日志可见）
   - `load address / size` 相关输出；
2. `printenv loadaddr octeon_reserved_mem_load_size octeon_reserved_mem_load_base`
3. `bdinfo`
4. 若可用，读取/显示 bootmem named block 信息；
5. 最重要的是记录最终：
   - `loadaddr`
   - `load_reserved_addr`
   - `load_reserved_size`
   - `gd->ram_size`
   - `bd->bi_uboot_ram_addr`
   - `bd->bi_uboot_ram_used_size`

这些结果回来后即可把 RAM 状态从 **CONDITIONALLY SAFE** 收敛为实机确认值，而无需修改当前构建。

## 14. 当前阶段结论更新

本轮没有修改 PA-220 固定源码，也没有执行任何 EEPROM/flash 持久化写入。

当前 P1-A5 的剩余项目应分成两类：

### 不阻塞构建、留待实机验证

- EEPROM physical model/capacity：**UNKNOWN**
- EEPROM 有效 TLV 终止位置：**待实机读取**
- EEPROM 当前 U-Boot 访问窗口：**源码已确定为 256 bytes**
- RAM 最终 relocation 数值：**需最终构建日志**
- load block 实际运行时地址/大小：**需启动后确认**

### 已足够指导后续编码

- EEPROM 读取地址：**bus 0 / slave 0x57**
- 当前 EEPROM addressing scheme：**bank-selector + 8-bit internal offset**
- 不应假定物理 EEPROM = 24C256；
- 不应在 Web 中提供 EEPROM 写接口；
- 默认 8 GiB PA-220 的 load block：**physical 0x20000000..0x30000000**
- 0x81000000：**UNSAFE**
- Web/TFTP 后续应使用运行时 reserved load block；
- TFTP 越界检查必须在 RAM 写入之前完成，而不能依赖事后 warning。

**因此本轮没有必要为了 EEPROM 物理容量或最终 relocation 数值阻塞 U-Boot 构建。后续构建完成后，按本节测试清单收集结果即可。**


## 15. RAM 第二轮核对：链接布局、重定位映射与地址语义

本轮继续核对 PA-220 固定源码 commit，不修改 PA-220 源码。

### 15.1 链接地址与 ELF 布局

证据文件：

- `board/octeon/pa220/config.mk`
- `arch/mips/cpu/octeon/u-boot.lds`
- `include/configs/octeon_common.h`

确认：

- PA-220 `CONFIG_SYS_TEXT_BASE = 0xC0000000`；
- linker script 的入口为 `_start`；
- `.text` 从 `CONFIG_SYS_TEXT_BASE + SIZEOF_HEADERS` 开始；
- 随后依次组织 GOT、rodata、data、sdata、`.u_boot_list`、sbss、bss、scommon、cvmx_shared_bss；
- `uboot_end` 标记拷贝范围结束；
- 镜像长度由最终链接产物的 `uboot_end - CONFIG_SYS_MONITOR_BASE` 决定，并在 relocation 时向上按 64 KiB 对齐。

注意：`0xC0000000` 是链接地址布局的起点，不能直接当成 DRAM physical address，也不能拿它与 named block 的物理地址区间直接比较。

### 15.2 relocation 区间的源码公式

`arch/mips/lib/board_octeon.c:board_init_f_finish()` 明确给出：

1. 对 CN63XX 及更新 OCTEON：
   `u_boot_mem_top = 0x20000000 + (gd->ram_size - 0x10000000)`（RAM > 256 MiB 分支）。
2. `len = ALIGN_UP(uboot_end - CONFIG_SYS_MONITOR_BASE, 64 KiB)`。
3. 代码/data/BSS 预留 `MAX(len, 512 KiB)`。
4. 再扣除 `STACK_SIZE = 64 KiB + CONFIG_SYS_BOOTPARAMS_LEN` 和 `TOTAL_MALLOC_LEN`。
5. 根据 `u_boot_mem_top - addr` 计算 TLB mapping alignment，向下对齐得到 `u_boot_base_phys`。
6. U-Boot relocation 的物理占用区间为：
   `[u_boot_base_phys, u_boot_mem_top)`。
7. heap 起点为 `u_boot_base_phys + len`；malloc arena 大小为 `TOTAL_MALLOC_LEN`。
8. stack/board data/global data/bootparams 由该函数继续在上述预留区中分配。
9. 最终写入：
   - `bd->bi_uboot_ram_addr = u_boot_base_phys`
   - `bd->bi_uboot_ram_used_size = u_boot_mem_top - u_boot_base_phys`

这证明 relocation 物理占用范围不是简单的“镜像大小”，而是包含 U-Boot 运行所需 heap、stack、bd、gd 和 bootparams 的整体保留窗口。

### 15.3 虚拟地址映射

同一函数后段调用：

- `addrmap_set_entry(CONFIG_SYS_TEXT_BASE, bd->bi_uboot_ram_addr, bd->bi_uboot_map_size * 2, CONFIG_ADDR_MAP_CODE_IDX)`
- 将 U-Boot link virtual base 映射到动态 relocation physical base；
- `addrmap_set_entry(0, 0, min(gd->ram_size, 256 MiB), CONFIG_ADDR_MAP_FIRST_256MB_IDX)`；
- boot bus 另有独立映射。

所以运行时 U-Boot 代码地址 `0xC0000000` 经 addrmap 映射到动态 relocation physical address；不能用简单的 KSEG0 低 29-bit 截断来推算这个高地址映射。

`cvmx-address.h` 对 XKPHYS 地址的结构定义为：最高两位 address region = XKPHYS，后续 CCA 位与 physical address 字段分离；该头文件明确把 XKPHYS 解释为通过 unmapped virtual address 访问 physical address。bootmem named block API 的分配器返回 physical base；指针型 API 再通过 `cvmx_phys_to_ptr()` 转成可访问指针。

### 15.4 低地址/异常/FDT 等区域与最终安全上传区

`octeon_mem_map.h` 明确写明 U-Boot 始终保留 DRAM 前 1 MiB；另有 idle-core code 与 bootmem descriptor 等固定保留区域。FDT 在早期初始化时由链接内嵌的 `__dtb_begin` 经 `MAKE_KSEG0()` 建立指针，后续另有 `__fdt` named block。异常 handler 也有独立 named block 名称 `__tmp_uboot_exception`。

这意味着最终上传区不能仅凭“RAM 中没有看到 U-Boot 镜像”来判定空闲，必须以运行时 named block 的 base/size 为准，并且上传区边界校验要在网络接收数据写入内存之前执行。

### 15.5 本轮结论与仍需运行时确认的值

源码已确认：

- link text base：`0xC0000000`；
- linker 的完整段组织及 `uboot_end` 终点；
- relocation 物理区间计算公式；
- heap、stack、bd、gd、bootparams 的保留关系；
- U-Boot virtual-to-physical addrmap 的建立方式；
- bootmem named block 的地址语义是 physical base/size，指针 API 才转换成可访问指针。

仍不应静态写死：

- 最终 ELF 的 `uboot_end` / `len`；
- 实际 `TOTAL_MALLOC_LEN`；
- 最终 TLB map size；
- 最终 `u_boot_base_phys`；
- 最终 stack/gd/bd/bootparams 数值；
- 最终运行时 `__tmp_load` named block 的地址与大小。

以上值均可在构建后通过 map/ELF 符号、启动 DEBUG 日志及 U-Boot 运行时查询确认，不阻塞当前阶段构建。后续实机测试仍按第 13.6 节清单执行，并补充提供最终 ELF/map 中的 `uboot_end` 与 `CONFIG_SYS_TEXT_BASE`。

**安全结论保持：0x81000000 不作为默认上传地址；上传 buffer 使用运行时 load-reserved named block，并在写入前做完整范围与溢出检查。**


## 16. load-memory 算法与地址映射最终交叉核对

本轮找到了此前需要进一步确认的实际实现：`arch/mips/lib/board_octeon.c:__octeon_configure_load_memory()`（约 620–735 行），并与 `lib/addr_map.c` 交叉核对。

### 16.1 默认大小算法

对于非 OCTEON1PLUS（PA-220 的 OCTEON II/III 路径）：

- 若 RAM <= 256 MiB：`min(128 MiB, align_down((RAM * 2 / 5), 1 MiB))`
- 若 RAM > 256 MiB：`min(256 MiB, align_down(((RAM - 256 MiB) / 3), 1 MiB)`

默认 PA-220 8 GiB RAM 落入第二分支，计算结果超过 256 MiB，因此最终 size = **256 MiB**。

若环境变量 `octeon_reserved_mem_load_size` 存在且不是 `auto`，则直接按十六进制解析覆盖自动 size。size 为 0 时不分配 named block，并输出 no reserved memory warning。

### 16.2 默认 base 算法

默认 `octeon_reserved_mem_load_base` 为 auto 时，代码先按 DRAM 低 256/512 MiB 区域、之前分配预留空间和 boot-bus hole 计算 mem_top，再得到 addr。

对于 8 GiB RAM / 256 MiB load size：

- mem_top 经 OCTEON II+ boot-bus hole 修正后计算；
- addr 最初可能落在 512 MiB 以上；
- 随后代码将 addr 限制为不高于 512 MiB；
- 最终 base = **0x20000000**。

所以默认 load block 是：

`physical base 0x20000000 + size 0x10000000 = [0x20000000, 0x30000000)`

### 16.3 named block 与 loadaddr 的关系

分配使用：

`cvmx_bootmem_phy_named_block_alloc(size, addr, addr + size, 0, "__tmp_load", 0)`

成功后：

- `setenv("loadaddr", "0x20000000")`
- `load_reserved_addr = addr`
- `load_reserved_size = size`

代码注释明确指出：若 reserved block 创建成功，`loadaddr` 每次启动都会被覆盖为 reserved block base。

若 named block 分配失败，会打印 ERROR；该分支没有设置 loadaddr/load_reserved globals。因此 Web/TFTP 不能只检查 loadaddr 是否存在，必须同时检查 load-reserved base/size 是否有效，并确认 named block 分配成功。

### 16.4 地址空间交叉核对

`cvmx-address.h` 的物理地址 decode 表明确：

- DRAM cached：`0x00000000 .. 0x0fffffff`
- Boot Bus uncached：`0x10000000 .. 0x1fffffff`
- DRAM cached：`0x20000000 ..`

`board_octeon.c` 后续为第一段 256 MiB 建立 1:1 addrmap，并为 256 MiB 以上的 DRAM 区域建立从虚拟 `0x20000000` 到 physical `0x20000000` 的映射；`lib/addr_map.c` 的转换公式为：

`physical = virtual - entry.vaddr + entry.paddr`

因此 PA-220 默认 loadaddr `0x20000000` 对应本段 DRAM 的 identity mapping，和 named block allocator 的 physical base 一致。该结论来自地址 decode + addrmap 实现，不是仅从变量名推测。

### 16.5 当前编码指导

后续上传实现应当：

1. 在 load-memory 配置成功后取得 `load_reserved_addr` / `load_reserved_size`；
2. 确认 `__tmp_load` named block 存在且 base/size 一致；
3. 上传目标地址必须位于该物理/addrmap identity 区间；
4. 对 `addr + len` 做溢出安全检查；
5. 只有完整满足 `addr >= base` 且 `len <= size - (addr - base)` 才允许写入；
6. TFTP 接收回调必须在 `memcpy` 前执行检查，不能依赖 `octeon_check_mem_load_range()` 的事后 warning。

以上默认 8 GiB 结果可用于当前设计；最终仍以实机 `gd->ram_size`、named block 查询结果和 loadaddr 为准。


## 17. TFTP 写入前安全边界的源码确认

继续核对 `net/tftp.c` 与 `arch/mips/cpu/octeon/octeon_boot.c`：

- `store_block()` 计算 `offset = block * TftpBlkSize + TftpBlockWrapOffset`；
- 普通 RAM 接收分支直接执行 `memcpy((void *)(load_addr + offset), src, len)`；
- 该写入前没有检查 `load_reserved_addr/load_reserved_size`；
- `octeon_check_mem_load_range()` 读取 `fileaddr/filesize`，在加载完成后才检查并打印 warning；
- 该检查没有阻止先前的 memcpy，也没有完整处理加法溢出；
- 现有 TFTP 接收路径因此不能被视为具有 reserved-memory 越界保护。

**后续实现要求进一步明确：**不能只在 TFTP 完成后调用现有 `octeon_check_mem_load_range()`。必须在每个接收 block 写 RAM 之前验证目的地址和长度，并对 block offset、`load_addr + offset`、`addr + len` 做溢出安全检查；越界时拒绝该 block/终止传输，绝不能先 memcpy 再告警。


## 18. load-reserved named block 生命周期与运行时查询接口

本轮继续核对 PA-220 固定源码 commit `801717ba6a8a59ed2cfd53bf3e1b19841fecd2e8`，重点确认 `__tmp_load` 是否会在 U-Boot 生命周期内被释放，以及 Web/TFTP 应使用什么接口确认其仍然有效。

### 18.1 named block 的真实名称和类型

证据文件：

- `arch/mips/include/asm/arch-octeon/octeon_mem_map.h`
- `arch/mips/lib/board_octeon.c`
- `arch/mips/cpu/octeon/octeon_boot.c`

源码定义：

- `OCTEON_BOOTLOADER_LOAD_MEM_NAME = "__tmp_load"`
- `OCTEON_BOOTLOADER_NAMED_BLOCK_TMP_PREFIX = "__tmp"`
- `OCTEON_LINUX_RESERVED_MEM_NAME = "__tmp_reserved_linux"`

`__tmp_load` 是临时 named block，不是永久保留的普通 DRAM 区。它的设计注释明确说明：供 U-Boot 加载 ELF 等镜像使用，在应用/OS 启动前释放。

### 18.2 创建时机

`octeon_boot.c` 在 bootmem allocator/descriptor 初始化后，完成 idle-core named block 分配，再调用 `octeon_configure_load_memory()`。该函数由 `board_octeon.c:__octeon_configure_load_memory()` 实现，并以 `OCTEON_BOOTLOADER_LOAD_MEM_NAME` 创建 named block。

分配成功后设置：

- 环境变量 `loadaddr`；
- 全局变量 `load_reserved_addr`；
- 全局变量 `load_reserved_size`。

该分配不是每次 TFTP/Web 请求重新执行；正常 U-Boot CLI 生命周期内，named block 由 bootmem allocator 持续登记。分配失败时不会建立有效的 reserved globals，故不能只看环境变量 `loadaddr`。

### 18.3 释放时机与例外

`common/cmd_bootm.c` 在调用实际 OS boot function 前调用 `arch_preboot_os()`。PA-220 的 `arch_preboot_os()` 实现位于 `arch/mips/cpu/octeon/octeon_boot.c`，进入 `octeon_bootloader_shutdown()`。

该 shutdown 路径：

1. 执行板级 OS handoff hook；
2. 关闭 watchdog；
3. 在 `api_enable` 环境变量不存在时，关闭网络/USB 等相关设备；
4. 调用 `octeon_free_tmp_named_blocks()`；
5. 最后重新保留低 1 MiB。

`octeon_free_tmp_named_blocks()` 遍历 bootmem named-block descriptor，对所有名称以 `"__tmp"` 开头且 size 非零的 block 调用 `cvmx_bootmem_phy_named_block_free()`。因此至少包括 `__tmp_load` 和 `__tmp_reserved_linux`，不是只释放单个 load block。

**例外：**若 `api_enable` 环境变量存在，shutdown 会跳过 `octeon_free_tmp_named_blocks()`。不能假设所有 handoff 都会释放这些 block。

释放函数没有清零对应 DRAM 内容；它释放的是 bootmem allocator 的占用登记。释放后不能再把旧地址当作仍被保留、可供 Web/TFTP 写入的安全区。

另一个需要避免的状态是：释放函数没有同步清零 `load_reserved_addr/load_reserved_size` 两个全局变量。bootm 的 OS handoff 路径在进入 OS 前调用 `arch_preboot_os()`；在会返回的 fake-go/调试路径中，若执行后继续回到 U-Boot，旧 globals 可能仍保留数值，但 named block 已经释放。后续实现不能仅凭 globals 非零就判定 block 仍有效，必须重新查询 named block descriptor。

### 18.4 可用的只读 named-block 查询 API

`cvmx-bootmem.h` 声明：

- `cvmx_bootmem_find_named_block(const char *name)`
- 返回 `const cvmx_bootmem_named_block_desc_t *`；未找到返回 NULL；
- descriptor 中有 `base_addr`、`size`、`name`。

另有 `cvmx_bootmem_phy_named_block_find(name, flags)`，返回 named-block descriptor 的物理地址或 0。

**后续首选接口：**通过 `cvmx_bootmem_find_named_block(OCTEON_BOOTLOADER_LOAD_MEM_NAME)` 读取 descriptor，并校验：

1. descriptor 存在；
2. `base_addr`、`size` 非零；
3. descriptor 的 `base_addr/size` 与 `load_reserved_addr/load_reserved_size` 一致；
4. 当前服务状态仍处于 U-Boot 可用阶段，未进入 OS handoff 清理阶段。

若旧版构建配置无法直接使用该指针 API，再评估 `cvmx_bootmem_phy_named_block_find()` 或遍历 descriptor array 的兼容实现。环境变量 `loadaddr` 只作为默认目的地址提示，不作为安全边界的权威数据源。

### 18.5 生命周期结论

- 正常 U-Boot CLI/TFTP/Web 阶段：`__tmp_load` 由 bootmem named block 管理，可作为**条件安全**上传区。
- OS handoff：默认会释放所有 `__tmp*` block；`api_enable` 存在时跳过释放。
- handoff 后或 fake-go 返回后：必须重新查询 named block，不能只相信仍未清零的 globals。
- 不应在 Web/TFTP 服务运行期间主动释放或重新分配 `__tmp_load`。
- 该生命周期结论来自固定源码，不需要实机验证；实际 descriptor base/size 仍留待构建后只读确认。

## 19. TFTP 与 Web 上传的写入/缓存路径对照

### 19.1 PA-220 原生 TFTP RAM 写入

证据文件：`net/tftp.c:store_block()`。

当前普通 RAM 分支：

- 计算 `offset = block * TftpBlkSize + TftpBlockWrapOffset`；
- 计算 `newsize = offset + len`；
- 直接执行 `memcpy((void *)(load_addr + offset), src, len)`；
- 更新 `NetBootFileXferSize`。

在该函数中没有看到：

- 对 `load_reserved_addr/load_reserved_size` 的 pre-write 范围校验；
- 对 block 乘法、offset 加法、目的地址加法的溢出保护；
- `flush_cache()` / `flush_dcache_range()` 等显式 cache maintenance。

若启用了 `CONFIG_SYS_DIRECT_FLASH_TFTP`，还存在直接 flash 写分支；RAM-only 维护服务不得把该分支误当作受 reserved RAM 限制的路径，必须确保配置/调用方式不会让上传请求落入 flash 写分支。

因此既有 TFTP 代码仍不能直接用于安全上传：每个数据 block 写入前必须先完成防溢出计算和完整 reserved-range 校验，拒绝越界 block 后立即终止传输。

### 19.2 uBootKit HTTP 上传路径（仅作为待适配行为参考）

固定 uBootKit 源码：`chenxin527/uboot-qsdk12.5-build@9ce315735ac88fd0a7c7697bc527a8616ed0a972`。

`net/httpd.c` 当前的 `httpd_get_upload_buffer_ptr()`：

- RAM >= 512 MiB 时选择 `CONFIG_SYS_SDRAM_BASE + 256 MiB`；
- 否则选择 IPQ 专用 `IPQ_TFTP_MIN_ADDR`；
- 调用 IPQ `is_memory_region_available()` 判断；
- 返回固定地址作为上传 buffer。

HTTP 接收过程由 TCP callback 将收到的 payload 通过 CPU `memcpy` 分段复制到 `upload_ptr + upload_size`。上传完成后调用 `flush_cache(upload_ptr, payload_size)`，然后把完整 payload 交给 URI handler。

适配 PA-220 时：

1. 必须移除 IPQ 固定地址和 IPQ memory-availability API；
2. 用 PA-220 `__tmp_load` named block descriptor 提供上传 buffer；
3. 在第一次 payload memcpy 前，按整个 HTTP payload 长度预留并验证范围；
4. 每个后续 TCP chunk 仍需验证当前 offset/len 不超过预先确认的总长度；
5. 上传完成后执行 PA-220 适用的 cache maintenance，再将 buffer 交给镜像验证/解析逻辑；
6. 不能让 URI handler 绕过上述检查直接持有任意 RAM 指针。

### 19.3 Cache coherency 当前结论

- HTTP 参考实现显式调用 `flush_cache()`，表明上传结束后有意将 CPU 写入的数据做 cache maintenance。
- PA-220 原生 TFTP `store_block()` 没有在该函数中执行 cache flush。
- PA-220 MIPS `do_bootm_linux()` 的准备/跳转代码本身也没有显示出通用 `flush_cache()` 调用；`octeon_bootloader_shutdown()` 的已核查路径同样没有明确的 upload-buffer flush。
- 当前证据不足以断言整个启动链绝对没有其他 cache 维护机制；但不能把 TFTP 的 CPU memcpy 等同于“已完成供 OS/其他执行单元读取的 cache 同步”。

**后续实现要求：**HTTP/TFTP 接收结束后必须有明确、可审查的 PA-220 cache maintenance 方案；同时确认 bootm/bootelf/OS handoff 对已加载镜像的缓存处理。该项属于后续实现必须闭环的源码工作，不需要为了本轮 P1 阶段继续猜测硬件 cache 状态，也不阻塞当前阶段性构建。

## 20. 本轮结论与后续待办

本轮已完成源码确认：

- `__tmp_load` 的真实名称及 `"__tmp"` 临时块前缀；
- 创建时机与 U-Boot 正常运行期间的生命周期；
- OS handoff 的释放调用链及 `api_enable` 例外；
- named-block descriptor 查询 API 的声明、实现和基础 CVMX 编译入口；
- 查询 API 返回 static descriptor、非 re-entrant，调用方必须立即复制字段；
- globals 在释放后未清零的生命周期注意事项；
- PA-220 TFTP 当前缺少 pre-write range validation 和显式 cache flush；
- uBootKit HTTP 的 IPQ 固定地址不可移植，但“分段 CPU memcpy + 完成后 cache flush”的流程可作为设计参考；
- bootm OS handoff 与 bootelf 应用执行路径对临时 named block 的不同处理。

下一轮继续核对的重点：

1. 追踪 OCTEON `load_elf64_image()` 的实际实现/来源、ELF segment 目标地址处理，以及目标入口前的 D/I-cache 同步；
2. 进一步明确 PA-220 MIPS cache maintenance API 的具体语义，闭环 TFTP 写入后的 cache flush 与 bootm/bootelf 消费路径；
3. 将 range validator 设计为 HTTP/TFTP 共用、写入前调用的单一入口，并覆盖 ELF segment 目的地址；
4. 在上述静态证据闭环后，再完成 P1-A5 的最终板级初始化时序、功能矩阵、ROADMAP 和 P1 总结。

以上源码结论不修改 PA-220 固定源码；本轮没有执行任何 EEPROM、SPI、eMMC 或其他持久化写入。


## 21. bootm 与 bootelf 对临时上传区的不同处理

继续核对 `common/cmd_bootm.c`、`arch/mips/lib/bootm.c`、`common/cmd_elf.c` 和 `arch/mips/cpu/octeon/commands/cmd_elf64.c`：

- `bootm_start()` 的 OS 执行路径会在调用实际 OS boot function 前调用 `arch_preboot_os()`，因此会进入 PA-220 的 `octeon_bootloader_shutdown()`，并按上一节规则释放 `__tmp*` 临时 named blocks（除非 `api_enable` 存在）。
- `bootelf` 命令的 `do_bootelf()` 则直接验证/装载 ELF 并调用 `do_bootelf_exec()`，没有调用 `arch_preboot_os()`。应用返回 U-Boot 后，`__tmp_load` 不会因为这条命令本身而被释放。
- OCTEON 的 64-bit `cmd_elf64.c` 覆盖实现中，`do_bootelf_exec()` 直接调用 entry，没有在该函数内执行 `flush_cache()` 或关闭 D-cache；不能将通用 `cmd_elf.c` 的 QNX 注释当作 OCTEON 64-bit 路径的缓存保证。
- 当前没有完成 `load_elf64_image()` 内部各 segment copy、D/I-cache 同步和目标入口跳转前 cache maintenance 的完整追踪。

**实现含义：**

1. 上传区生命周期不能简单等同于“执行任何 boot 命令就释放”；bootm OS handoff 与 bootelf 应分别处理。
2. bootelf 读取 ELF header/program header 并将 segment 装载到目标地址时，必须验证每个 segment 的目标范围；仅验证上传 ELF 文件本身位于 `__tmp_load` 并不代表其声明的 segment 地址安全。
3. Web/TFTP 上传缓冲区 cache maintenance 与 ELF segment 目的地址 cache maintenance 是两个不同问题，不能只 flush 原始上传 buffer 就认为整个 bootelf 路径已解决。
4. 该项留作后续 bootelf/ELF loader 专项核查，不影响本轮当前阶段性构建；不得在未核查前宣称 bootelf 缓存一致性已闭环。


## 22. named-block 查询 API 的实现与可编译性复核

本轮继续读取 PA-220 固定源码的 `arch/mips/cpu/octeon/cvmx/cvmx-bootmem.c` 与 `arch/mips/cpu/octeon/cvmx/Makefile`：

- `cvmx_bootmem_find_named_block()` 并非只有头文件声明；其实现调用 `__cvmx_bootmem_find_named_block_flags(name, 0)`，从 bootmem named-block table 查找并填充 descriptor。
- `cvmx_bootmem_named_block_desc_t` 包含 `base_addr`、`size` 和 `name`。
- `__cvmx_bootmem_find_named_block_flags()` 返回的是**函数内部 static descriptor** 的地址，不是 bootmem descriptor array 中原始条目的直接指针；注释明确该 API 非 re-entrant。
- `arch/mips/cpu/octeon/cvmx/Makefile` 在 `CONFIG_OCTEON` 下编入 `cvmx-bootmem.o`，所以该函数属于 OCTEON 基础 CVMX 对象，不依赖可选 QLM/networking 子配置。

**实现注意：**调用 `cvmx_bootmem_find_named_block()` 后，应立即把 `base_addr`、`size` 复制到调用方自己的局部结构/变量中，不要长期保存返回指针，也不要在仍依赖该 static descriptor 内容时嵌套调用同一 API。

这进一步确认了该接口适合作为 PA-220 上传区运行时查询的首选接口；具体目标配置的最终链接符号仍可在构建 map/ELF 中复核，不需要为此阻塞当前阶段。


## 23. OCTEON ELF64 装载器的真实实现与边界审计

固定 PA-220 源码：`IronDemiGod/pa220-openwrt@801717ba6a8a59ed2cfd53bf3e1b19841fecd2e8`。

### 23.1 实际实现和编译入口

- 实际实现：`u-boot/arch/mips/cpu/octeon/commands/cmd_elf64.c`。
- 命令入口：`do_bootelf()`。
- OCTEON ELF64 loader：`load_elf64_image(unsigned long addr, uint64_t load_override)`。
- 编译入口：`u-boot/arch/mips/cpu/octeon/Makefile` 中 `CONFIG_CMD_OCTEON_ELF` 对应 `commands/cmd_elf64.o`；`include/configs/octeon_cmd_conf.h` 定义该命令配置。
- 这不是通用 `common/cmd_elf.c` 的 ELF64 实现。通用文件不能替代本节的实际证据。

### 23.2 Header / Program Header 读取路径

`do_bootelf()` 只取得 ELF 起始地址（默认 `load_addr`，或命令参数），随后调用 `valid_elf_image(addr)`。该验证函数通过 `cvmx_phys_to_ptr(addr)` 读取 ELF header，只检查：

- ELF magic；
- ELFCLASS32 / ELFCLASS64；
- `e_type == ET_EXEC`；
- `e_machine == EM_MIPS`。

它没有接收镜像长度参数，也没有检查 header、program-header table 或 section-header table 是否落在实际上传文件范围内。

ELF64 loader 随后直接把 `addr` 转为 `Elf64_Ehdr *`，读取：

- `e_phoff`
- `e_phnum`
- `e_phentsize`
- `e_type`

当 `ET_EXEC && e_phoff && e_phnum` 时，按 `e_phnum` 遍历 program header。每个 header 地址由：

`(uint32_t)(addr + phoff + (i * phentsize))`

计算，再读取 `p_type/p_offset/p_filesz/p_memsz/p_paddr`。

### 23.3 已确认的整数、源文件和目标范围缺口

当前代码没有完整执行以下验证：

1. `e_phoff + e_phnum * e_phentsize` 的乘法/加法溢出检查；
2. program-header table 是否完全位于上传 ELF payload 内；
3. `e_phentsize` 是否与实际 `Elf64_Phdr` 布局兼容；
4. `addr + phoff + i * phentsize` 经 `uint32_t` 转换后的截断/回绕；
5. `p_offset + p_filesz` 是否落在上传 payload 内；
6. `p_filesz <= p_memsz`；
7. `p_paddr` 经 `octeon_fixup_xkphys()` 转换后的目标地址是否仍属于有效 `__tmp_load`；
8. `daddr + p_memsz`、`daddr + filesz` 是否溢出；
9. ELF entry 是否位于一个已验证的可执行 PT_LOAD segment 内。

源数据地址也经过 32-bit 转换：`image = (unsigned char *)(uint32_t)(offset + addr)`，随后调用 `memcpy64(daddr, (int32_t)image, filesz)`。不能因为 `memcpy64()` 能处理非对齐和尾数字节，就认为它会验证源/目标范围；`octeon_boot.c` 中的实现注释明确要求调用者保证两个 64-bit 地址有效。

### 23.4 Segment 写入与 little-endian 转换

PT_LOAD 的当前顺序是：

1. 由 `p_paddr` 或 `load_override` 得到目标地址；
2. OCTEON 下调用 `octeon_fixup_xkphys(paddr)`；
3. `memcpy64()` 复制 `p_filesz`；
4. 若 `p_memsz > p_filesz`，`memset64()` 清零剩余空间；
5. little-endian ELF 再以 8-byte `ld/dsbh/dshd/sd` 循环转换；
6. 调用 `flush_cache(daddr, memsz)`。

当前代码没有在第 2 步后检查目标 segment 是否位于 `__tmp_load`。此外 little-endian 转换循环条件为 `pos < memsz`、每轮固定递增 8；当 `memsz` 不是 8 的整数倍时，最后一次 8-byte 读写可能越过 segment 的逻辑末尾。P2 必须修正尾部处理或明确拒绝不符合约束的 segment，不能只把范围检查长度简单写成 `memsz` 而忽略实际 8-byte 访问。

### 23.5 Section-header fallback 也是写内存路径

若 ELF64 不满足 PT_LOAD 分支条件，当前实现会回退到 section-header 装载：

- 直接读取 `e_shoff/e_shstrndx/e_shnum`；
- 计算 section header 和 string table 地址；
- 对 SHF_ALLOC section 执行 `memset64()` 或 `memcpy64()`；
- 每个 section 后调用 `flush_cache()`。

该 fallback 同样没有上传文件长度边界、整数溢出和 `__tmp_load` 目标范围检查；并且部分字段直接使用 ELF 结构体原始值而非统一 endian accessor。后续必须对 fallback 做同等验证，或明确限制 ELF64 只接受经过验证的 PT_LOAD 路径，不能留下未保护的第二条写内存路径。

### 23.6 Entry 和执行阶段

PT_LOAD 路径返回 `octeon_fixup_xkphys(e_entry)`。OCTEON 的 `do_bootelf_exec()` 位于 `arch/mips/cpu/octeon/octeon_exec.c`，先调用 `octeon_restore_std_mips_config()`，随后直接调用 entry；该执行包装函数本身没有额外的 `flush_cache()`。

因此入口前 Cache 操作的实际位置是 ELF loader 的每个 segment 装载完成后，而不是 `do_bootelf_exec()`。同时 bootelf 不调用 `arch_preboot_os()`，不会走 bootm 的临时块释放链；应用返回 U-Boot 后仍须重新查询 named block，不能只相信未清零的 globals。

### 23.7 ELF64 结论

- 实际 loader 来源已定位并确认，不再是 UNKNOWN。
- 当前 ELF64 loader **没有足够的源文件长度和目标 RAM 范围验证**。
- `octeon_fixup_xkphys()` 负责地址格式规范化/清理 CCA 位，不是 RAM 安全范围验证器。
- PT_LOAD 与 section fallback 两条写入路径都必须纳入 P2 加固。
- 在加固完成前，不应把未经验证的 ELF 文件交给 `bootelf`；特别不能把“ELF 文件本身在 __tmp_load 内”当成 segment 目标安全的证明。

## 24. PA-220 Cache API 的具体语义与调用链闭环

固定 PA-220 源码中的实际实现位于 `u-boot/arch/mips/cpu/octeon/cpu.c`；底层宏位于 `u-boot/arch/mips/include/asm/arch-octeon/cvmx-asm.h`，定义由 `octeon_boot.h` 引入。

### 24.1 API 的真实操作

| API | 固定源码实际行为 | 不能误读为 |
|---|---|---|
| `flush_cache(start,size)` | `CVMX_SYNC`，随后 `CVMX_ICACHE_INVALIDATE` 和 `CVMX_DCACHE_INVALIDATE` | 指定范围逐 cache-line writeback |
| `flush_dcache_range(start,stop)` | 执行 `CVMX_SYNCW` | 逐行 D-cache writeback/invalidate |
| `invalidate_dcache_range(start,stop)` | 空实现 | 实际 D-cache invalidate |

关键宏定义：

- `CVMX_SYNC`：`sync` 指令并带 compiler memory clobber。
- `CVMX_SYNCW`：`syncw` 指令并带 compiler memory clobber。
- `CVMX_ICACHE_INVALIDATE`：执行同步后 `synci 0($0)`；源码注释描述为整个 I-cache 失效。
- `CVMX_DCACHE_INVALIDATE`：执行同步后 `cache 9, 0($0)`；源码注释描述为整个 D-cache 失效。

特别注意：`flush_cache()` 的 `start_addr/size` 参数在 PA-220 实现中没有参与运算，调用它会触发全局 I/D-cache 操作。不能根据 API 名称或其他 MIPS 平台实现推断它只影响传入区间。

### 24.2 TFTP CPU memcpy 后的状态

PA-220 `net/tftp.c:store_block()` 普通 RAM 分支执行 CPU `memcpy(load_addr + offset, src, len)`。该函数没有显式调用：

- `flush_cache()`
- `flush_dcache_range()`
- `CVMX_SYNCW`

所以现状是：**TFTP block 写入路径没有显式的完成同步操作**。不能仅凭 CPU memcpy 返回，就把数据已对后续消费者完成同步作为源码保证。

### 24.3 HTTP 上传完成后的状态

uBootKit 固定源码 `net/httpd.c` 在 HTTP payload 完成时调用 `flush_cache(upload_ptr, payload_size)`。这是可以参考的“上传完成后同步”流程，但它的 IPQ 固定地址/availability API 不能移植。

若使用 PA-220 的同名 API，该调用不是局部 payload flush，而是 PA-220 CPU 层的全局同步和 I/D-cache 失效。另外原参考实现把 `upload_ptr[payload_size] = 0` 放在 `flush_cache()` 之后；若保留这个 NUL 终止字节，RAM reservation 需要覆盖 `payload_size + 1`，或改用长度感知解析、不再写尾随字节。

### 24.4 bootm 的 Cache 路径

PA-220 `common/cmd_bootm.c` 的 OS image load/decompress 路径在完成目标地址写入后调用：

`flush_cache(load, (*load_end - load) * sizeof(ulong))`

由于 PA-220 的 `flush_cache()` 忽略范围参数，实际效果是全局同步并失效 I/D-cache。

随后 `boot_selected_os()` 调用 `arch_preboot_os()`，进入 OCTEON 的 `octeon_bootloader_shutdown()`；再由 `arch/mips/lib/bootm.c` 的 `boot_jump_linux()` 调用内核入口。已核对的 `arch_preboot_os()` / `boot_jump_linux()` 本身没有另一个 cache range 操作。

因此准确结论是：**bootm 有装载完成后的全局 Cache 操作，但不是由 OS handoff hook 执行。**

### 24.5 bootelf 的 Cache 路径

PT_LOAD 每个 segment 在 copy、zero-fill 和 little-endian 转换后调用 `flush_cache(daddr, memsz)`；section fallback 每个可装载 section 后也调用该函数。根据 24.1，实际都是全局同步并失效 I/D-cache。

`do_bootelf_exec()` 本身不再做 Cache 操作。当前 bootelf 的代码执行前同步来自 loader 的 segment/section 循环。

## 25. 统一 RAM 写入前范围验证接口设计

本节是 P2 实现约束，不代表代码已经实现或编译通过。

### 25.1 唯一权威边界

唯一可接受的 RAM 写入窗口来源：

`cvmx_bootmem_find_named_block(OCTEON_BOOTLOADER_LOAD_MEM_NAME)`

其中 `OCTEON_BOOTLOADER_LOAD_MEM_NAME` 为 `"__tmp_load"`。

每次查询必须立即把 static descriptor 的 `base_addr/size` 复制到调用方变量；不能保存返回指针。必须检查：

1. descriptor 存在；
2. `base_addr != 0` 且 `size != 0`；
3. `base_addr + size` 不溢出；
4. `load_reserved_addr/load_reserved_size` 与当前 descriptor 一致；
5. 当前仍处于 U-Boot 服务阶段，尚未进入释放临时块的 OS handoff。

`loadaddr` 只是默认写入地址提示，不是安全边界权威数据。禁止使用硬编码地址、`gd->ram_size` 推算的空闲区或旧 globals 单独作为安全证明。

### 25.2 统一接口建议

建议提供一个无缓存 descriptor 指针的公共检查入口，例如：

`int octeon_load_range_check(uint64_t addr, uint64_t len);`

该函数每次调用都重新查询 `__tmp_load`，立即复制 descriptor 字段，再验证当前请求。内部统一采用半开区间和减法式检查：

- 拒绝零长度写入；
- 拒绝 `addr < base`；
- `offset = addr - base`；
- 拒绝 `offset > size`；
- 拒绝 `len > size - offset`。

先检查 `size <= UINT64_MAX - base`。不得先计算可能溢出的 `addr + len` 再比较。需要形成 end 地址时，必须先通过 checked-add helper。

乘法统一采用：若 `a != 0` 且 `b > UINT64_MAX / a`，立即拒绝。所有 offset、地址和长度统一提升到 `uint64_t` 后再计算。

### 25.3 TFTP 接收 block

在 `store_block()` 的 RAM `memcpy` 之前：

1. checked-multiply 计算 block offset；
2. checked-add 合并 wrap offset；
3. checked-add 计算目的地址；
4. 调用统一范围检查；
5. 只有检查通过才能 memcpy；
6. 越界或溢出时拒绝当前 block 并终止/失败本次传输。

若协商得到 TFTP `tsize`，在接收首 block 前先对完整 payload 做 reservation 检查；若服务器没有提供总长度，则把从 load address 到 named block 末端的剩余空间作为本次传输的最大允许窗口，并继续逐 block 检查，任何 block 都不得突破该窗口。

### 25.4 HTTP payload/chunk

HTTP 在第一次 payload memcpy 前已知 `Content-Length`，必须先对完整 payload 做一次 reservation 检查。随后每个 TCP chunk 写入前：

- checked-add 计算 `upload_size + chunk_len`；
- 确认不超过 `payload_size`；
- checked-add 计算当前目的地址；
- 对当前 chunk 再调用统一范围检查；
- 检查通过后才能 memcpy。

若继续写 `upload_ptr[payload_size] = 0`，预留长度必须包含额外 1 byte；更稳妥的方案是去掉尾随 NUL，后续解析全部使用显式 payload length。适配版 HTTP 上传的所有路径（包括小于内部 header buffer 的短 payload）都必须遵守统一 reservation 规则，不能出现绕过 named block validator 的小包分支。

### 25.5 ELF source 和 segment destination

ELF loader 必须显式取得当前上传镜像的真实长度；不能只用地址，也不能信任可能陈旧的 `filesize` 环境变量。

写 segment 前至少验证：

- ELF header 完整位于上传 payload；
- `e_phoff`、`e_phnum * e_phentsize` 和 program-header table 全部无溢出且位于 payload；
- `e_phentsize` 与实际读取结构布局兼容；
- 每个 PT_LOAD 的 `p_offset/p_filesz` 完全位于 payload；
- `p_filesz <= p_memsz`；
- 目标地址经严格的 OCTEON 地址规范化后转换到与 named block 一致的 physical address domain；
- 每个 segment 的实际写入长度（包括 endian conversion 的实际访问宽度）在 `__tmp_load` 内；
- 所有 segment destination 不与仍需读取的 ELF source/header/program-header 数据重叠；
- segment 之间不存在未经支持的目标重叠；
- entry 位于一个已验证的可执行 PT_LOAD segment。

`octeon_fixup_xkphys()` 之后仍必须做物理地址归一化和 named-block 检查；不能把它当成 range validator。section-header fallback 若保留，必须做同等级验证，否则应明确拒绝该 fallback。

### 25.6 Cache 操作与范围验证分离

范围检查解决“能不能写”；Cache 同步解决“写入何时对后续消费者可见”，两者不可互相替代。

- TFTP/HTTP payload 完成后，优先调用 PA-220 `flush_dcache_range(start, stop)` 作为写入完成同步；该 API 实际执行 `syncw`，不是逐行 cache flush。调用前必须安全计算 stop。
- 不应为了每个网络 chunk 调用 PA-220 的 `flush_cache()`，因为它会全局失效 I/D-cache。
- ELF segment 装载后仍需处理执行代码的 I-cache 一致性；现有 `flush_cache()` 的全局操作语义已确认，P2 可以在确认性能/硬件语义后决定保留或优化，但不得静默改成 no-op。
- bootm 现有装载后全局 Cache 操作应保持独立，不要把 bootm handoff 与 HTTP/TFTP 上传完成同步混为一个接口。

## 26. P1-A5 静态审计收敛状态

截至本次检查：

- 板级 early init、CPLD/QLM/MDIO/PHY/boot-bus 调用链：已完成静态收敛；
- TLV EEPROM 软件访问窗口：源码已确认 256 bytes；物理器件容量仍 UNKNOWN；
- RAM relocation/linker/addrmap/load block：源码公式已确认；最终运行时 descriptor 和 relocation 数值待构建后只读验证；
- `__tmp_load` 生命周期、释放例外和只读查询 API：已完成静态收敛；
- TFTP/HTTP RAM 写入路径：已完成源码定位；尚未实现统一 pre-write validator；
- OCTEON ELF64 loader 和 PA-220 Cache API：本轮已完成静态调用链/语义审计；
- P1-A5 仍未进行最终构建或实机验证。

**P1-A5 源码静态审计可以结束；不代表上传安全功能已经实现。**统一 validator、TFTP/HTTP 接入和 ELF loader 加固属于后续开发工作。TLV 实际容量、最终 relocation/load block 数值、PHY/CPLD 硬件行为等可在构建后用只读命令确认的事项，不阻塞阶段性构建，必须保留到实机测试清单。
