# PA-220 U-BootKit P1 审计总结与 P2 开发顺序

- 日期：2026-11-18
- 集成仓库：`sdzbzyh/uboot-qsdk12.5-build`
- 工作分支：`feature/pa220-ubootkit-web`
- uBootKit 固定审计源码：`chenxin527/uboot-qsdk12.5-build@9ce315735ac88fd0a7c7697bc527a8616ed0a972`
- PA-220 固定源码：`IronDemiGod/pa220-openwrt@801717ba6a8a59ed2cfd53bf3e1b19841fecd2e8`
- 本总结对应的工作分支前置检查点：`cb583b723d0bf69a477820a9eee5fa7c5c8a64eb`
- 状态：**P1-A0～A5 源码静态审计完成；未代表构建通过或实机通过。**

## 1. P1 完成范围

| 子阶段 | 审计范围 | 结论 |
|---|---|---|
| P1-A0 | uBootKit 固定源码树、模块边界、配置入口 | 已完成 |
| P1-A1 | HTTPD/TCP/failsafe 核心、页面资源、URI/handler 映射 | 已完成 |
| P1-A2 | sysinfo/network/env/syslog/misc/webterm/backup/mibib/mac/upgrade 业务模块及持久化写入链 | 已完成 |
| P1-A3 | BOOTP/DHCP、TFTP、WGET、TelnetD、Ethernet、NetLoop、net_abort | 已完成 |
| P1-A4 | eMMC、SPI NOR、GPT、FAT/EXT4、环境后端、bootloader update、USB storage | 已完成 |
| P1-A5 | PA-220 板级硬件调用链、TLV EEPROM、CPLD、QLM/MDIO/PHY/boot-bus、watchdog/handoff、RAM/linker/relocation、named block、TFTP/HTTP、ELF64、Cache | 已完成 |

上述“完成”仅指固定 commit 上的源码静态审计与文档记录。没有因为源码存在、配置存在或函数可编译就推断硬件存在/功能可用。

## 2. 最终板级初始化时序（源码调用链）

### 2.1 Stage 3 的 board_init_f

PA-220 使用 OCTEON 专用 `arch/mips/lib/board_octeon.c`，不是通用 MIPS board.c 的初始化序列。当前固定源码中关键先后顺序为：

1. `board_fdt_init_f()`
2. `init_twsi_bus()`
3. `board_early_init_f()`
4. `octeon_boot_bus_init()`
5. `early_board_init()`
   - PA-220 CPLD LED/状态初始化；
   - GPIO2/GPIO3 USB power 拉低；
   - TLV EEPROM clock/board descriptor/MAC 读取；
6. `octeon_env_init()` / `octeon_import_ram_env()`
7. baudrate、serial、console 初始化
8. `octeon_init_dram()`
9. `octeon_dram_test()`
10. `checkboard()` / `display_board_info()`

重要约束：`early_board_init()` 在 DRAM 初始化之前运行；不能把其中的 TLV/CPLD/GPIO 行为挪到依赖 DRAM 的晚期阶段，也不能在 Web/诊断路径重复执行板级初始化。

### 2.2 board_init_r 的关键硬件时序

完成 relocation 后，关键顺序为：

1. `octeon_boot_bus_moveable_init()`
2. `octeon_configure_load_memory()`：建立 `__tmp_load` named block，并设置 load reservation 信息
3. `octeon_configure_linux_reserved_memory()`
4. 可选 OCTEON hardware watchdog 启动
5. `octeon_boot_bus_late_init()`
   - 进入 PA-220 `octeon_boot_bus_board_post_init()`
   - 检查 boot-bus CS0 映射
   - CPLD reg d bit0 low-to-high pulse，间隔 10 ms
6. `board_configure_qlms()`：DLM0 QSGMII/QSGMII 配置
7. 网络阶段：
   - `board_net_preinit()`
   - `eth_initialize()`
   - OCTEON `cpu_eth_init()` / `octeon_mdio_initialize()`
   - `board_mdio_init()` / `octeon_board_phy_init()`
   - `board_net_postinit()`
8. 进入正常 U-Boot main loop/CLI

QLM、MDIO、PHY、boot-bus late-init hook 均已找到正式调用链。后续不要在 Web handler 中再次初始化 QLM/MDIO/PHY，也不要把 IPQ PHY/PPE 寄存器逻辑移植到 OCTEON。

### 2.3 OS handoff 与 bootelf 的不同生命周期

**bootm：**

1. `bootm_load_os()` 完成内核 copy/decompress 后调用 `flush_cache()`；
2. `boot_selected_os()` 调用 `arch_preboot_os()`；
3. PA-220 进入 `octeon_bootloader_shutdown()`；
4. 执行板级 handoff hook、关闭 watchdog、关闭网络/USB 等；
5. 默认调用 `octeon_free_tmp_named_blocks()` 释放所有 `__tmp*` named blocks；
6. `api_enable` 存在时跳过临时块释放；
7. `boot_jump_linux()` 跳转内核。

因此 bootm 的 cache 操作发生在镜像 load/decompress 阶段；OS handoff hook 本身不是 cache flush 函数。

**bootelf：**

1. `do_bootelf()` 验证 ELF；
2. `load_elf64_image()` 逐个装载 PT_LOAD 或 section；
3. 每个 segment/section 完成写入后调用 `flush_cache()`；
4. OCTEON `do_bootelf_exec()` 恢复标准 MIPS 配置后直接调用 entry。

bootelf 不调用 `arch_preboot_os()`，因此不会走 bootm 的 `__tmp*` 释放路径。若应用返回 U-Boot，必须重新查询 named block descriptor，不能仅凭 globals 仍有数值就认为 block 仍有效。

## 3. P1-A5 最终技术结论

### 3.1 RAM 与 named block

- 运行时上传窗口唯一权威来源为 `cvmx_bootmem_find_named_block("__tmp_load")`。
- 该 API 返回函数内部 static descriptor，必须立即复制 `base_addr/size`。
- 正常 U-Boot 阶段 `__tmp_load` 可作为条件安全上传窗口；OS handoff 默认释放所有 `__tmp*` block，`api_enable` 是例外。
- 默认 8 GiB 配置的源码公式结果为 physical `[0x20000000,0x30000000)`；最终实机值必须运行时查询。
- `0x81000000` 对应低端 DRAM physical 16 MiB，落入默认 Linux reserved 区，不能作为默认上传安全地址。
- relocation、heap、stack、GD/BD、bootparams、TLB mapping 等最终地址依赖构建产物和运行时 DRAM，不能静态写死。

### 3.2 TFTP / HTTP

- PA-220 TFTP `store_block()` 当前在普通 RAM 分支直接 memcpy，没有 named-block pre-write range check，也没有显式 `syncw`。
- uBootKit HTTP 的 IPQ 固定地址与 IPQ availability API 不可移植；其“分段 memcpy、完整 payload 后同步”流程可参考。
- HTTP 完整 payload 长度在接收前已知，必须先完整 reservation，再对每个 chunk 单独 pre-write check。
- 所有地址加法、offset 加法、block 乘法均需 checked arithmetic。
- 越界必须在 memcpy/memset 前拒绝，不能先写后告警。

### 3.3 ELF64

- 实际实现是 OCTEON 专用 `arch/mips/cpu/octeon/commands/cmd_elf64.c:load_elf64_image()`。
- `valid_elf_image()` 不知道实际 payload 长度；PT_LOAD 和 section fallback 均缺少完整 source/destination 范围验证。
- program-header 地址和 source image 指针存在 32-bit cast；没有完整的乘法/加法溢出、`p_offset+p_filesz`、`p_filesz<=p_memsz` 检查。
- `octeon_fixup_xkphys()` 不是安全范围验证器。
- little-endian 8-byte swap loop 对非 8-byte 整数倍长度有越界访问风险。
- 每个 segment 后确实调用 `flush_cache()`，但目标地址必须先验证到运行时 `__tmp_load`。

### 3.4 Cache API

| API | PA-220 实际语义 |
|---|---|
| `flush_cache(start,size)` | 全局同步并失效 I-cache、D-cache；忽略 start/size |
| `flush_dcache_range(start,stop)` | `syncw`；不是逐 Cache Line 操作 |
| `invalidate_dcache_range(start,stop)` | 空实现 |

数据上传完成同步与代码执行前 I-cache 维护是不同问题。不能把全局 `flush_cache()` 当成轻量的 payload range flush。

## 4. 统一 RAM 写入前 validator 的 P2/P6 实施约束

建议的公共入口：

`int octeon_load_range_check(uint64_t addr, uint64_t len);`

内部每次查询当前 `__tmp_load` descriptor，立即复制 base/size，并检查 descriptor 与 `load_reserved_addr/load_reserved_size` 一致。用减法式半开区间验证：

- `addr >= base`
- `offset = addr - base`
- `offset <= size`
- `len <= size - offset`

所有乘法/加法都使用 checked arithmetic；不能先计算可能溢出的 end address。该接口必须在以下写入发生之前调用：

1. TFTP 每个接收 block 的 memcpy；
2. HTTP 第一次 payload memcpy 前的完整 reservation；
3. HTTP 每个后续 TCP chunk 的 memcpy；
4. ELF 每个 PT_LOAD segment 的 memcpy/memset；
5. ELF section fallback 的每个写入（若保留该路径）。

ELF 还必须验证源文件 header/program-header/source segment 范围、segment 目标范围、source/destination overlap、segment overlap 和 entry executable segment。Cache 同步在范围检查通过并完成写入后单独执行。

## 5. P2 阶段的执行顺序

P2 定义仍按 ROADMAP：PA-220 固定基线、补丁叠加、工具链和可重复构建复核。

1. 锁定分支、固定源码 commit、补丁顺序、工具链版本。
2. 干净执行 `build-pa220.sh`；记录完整日志、最终 .config、map/ELF、`uboot_end`、产物大小和 SHA256。
3. 静态确认 `CONFIG_CMD_OCTEON_ELF`、`cmd_elf64.o`、named-block API、Cache API 相关对象/符号确实编入。
4. 使用原厂 Stage2 的 TFTP 将新 Stage3 加载到 RAM 测试；不执行 SPI/eMMC/NAND/EEPROM 写入。
5. 收集运行时只读信息（见第 6 节）。这些项目不阻塞构建。
6. P2 验收后进入 P3 只读 CLI/诊断；随后按 ROADMAP 进入 P4 网络、P5 HTTP 只读、P6 RAM 上传、P7 dry-run、P8 eMMC、P9 启动管理、P10 SPI 独立高风险节点、P11 回归发布。
7. P6 开放任何 HTTP/TFTP 上传前，必须先实现并接入统一 `__tmp_load` validator。validator 未完成前，不把新 U-Boot 的网络接收路径标记为安全可用。

## 6. 构建后必须提醒用户执行的只读验证

这些事项不影响当前静态审计结论或阶段性构建，按要求留待构建后通过只读命令确认：

1. 完整启动日志：记录 DRAM 容量、Base DRAM address、Stack Pointer、TLB map size（若可见）。
2. 执行 `bdinfo`，反馈完整输出。
3. 执行 `printenv loadaddr octeon_reserved_mem_load_base octeon_reserved_mem_load_size`。
4. 通过现有只读命令或后续 P3 只读诊断输出查询 `__tmp_load` descriptor 的实际 base/size。
5. 记录最终 relocation 地址和使用长度（结合 map/ELF 与启动日志）。
6. 执行 TLV EEPROM 只读检查：`i2c dev 0`、`i2c probe 57`，只读读取 offset 0、0x100、0x200，记录 END tuple/MAC/board descriptor/clock tuple。
7. 只读确认 PHY ID/link、CPLD version/reg5/reg d 与 watchdog/failsafe 行为；不要为验证而主动写 CPLD 寄存器。

**禁止：**`saveenv`、`eeprom write`、TLV set/delete、`sf write/erase/update`、eMMC/NAND/USB block write、任何未经独立批准的持久化写入。

## 7. 最终状态声明

P1-A0～A5 固定源码静态审计已完成；P1 没有执行构建、RAM 启动或实机功能测试。源码/配置存在不等于硬件存在，编译成功不等于实机成功。所有运行时可只读确认的 UNKNOWN 项都保留在第 6 节，不阻塞 P2。

