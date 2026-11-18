# P1-A5：平台硬件 API 审计（第一批：IPQ API/SMEM/GPIO/LED 与 PA-220 OCTEON 板级接口）

- 状态：**本批源码静态审计完成；P1-A5 整体进行中；未编译、未实机验证**
- 日期：2026-11-18
- uBootKit：chenxin527/uboot-qsdk12.5-build@9ce315735ac88fd0a7c7697bc527a8616ed0a972
- PA-220：sdzbzyh/pa220-openwrt@801717ba6a8a59ed2cfd53bf3e1b19841fecd2e8
- 范围：lib/ipq_api.c、lib/ipq_led.c、include/ipq_api.h、arch/arm/cpu/armv7/qca/common/smem.c、common/main.c；对照 PA-220 pa220_board.c、octeon_board_common.c、octeon_gpio.c。

## 1. uBootKit IPQ API 构建与启动挂钩

- include/ipq_api.h 声明 IPQ GPIO/LED、failsafe、flash detect、MIBIB、内存范围、board info、partition、env 等 API。
- lib/Makefile:100：CONFIG_LIB_IPQ_API 编入 ipq_api.o、ipq_led.o。
- arch/arm/cpu/armv7/qca/common/Makefile:3：SMEM 目标目录编入 smem.o。
- board/qca/arm/common/board_init.c:475 在板级 late init 路径调用 detect_flash_device()。
- common/main.c:92-95 在 CONFIG_HTTPD 下，main_loop() 先后调用：
  1. reload_mibib_from_flash_and_set_default_flash_type_in_9008_mode()
  2. do_network_check()
  3. do_httpd_check()
- 这说明 CONFIG_HTTPD 不只是编入 HTTP server；它还在 autoboot 前增加 QCA recovery/网络环境/按钮检查流程。目标配置是否同时启用 CONFIG_HTTPD 与 CONFIG_LIB_IPQ_API，仍需最终 .config 确认。

## 2. IPQ API 逐项边界

### 2.1 GPIO/LED/按钮
- lib/ipq_api.c:53-73 ipq_gpio_init() 遍历 FDT 的 /leds、/keys、/gpio-export 节点，调用 gpio_request()/gpio_direction_input()/gpio_direction_output()。
- lib/ipq_api.c:75-145 is_button_pressed()/is_any_button_pressed()/is_any_button_pressed_for_enough_time() 通过 FDT GPIO 描述和 gpio_get_value() 检测按键，长按约 3 秒期间控制 power_led。
- lib/ipq_led.c 通过 FDT LED label 和 IPQ GPIO 控制 led_on()/led_off() 等状态。
- 这些依赖 QCA TLMM/GPIO、FDT 节点命名和 IPQ LED 控制，不可直接移植到 OCTEON GPIO/CPLD。

### 2.2 Failsafe 启动和环境变量
- lib/ipq_api.c:147-161 failsafe_env_exists() 读取 failsafe 环境变量；值不是 always 时 setenv("failsafe", NULL) 后调用 saveenv()。
- lib/ipq_api.c:163-206 check_9008_mode_and_failsafe_env() 检查 9008 recovery mode、failsafe env 和按键状态。
- lib/ipq_api.c:209-224 do_httpd_check() 在触发条件满足时 run_command("httpd") 并进入 cli_loop()。
- lib/ipq_api.c:226-290 do_network_check() 在 custom_network 未启用时设置默认 ipaddr/netmask/serverip；如果值发生变化调用 saveenv()。
- 上述两个路径均可能在 autoboot 前触发 SPI/NAND/MMC 环境持久化写入；尤其 do_network_check() 的 saveenv() 不是单纯修改 RAM。
- PA-220 没有 QCA 9008 recovery mode，也没有相同 failsafe env/按键状态机。不能将这些 main_loop hook 整体移植到 PA-220。
- PA-220 网络环境应沿用现有 Stage2 传入环境和 NetInitLoop()；禁止启动时自动覆盖用户网络配置或调用 saveenv()。

### 2.3 Flash detect / SMEM / MIBIB
- lib/ipq_api.c:293-348 detect_flash_device()/has_spi()/has_nand()/has_mmc() 检测 QCA SPI/NAND/MMC 控制器和设备存在状态。
- lib/ipq_api.c:349-405 flash_type_to_string()/string_to_flash_type() 转换 QCA SMEM flash type enum。
- lib/ipq_api.c:406-448 get_mibib_and_ptable_addr() 在镜像内扫描 Qualcomm MIBIB/PTABLE magic。
- lib/ipq_api.c:476-534 reload_mibib_from_flash() 从检测到的 QCA flash 读取 MIBIB 到 RAM，解析并重建 SMEM partition table；该恢复路径更新 RAM 元数据，不等于写回 MIBIB flash。
- lib/ipq_api.c:535-588 reload_mibib_from_flash_and_set_default_flash_type_in_9008_mode() 在 9008 recovery 下尝试 SPI/NAND/MMC 的 MIBIB 恢复顺序。
- arch/arm/cpu/armv7/qca/common/smem.c:312-380 枚举 CONFIG_QCA_SMEM_BASE 指向的 Qualcomm shared-memory partition；396-458 读取 SMEM allocation entries；460-515 初始化 SMEM flash partition table；516-560 读取 dual-boot config；628-720 提供 SMEM partition lookup；722-780 读取 boot flash type/index/CS/block size/density。
- PA-220 没有 Qualcomm SMEM/MIBIB/PTABLE，也没有 QCA flash_type enum；内部 eMMC 使用 GPT，板级数据使用 TLV EEPROM，SPI NOR 布局由 PA-220 boot chain/CPLD/板级配置定义。
- 适配判断：QCA SMEM/MIBIB/flash detect 不可移植；getpart_offset_size 类需求改用 PA-220 GPT 只读查询和固定 board-specific layout。

### 2.4 内存范围和通用辅助函数
- lib/ipq_api.c:590-606 is_load_addr_valid()、607-635 is_memory_region_available() 使用 CONFIG_SYS_SDRAM_BASE、gd->ram_size、IPQ FDT/TrustZone/secure memory 等边界。
- 其中 load_addr + size 的边界计算需要防止整数溢出；QCA 保留区不能直接代表 PA-220 保留区。
- PA-220 的 DRAM 已由原厂 Stage2 初始化；上传/TFTP/WGET/RAM boot 地址必须按 PA-220 U-Boot、heap、stack、GD、mtest、Stage2 handoff、TFTP buffer 和用户加载地址重新规划。
- lib/ipq_api.c:636-689 print_board_info() 读取 FDT model/compatible 等信息；PA-220 应以 OCTEON board descriptor/TLV EEPROM 和 CVMX sysinfo 为准。
- lib/ipq_api.c:690-734 json_escape()、753-766 set_file_info_env()、767-798 print_progress_bar()、799-821 get_enable_state() 属于相对通用的辅助逻辑，可在核对 PA-220 旧 U-Boot API 后单独重写/复用。
- lib/ipq_api.c:735-752 mmc_part_exists() 使用 QCA GPT partition lookup；PA-220 可改用 get_partition_info_efi_by_name() 或已有 GPT API，但需按旧 U-Boot 签名适配。

## 3. PA-220 OCTEON 板级信息和 GPIO

### 3.1 TLV EEPROM 读取
- board/octeon/pa220/pa220_board.c:120-145 early_board_init()：
  - pa220_early_stat_led() 设置 CPLD 前面板 LED；
  - GPIO2/GPIO3 输出低，保持 USB power off；
  - octeon_board_get_clock_info(PA220_DEF_DRAM_FREQ)；
  - octeon_board_get_descriptor(CVMX_BOARD_TYPE_PA220, 1, 0)；
  - octeon_board_get_mac_addr()；
  - 依据 OCTEON CPU multiplier 设置 gd->cpu_clk。
- board/octeon/common/octeon_board_common.c:409-433 octeon_board_get_mac_addr() 读取 TLV EEPROM MAC tuple；缺失时生成 locally administered random MAC。
- octeon_board_common.c:435-505 octeon_board_get_clock_info() 读取 clock/DDR clock tuple。
- octeon_board_common.c:507-595 octeon_board_get_descriptor() 读取 board type、revision、serial；并检查 OCTEON_GPIO_FAILSAFE_BIT。
- PA-220 board info 来源是 TLV EEPROM + OCTEON GPIO/CSR，不是 QCA SMEM，也不能直接以 IPQ FDT board_info 替代。

### 3.2 OCTEON GPIO
- drivers/gpio/octeon_gpio.c:54-69 gpio_in_range() 按 OCTEON 节点/芯片限制 GPIO 范围。
- gpio_direction_input()/gpio_direction_output()/gpio_get_value()/gpio_set_value()（84-127）通过 CVMX GPIO CSR 操作。
- PA-220 early_board_init() 使用 GPIO2/GPIO3 控制 USB power；octeon_board_common.c 使用 OCTEON_GPIO_FAILSAFE_BIT 输入。
- 这套 API 是 OCTEON CVMX GPIO，不是 QCA TLMM；IPQ GPIO 编号、复用、上下拉和电平语义不能直接套用。

## 4. PA-220 CPLD：现有硬件操作必须保留并明确标记

文件：board/octeon/pa220/pa220_board.c。

### 4.1 CPLD 映射
- CPLD 位于 boot bus CS0，物理窗口 0x1b040000。
- start.S 会关闭 CS0；pa220_early_stat_led() 通过 /soc/bootbus DT node 和 octeon_boot_bus_generic_init() 重新映射。
- octeon_boot_bus_board_post_init()（46-68）读取 boot bus CS0 配置；若映射正确，读取 reg d 并执行 bit0 low→high pulse。代码注释说明这是告诉 CPLD 当前 normal-flash bootloader 存活，否则 CPLD watchdog 会在数秒后切换到 failsafe SPI flash。
- 该回调是否在当前 PA-220 启动链每次都被调用，仍需核对 bootbus callback 调用点；本报告只确认函数本身的硬件操作。

### 4.2 已有寄存器写操作
- pa220_early_stat_led()（95-117）：
  - reg7：设置 STAT orange，清除 HA/ALM green/STAT green 的相关位；
  - reg6：清除 TEMP LED bits 0-1；
  - 使用 read-modify-write，保留未知 reg6 bits。
- board_prepare_os_handoff()（177-194）：
  - reg5 bit2 = 88E1680 PHY reset；
  - reg7 bit2 = ALM green off。
- board_enter_cli()（196-208）：
  - reg7 bit2 = ALM green on。
- 这些是现有板级 LED/PHY/boot watchdog 操作；用户允许保留已有 CPLD 写入。本审计没有新增寄存器写入，也没有执行。

### 4.3 CPLD 命令
- do_cpld()（220-265）支持 display/read/write reg 0-d。
- reg0 为 version read-only；reg5 bit2 控制 88E1680 reset；reg6 TEMP LED；reg7 HA/ALM/STAT LED；reg d 为 flash select/reboot。
- 写 reg d 必须显式使用 cpld write -f d <val>；这是高风险硬件控制，不得暴露到 Web RPC 或远程命令接口。
- 对新增 CPLD 写操作，必须逐项记录寄存器/bit、目的、默认值、影响、回滚方式和实机测试注意事项。

## 5. TLV EEPROM 持久化写入入口（完整细节在 P1-A5-TLVEEPROM-CPLD.md）

- PA-220 octeon_pa220.h:51-61：EEPROM I2C address 0x57、24C256 page size 64 bytes、page write delay 5 ms、address length 1。
- octeon_cmd_conf.h 启用 CONFIG_CMD_EEPROM；PA config 启用 CONFIG_CMD_OCTEON_TLVEEPROM。
- common/cmd_eeprom.c:86-92 的 eeprom write 命令直接调用 eeprom_write()；eeprom_write()（192-363）通过 I2C page writes 写入 EEPROM。
- cmd_octeon_tlveeprom.c:103-141 write_tlv_tuple() 计算 TLV checksum 后调用 eeprom_write()。
- cmd_octeon_tlveeprom.c:144-212 delete_tuple() 将目标 tuple 填 0xff，再把后续 tuple 逐个搬移覆盖；没有事务/双副本保护，断电或中途失败可破坏后续 board data。
- do_tlv_eeprom()（252-738）提供 display/read、set、delete；可修改 board descriptor、MAC base/count、clock/DDR clock、chip capability、voltage/multiplier 等。
- TLV EEPROM 直接影响 board type、revision、serial、MAC、CPU/DDR clock；Web 第一版只读展示，不开放 raw eeprom write 或 tlv_eeprom set/delete。

## 6. 适配结论

| uBootKit IPQ 功能 | PA-220 对应 | 适配判断 |
|---|---|---|
| SMEM/MIBIB/SMEM partition table | eMMC GPT + 固定板级布局 | 不可直接移植；重写只读 GPT API |
| IPQ flash detect | PA 固定 OCTEON MMC/SPI 配置 + CVMX | 保留现有驱动，不移植 |
| IPQ GPIO/LED | OCTEON CVMX GPIO + PA CPLD | 重新实现板级控制 |
| 9008/button failsafe | PA GPIO failsafe input/CPLD boot watchdog | 需独立设计，不照搬 |
| do_network_check | PA Stage2 传入环境 + NetInitLoop | 不移植；禁止自动 saveenv |
| print_board_info | OCTEON board descriptor/TLV EEPROM/CVMX sysinfo | 重写只读展示 |
| is_memory_region_available | PA-220 DRAM/Stage3 reserved layout | 算法可参考，地址边界必须重写 |
| json_escape/get_enable_state/progress | 无硬件依赖 | 可单独复用/重写 |
| EEPROM/TLV set/delete | PA TLV EEPROM | 仅保留离线维护，不开放 Web 写入 |
| CPLD reg read/write | PA bootbus CS0 CPLD | 只读可研究；reg d 写入高风险且需 -f |

## 7. 本批边界

- P1-A5 第一批完成 IPQ API/SMEM/GPIO/LED 与 PA-220 board/GPIO/CPLD/TLV API 初步对照。
- TLV EEPROM 与 CPLD 详细写入路径需要独立报告补充。
- 尚未完整审计所有 QCA IPQ board-specific driver、PA OCTEON QLM/MDIO/PHY/USB power sequencing。
- 未编译、未实机验证；未执行 EEPROM、CPLD、SPI、eMMC 写操作。
