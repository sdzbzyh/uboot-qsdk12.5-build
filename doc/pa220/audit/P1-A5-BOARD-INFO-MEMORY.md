# P1-A5：PA-220 板卡信息、时钟与 DRAM 运行时信息审计

- 状态：**本批源码静态审计完成；P1-A5 整体进行中；未编译、未实机验证**
- 日期：2026-11-18
- PA-220 固定源码：sdzbzyh/pa220-openwrt@801717ba6a8a59ed2cfd53bf3e1b19841fecd2e8
- uBootKit 固定源码：chenxin527/uboot-qsdk12.5-build@9ce315735ac88fd0a7c7697bc527a8616ed0a972
- 范围：IPQ print_board_info()/memory checks 与 PA-220 CVMX sysinfo、TLV board descriptor、DRAM init 和内存配置对照。

## 1. PA-220 板卡信息数据源

### 1.1 TLV EEPROM
- early_board_init() 调用 octeon_board_get_descriptor(CVMX_BOARD_TYPE_PA220, 1, 0)。
- octeon_board_get_descriptor() 解析 TLV EEPROM board descriptor，填充 gd->arch.board_desc.board_type、revision、serial，并同步 gd->board_type。
- octeon_board_get_mac_addr() 读取 MAC base/count；octeon_board_get_clock_info() 读取 CPU/DDR clock descriptor。
- 这些 API 在 P1-A5-TLVEEPROM-CPLD.md 已追踪；此处只确认它们是 PA-220 系统信息页的主要 board-specific 数据源。

### 1.2 CVMX sysinfo
- arch/mips/include/asm/arch-octeon/cvmx-sysinfo.h 定义 struct cvmx_sysinfo，包含：
  - system_dram_size；
  - cpu_clock_hz；
  - board_type；
  - board_rev_major/board_rev_minor；
  - board_serial_number；
  - 以及 OCTEON 节点/内存相关字段。
- cvmx_sysinfo_get() 返回当前 CVMX sysinfo。
- 只读状态页可将 CVMX sysinfo 与 gd->arch.board_desc 组合展示；不能将其等同于 Qualcomm SMEM board info。
- board/octeon/common/octeon_board_common.c:87-94 的默认 checkboard() 是 weak stub，返回 0，不输出 PA-220 硬件信息；不能依赖 checkboard() 作为完整 board info API。

## 2. DRAM 大小来源和 Stage2 handoff

- PA-220 原厂 Stage2 先完成 DRAM 初始化，再加载 Stage3 U-Boot；Stage3 必须尊重 Stage2 传入的内存大小。
- arch/mips/cpu/octeon/octeon_dram.c:340-360：
  - 若 GD_FLG_RAM_RESIDENT 且存在 dram_size_mbytes 环境变量，直接使用该值；
  - 注释明确说明 DRAM controller 已由 remote entity 初始化，因此不重新读取 DIMM SPD。
- octeon_dram.c:480-512：
  - 读取 dram_size_mbytes；
  - 允许将 mem_mbytes 向下缩小；
  - 若请求大于实际/已检测内存则拒绝扩大；
  - 最终设置 gd->ram_size 和 gd->arch.ram_sizes[0]。
- octeon_pa220.h 的默认环境包含 dram_size_mbytes=8192，但配置注释说明 Stage2 传入值优先；该默认值不是实机 DRAM 容量证据。
- 实际 dram_size_mbytes、gd->ram_size 和多节点内存布局必须在设备运行时只读确认；本报告不把 8192 MiB 当作已实测容量。

## 3. PA-220 内存配置证据

- octeon_common.h:53-55：CONFIG_SYS_TEXT_BASE 默认 0xC0000000。
- octeon_common.h:263：CONFIG_SYS_MALLOC_LEN = 12 MiB。
- octeon_common.h:266-268：CONFIG_SYS_LOAD_ADDR 默认 0x20000000。
- octeon_common.h:280：CONFIG_SYS_SDRAM_BASE = 0x80000000（cached address）。
- octeon_common.h:507：CONFIG_SYS_BOOTMAPSZ = 1 MiB。
- octeon_pa220.h:196-197：CONFIG_SYS_MEMTEST_START=0x22000000、CONFIG_SYS_MEMTEST_END=0x2a000000。
- 项目决策 D-005 已区分 0xC0000000 链接/镜像头地址与 0x81000000 安装资料中的 TFTP RAM 加载地址。
- 0x20000000 是 CONFIG_SYS_LOAD_ADDR 默认值，0x81000000 是既有 TFTP 测试资料地址，二者不能混为一谈；都不能仅凭配置/文档推断为任意长度上传的安全缓冲区。
- MIPS64/n32 地址转换、cached/uncached alias、Stage2 保留区、U-Boot image/heap/stack/GD、FDT、bootmap、mtest 区域和用户 TFTP/RAM boot 地址必须一起纳入内存规划。

## 4. uBootKit IPQ 内存检查的适配边界

- lib/ipq_api.c:is_load_addr_valid()/is_memory_region_available() 使用 CONFIG_SYS_SDRAM_BASE、gd->ram_size 和 IPQ secure/TZ/FDT 保留区判断。
- 其“先验证地址区间，再允许 upload/download”思路可借鉴。
- IPQ 保留区宏、ARM 物理地址和 FDT high memory 不能直接用于 PA-220。
- PA-220 应使用 gd->ram_size/cvmx_sysinfo_get() 的运行时 DRAM 范围，并排除 PA-220 实际 U-Boot/heap/stack/GD/FDT/bootmap/Stage2 handoff/测试区和上传缓冲区。
- 区间检查必须使用防溢出写法：先检查 addr <= end，再检查 len <= end - addr；不能只检查 addr + len <= end。
- 需要考虑 MIPS64 pointer、n32 ABI 和 cached/uncached alias；不能将普通 unsigned long 运算直接当作完整物理地址验证。
- 在明确 PA-220 安全 RAM 区域之前，不实现 HTTP upload/TFTP WGET 缓冲区自动选择。

## 5. 适配结论

| uBootKit 功能 | PA-220 对应数据源 | 适配结论 |
|---|---|---|
| print_board_info() | TLV board descriptor + CVMX sysinfo | 重写只读展示 |
| FDT model/compatible | PA-220 DTS（若运行时 fdt_blob 可用） | 可作补充信息，不能替代 EEPROM board descriptor |
| CPU/DDR clock | TLV clock tuples + gd->cpu_clk/gd->mem_clk + CVMX sysinfo | 只读展示；数值来源需标明 |
| DRAM size | gd->ram_size + Stage2 dram_size_mbytes + CVMX sysinfo | 运行时读取；不硬编码 8192 MiB |
| is_memory_region_available() | PA-220 MIPS64/n32 DRAM 与 reserved map | 重写；地址范围未完成前不开放上传 |
| set_file_info_env() | PA 旧环境 API | 可重写为 RAM-only 元数据设置，避免 saveenv |

## 6. 本批未完成

- 尚未通过最终链接 map/运行时 dump 确认 board info、CVMX sysinfo、DRAM size 的实际初始化顺序。
- 尚未完成 PA-220 安全 RAM 上传区的精确地址图；CONFIG_SYS_LOAD_ADDR、TFTP 测试地址、mtest 区间不能互相替代。
- 未进行任何 DRAM 测试、地址探测、上传或持久化写入。
- 编译成功和实机验证均 NOT VERIFIED。
