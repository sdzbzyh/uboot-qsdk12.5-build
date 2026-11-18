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
