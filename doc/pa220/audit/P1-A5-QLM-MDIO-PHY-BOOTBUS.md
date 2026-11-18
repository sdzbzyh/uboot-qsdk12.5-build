# P1-A5：OCTEON QLM/MDIO/PHY 与板级时序审计

- 状态：**本批源码静态审计完成；P1-A5 整体进行中；未编译、未实机验证**
- 日期：2026-11-18
- PA-220 固定源码：sdzbzyh/pa220-openwrt@801717ba6a8a59ed2cfd53bf3e1b19841fecd2e8
- 范围：pa220.dts、pa220_board.c、octeon_board_phy.c、octeon_board_common.c、octeon_boot.c、common/main.c、octeon_boot_bus.c。

## 1. PA-220 Ethernet/PHY 拓扑（DTS 证据）

- board/octeon/pa220/pa220.dts:86-160 定义 Marvell 88E1680 PHY，MDIO 地址 8-15；每个 PHY 节点带有 marvell,reg-init 序列。
- pa220.dts:175-181 定义 Marvell 88E1512 PHY，MDIO 地址 1，也带有 marvell,reg-init。
- pa220.dts:199-255 将 ethdev0..7 绑定到 PHY8..15；pa220.dts:266-270 的另一个接口绑定 PHY1。
- pa220.dts:197、231 等配置 cavium,qlm-trim = "0,qsgmii"；PHY 节点配置 cavium,qlm-trim = "0,qsgmii-qsgmii"。
- pa220.dts:370-422 定义 OCTEON bootbus 和 pan-cpld@0,0（compatible kingfisher-mp,cpld）；GPIO controller compatible 为 cavium,octeon-3860-gpio。
- 这是 PA-220 自身的 OCTEON QSGMII + Marvell PHY 拓扑，不是 IPQ PPE/QCA833x/Qualcomm S17 网络拓扑。

## 2. OCTEON PHY 初始化路径

### 2.1 编入关系
- board/octeon/common/Makefile:36-45 在未定义 CONFIG_OCTEON_DISABLE_NETWORKING 且 CONFIG_OF_LIBFDT 时编入 octeon_board_phy.o。
- octeon_common.h 定义 CONFIG_OF_LIBFDT；octeon_pa220.h 在 CONFIG_CMD_NET 下定义 CONFIG_PHY_MARVELL。
- board/octeon/common/octeon_board_common.c:610-617 定义 __board_mdio_init()，在 CONFIG_OF_LIBFDT 下调用 octeon_board_phy_init()，并将 board_mdio_init() weak alias 到该函数。
- octeon_board_phy_init()（约 698-900）遍历 eth_device 链表，读取每个 octeon_eth_info 的 phy_fdt_offset 和 phydev；根据 FDT compatible 分派 Marvell/Broadcom/Aquantia/Vitesse/TI/C22/C45 等配置函数。
- 对 Marvell PHY，调用 octeon_fdt_marvell_config()（210-285），读取 FDT marvell,reg-init 属性，逐项执行 phy_read()/phy_write()。
- 这是 MDIO/PHY runtime register 配置，不等于 EEPROM 或 PHY SPI Flash 写入。
- board_mdio_init() 为板级 hook；本次没有在 PA-220 固定源码中确认其最终调用点/时序，标记 NOT VERIFIED。不能把函数存在等同于实机已完成 PHY 初始化。

### 2.2 PA-220 具体 PHY 配置
- 88E1680 地址 8-15 和 88E1512 地址 1 由 pa220.dts 的 compatible + marvell,reg-init 提供参数。
- octeon_board_phy_init() 按 PHY 节点逐一调用 MDIO 配置；不应把 IPQ 的 QCA switch init、PPE port map 或 SMEM PHY table 移植过来。
- PA-220 现有 octeon_eth.c 驱动使用 OCTEON CVMX IPD/PIP/PKO；PA-220 QSGMII 前面板 PHY 必须沿用这套驱动和板级 FDT。
- 需要在后续只读实机测试中确认各端口 MDIO 地址、PHY ID、link 状态；P1 不进行 PHY 寄存器写测试。

## 3. QLM 初始化边界

- pa220_board.c:164-174 定义 board_configure_qlms()，调用 cvmx_qlm_set_mode(0, CVMX_QLM_MODE_QSGMII_QSGMII, ...) 配置 DLM0 为 QSGMII/QSGMII。
- PA-220 DTS 同时提供 cavium,qlm-trim 字段。
- 在固定源码中没有找到 board_configure_qlms() 的调用点；标记 NOT VERIFIED。不能仅凭函数存在就认定 DLM0 配置在当前 U-Boot 启动路径执行。
- 需要继续确认 QLM 是否由原厂前级/早期 OCTEON init/FDT helper 配置；不得在未确认当前状态时重复调用 cvmx_qlm_set_mode()，避免影响 PCIe/SGMII/QSGMII lane。
- 不新增 QLM 配置，不改 QLM mode，不做 PHY reset 实验。

## 4. PHY 自身 SPI Flash 写入代码边界

- octeon_board_phy.c:963-1092 实现 marvell_88x3140_flash_download()：通过 MDIO 命令先擦除 PHY 内部 SPI Flash program area，再分块写入并校验 buffer checksum/words written。
- octeon_board_phy.c:1105-1158 实现 marvell_88x3140_mdio_ram_download()，先向 PHY DSP RAM 下载 flash programmer stub。
- octeon_board_phy.c:1166-1224 注册 marvell88x3140 program 命令；要求 PHY flash programming jumper、用户按 ENTER 后才执行写入。
- 整段代码受 CONFIG_PHY_MARVELL && CONFIG_PHY_MARVELL_FLASH_PGM 条件编译。
- PA-220 固定配置链中未发现 CONFIG_PHY_MARVELL_FLASH_PGM 定义；因此不能认定 marvell88x3140 命令在 PA-220 编入。
- PA-220 DTS 绑定的是 88E1680/88E1512，不是 88x3140。该功能不适用于 PA-220；禁止因文件编译存在就移植/开放 PHY flash programming。
- 即使其他 OCTEON board 启用了该功能，它写的是 PHY 自身 SPI Flash，不是 PA-220 主板 SPI NOR；仍属于持久化写入。

## 5. PA-220 USB 电源时序补充

- pa220_board.c:120-145 early_board_init() 调用 gpio_direction_output(2, 0) 和 gpio_direction_output(3, 0)，代码注释说明保持 USB power off，直到 Linux 启动后再开启。
- pa220.dts:437、457 分别将 USB power-gpios 绑定到 GPIO2/GPIO3。
- 虽然 CONFIG_OCTEON_USB_OCTEON3、CONFIG_CMD_USB、CONFIG_USB_STORAGE 源码配置存在，但 U-Boot early_board_init() 会把 USB power 保持关闭。
- 因此 PA-220 的 USB Mass Storage 命令“源码存在/配置存在”不代表 U-Boot 阶段 USB 端口有供电或可用；P1-A4-USB-STORAGE.md 的 USB 实机可用性必须标记 NOT VERIFIED，不能作为当前可用的备份/升级介质。
- 不要在 Web 初始化时自动拉高 GPIO2/GPIO3；若将来需要 U-Boot USB 存储，必须先独立审查供电电流、端口拓扑、过流保护、热插拔和 Linux handoff。

## 6. CPLD/板级 hook 调用链

### 6.1 已确认调用
- board_enter_cli()：common/main.c:418-420 定义 weak default；main_loop() 在 autoboot 被中断/失败后于 485 行调用 board_enter_cli()。PA-220 实现会写 CPLD reg7 bit2，点亮 ALM green。
- board_prepare_os_handoff()：arch/mips/cpu/octeon/octeon_boot.c:2269-2281 定义 weak default；octeon_bootloader_shutdown() 调用它，发生在网络/USB shutdown 之前。PA-220 实现写 CPLD reg5 bit2（88E1680 PHY reset）和 reg7 bit2（ALM green off）。
- pa220_early_stat_led()：由 early_board_init() 直接调用，执行 reg6/reg7 LED read-modify-write。
### 6.2 部分/未确认调用
- octeon_boot_bus_board_post_init()：octeon_boot_bus.c:1930-1932 为 weak hook；octeon_boot_bus_late_init() 在 2018 行调用该 hook；PA-220 实现对 reg d bit0 做 low→high pulse。
- 但本次在 PA-220 固定源码的 board_r.c、board_f.c、cpu.c、octeon_boot.c 和 octeon_board_common.c 中没有找到 octeon_boot_bus_late_init() 的调用点。该 hook 的当前启动链可达性 NOT VERIFIED。
- board_configure_qlms()：函数定义存在，但固定源码未找到调用点；QLM 实际配置路径 NOT VERIFIED。
- 以上未确认项不能标记为“已执行的硬件初始化”；需要结合最终链接 map、启动日志和只读 CSR 观测确认。

## 7. 项目适配决策

- 保留 PA-220 octeon_eth.c、octeon_board_phy.c、octeon_gpio.c、pa220_board.c 及现有 FDT PHY 描述。
- 不移植 IPQ GPIO/TLMM、QCA switch/PPE、IPQ PHY reset、SMEM/MIBIB、9008 button recovery。
- 系统信息页优先读取 OCTEON board descriptor/TLV EEPROM、CVMX sysinfo、现有 eth_device/PHY 状态。
- PHY 信息只读；不得提供 MDIO arbitrary write 或 PHY SPI flash programming。
- CPLD 只读展示优先；已有 board hook 写操作保留。新增 CPLD/QLM/PHY 写操作必须单独说明寄存器、目的、影响和回滚。
- USB host 供电在 U-Boot 阶段默认关闭；不将 USB storage 作为当前可用维护通道。

## 8. 本批边界

- 已完成源码级 PHY/QLM/CPLD hook 追踪；若调用点未确认已明确标注。
- 未编译、未实机验证；未进行任何 QLM/MDIO/PHY/GPIO/CPLD 写操作。
