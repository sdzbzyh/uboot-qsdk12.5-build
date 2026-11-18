# P1-A5：PA-220 TLV EEPROM 与 CPLD 硬件写入路径审计

- 状态：**本批源码静态审计完成；P1-A5 整体进行中；未实机验证**
- 日期：2026-11-18
- PA-220 固定源码：sdzbzyh/pa220-openwrt@801717ba6a8a59ed2cfd53bf3e1b19841fecd2e8
- 前置：P1-A5-IPQ-PA-HARDWARE-API.md
- 范围：cmd_octeon_tlveeprom.c、lib_octeon_shared.c TLV parser、common/cmd_eeprom.c、pa220_board.c CPLD/GPIO 操作。

## 1. TLV EEPROM 硬件配置

PA-220 octeon_pa220.h:
- CONFIG_SYS_I2C_EEPROM_ADDR = 0x57。
- CONFIG_SYS_DEF_EEPROM_ADDR 指向同一设备。
- CONFIG_SYS_EEPROM_PAGE_WRITE_BITS = 6，即 page size 64 bytes。
- CONFIG_SYS_EEPROM_PAGE_WRITE_DELAY_MS = 5。
- CONFIG_SYS_I2C_EEPROM_ADDR_LEN = 1。
- CONFIG_CMD_EEPROM 和 CONFIG_CMD_OCTEON_TLVEEPROM 在固定配置链中启用。

板级注释将该设备描述为 24C256、64-byte page；但 CONFIG_SYS_I2C_EEPROM_ADDR_LEN=1 在 common/cmd_eeprom.c 中走“8-bit EEPROM page address/bank select”分支，而 24C256 通常使用 16-bit word address。不能只凭注释认定物理器件/寻址方式正确；实际 EEPROM 型号、地址映射、可访问容量需要只读实测确认，标记 NOT VERIFIED。

## 2. TLV EEPROM 读取和解析

### 2.1 底层 parser
- 文件：arch/mips/cpu/octeon/lib_octeon_shared.c:10924-11052。
- octeon_tlv_eeprom_get_next_tuple()：
  - 通过 eeprom_read() 读取 TLV header；
  - 将 type/length/checksum 从 big-endian 转换；
  - 检查 length 不小于 header、不大于 OCTEON_EEPROM_MAX_TUPLE_LENGTH（256）；
  - 读取 payload 并计算 16-bit byte-sum checksum；
  - checksum 不匹配返回 -1，合法 tuple 返回 length。
- octeon_tlv_get_tuple_addr()：
  - 从 CONFIG_EEPROM_TLV_BASE_ADDRESS（若定义）或 EEPROM offset 0 开始顺序扫描；
  - 找到匹配 type/major_version 返回正 offset；
  - 未找到时返回下一个可用 offset，并设置 bit31 标识“new tuple”。
- parser 没有显式 EEPROM 总容量参数；损坏/缺失 END tuple 时，扫描上限依赖调用方和底层 I2C 设备行为。需补容量边界审查。

### 2.2 PA-220 启动读取调用链
- board/octeon/pa220/pa220_board.c:120-145 early_board_init() 调用 octeon_board_get_clock_info()、octeon_board_get_descriptor()、octeon_board_get_mac_addr()。
- board/octeon/common/octeon_board_common.c:409-433：
  - octeon_board_get_mac_addr() 通过 octeon_tlv_get_tuple_addr() 查找 EEPROM_MAC_ADDR_TYPE；
  - 找到后读取 base MAC 和 count；
  - 不存在/读取失败时生成 locally administered random MAC。
- octeon_board_common.c:435-505：
  - octeon_board_get_clock_info() 读取 clock descriptor 和 DDR clock tuple；
  - 依据 tuple 版本解析 CPU/DDR clock 参数；缺失时使用传入默认值。
- octeon_board_common.c:507-595：
  - octeon_board_get_descriptor() 根据 OCTEON_GPIO_FAILSAFE_BIT 和 TLV EEPROM 读取 board type、revision、serial；
  - 若读取失败会使用传入的 board type/revision fallback。
- EEPROM 内容会影响 board identity、MAC、clock/DDR clock；不能把它当作普通用户配置区。

## 3. TLV EEPROM 持久化写入命令

### 3.1 tlv_eeprom write path
- 文件：arch/mips/cpu/octeon/commands/cmd_octeon_tlveeprom.c。
- write_tlv_tuple()（103-141）：
  1. 读取 TLV type/major_version/length；
  2. checksum 字段清零；
  3. 对 tuple length 范围内所有字节计算 16-bit sum；
  4. 调用 octeon_tlv_get_tuple_addr() 查找现有 tuple 或下一个空闲位置；
  5. 将返回地址的 bit31 标记清除（addr &= 0xffff）；
  6. 调用 eeprom_write(CONFIG_SYS_DEF_EEPROM_ADDR, addr, ...) 写入。
- 未发现写后完整 read-back/重新校验 tuple checksum 的流程。
- 若 EEPROM 扫描遇到损坏 tuple/缺失 END marker，必须先确认 next free offset 不越过实际器件容量；不能把 addr mask 当成容量验证。

### 3.2 delete path
- delete_tuple()（144-212）：
  1. 读取 delete_addr 处 tuple；
  2. 将被删除 tuple 填 0xff 并写入；
  3. 从后续地址逐个读取 tuple；
  4. 将后续 tuple 向前搬移覆盖空洞；
  5. 对未覆盖的旧 tuple header 再写 0xff。
- 该操作是多次 I2C page write，没有双副本、事务或回滚。
- 第一笔删除写入的 eeprom_write() 返回值未被检查；清理旧 header 的 eeprom_write() 返回值也未检查。中途失败可能留下 TLV 空洞、重复 tuple 或损坏后续 board data。
- 删除命令以用户输入地址为目标；不能由 Web 任意暴露。

### 3.3 可修改的数据类型
do_tlv_eeprom()（252-738）支持 display/print/help、set、delete。set 分支可修改：
- clock descriptor / CPU multiplier / voltage；
- board descriptor（board type、revision、serial）；
- chip capability；
- MAC base address 和 MAC count；
- voltage multiplier、voltage descriptor；
- DDR clock descriptor；
- DDR verbose 配置。
这些值直接参与板级初始化和网卡 MAC 分配。P1 Web 初版只读展示，不开放 set/delete。

## 4. 通用 raw EEPROM 命令

- 文件：common/cmd_eeprom.c。
- CONFIG_CMD_EEPROM 下注册 eeprom 命令。
- 命令处理路径约 53-101：eeprom read 调用 eeprom_read()；eeprom write 调用 eeprom_write()。
- eeprom_write()（192-363）按 page boundary 分段，依赖 CONFIG_SYS_EEPROM_PAGE_WRITE_BITS 和 CONFIG_SYS_EEPROM_PAGE_WRITE_DELAY_MS，使用 i2c_write() 完成写入。
- 命令层通过 simple_strtoul() 解析 device address、RAM address、EEPROM offset 和 byte count；未见针对实际 TLV 区域的白名单限制。
- raw eeprom write 可以绕过 TLV checksum/parser，直接覆盖 board descriptor、MAC、clock tuple 或 END marker。
- 因此即使禁用 tlv_eeprom set/delete，保留通用 eeprom write 仍可绕过限制；Web 不能提供任意 eeprom write。

## 5. PA-220 CPLD 寄存器和已有写操作

文件：board/octeon/pa220/pa220_board.c。

### 5.1 地址和映射
- CPLD 位于 OCTEON boot bus CS0，物理窗口 0x1b040000。
- start.S 关闭 CS0；pa220_early_stat_led() 通过 FDT /soc/bootbus 的 pan-cpld@0,0 节点重新映射。
- reg0 是 CPLD version，只读。
- reg5 bit2 控制 88E1680 PHY reset。
- reg6 bits0-1 为 TEMP LED。
- reg7 控制 HA/ALM/STAT LEDs。
- reg d 控制 flash select/reboot；写入需要显式 -f。

### 5.2 当前已有 CPLD 写路径
- pa220_early_stat_led()（95-117）：read-modify-write reg7/reg6，设置 STAT orange，清除 HA/ALM green/STAT green/TEMP 状态。
- octeon_boot_bus_board_post_init()（46-68）：读取 reg d，bit0 low→high pulse，向 CPLD boot watchdog 表明 normal-flash bootloader 存活；否则 watchdog 会切换到 failsafe SPI flash。函数调用点仍需核对。
- board_prepare_os_handoff()（177-194）：设置 reg5 bit2，保持 88E1680 PHY reset；清除 reg7 bit2 ALM green。
- board_enter_cli()（196-208）：设置 reg7 bit2 ALM green。
- early_board_init()（120-145）：GPIO2/GPIO3 输出低，保持 USB power off。
- 这些是现有板级操作；用户已允许保留现有 CPLD 写入。本次只记录，不修改、不执行。

### 5.3 cpld 命令
- do_cpld()（220-265）允许 display/read/write。
- write reg d 只有在显式 cpld write -f d <val> 时才允许；普通 cpld write d <val> 返回失败。
- 该命令没有按寄存器功能进一步限制可写位；除 reg d 之外的寄存器也可以被任意覆盖。
- Web RPC 不得直接映射 do_cpld()，不得开放 reg d，也不得开放 raw register write。
- 如果将来必须增加 CPLD 写操作，应使用专用的语义函数（例如“设置 STAT LED”或“释放 PHY reset”），不要暴露寄存器地址和值。

## 6. 项目适配结论

| 功能 | PA-220 现状 | Web/维护建议 |
|---|---|---|
| 读取 board type/revision/serial | TLV EEPROM + octeon_board_get_descriptor() | 可只读显示；需要 checksum/容量验证 |
| 读取 MAC base/count | TLV EEPROM + octeon_board_get_mac_addr() | 可只读显示；不提供 set |
| 读取 CPU/DDR clock | TLV EEPROM + octeon_board_get_clock_info() | 可只读显示 |
| 写 TLV EEPROM | tlv_eeprom set/delete | 不开放 |
| raw EEPROM write | eeprom write | 不开放 |
| CPLD version/status | cpld display/read | 可只读显示 |
| CPLD LED/PHY control | board init/handoff existing writes | 保留现有流程；新增功能需独立审查 |
| CPLD reg d flash select/reboot | cpld write -f d | 禁止 Web/远程触发 |
| IPQ 9008/button recovery | QCA FDT GPIO/button state machine | 不直接移植；PA 需独立设计 |

## 7. 未完成与安全边界

- TLV EEPROM 实际器件型号、有效容量、0x57 地址映射、1-byte bank addressing 与 24C256 注释的关系 NOT VERIFIED。
- TLV EEPROM tuple parser 的总容量边界、损坏 END marker 行为需进一步审计。
- octeon_boot_bus_board_post_init()、board_prepare_os_handoff()、board_enter_cli() 的完整调用点/时序需继续确认。
- 未执行 eeprom write、tlv_eeprom set/delete、cpld write、CPLD reg d 操作。
- 未进行硬件探测、编译或实机验证。
