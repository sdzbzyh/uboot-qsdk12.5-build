# P1-A4：存储介质审计（第四批：USB Mass Storage 与额外存储命令边界）

- 状态：**本批源码静态审计完成；P1-A4 源码审计完成；未编译、未实机验证**
- 日期：2026-11-18
- PA-220 固定源码：sdzbzyh/pa220-openwrt@801717ba6a8a59ed2cfd53bf3e1b19841fecd2e8
- 范围：octeon_common.h USB 配置、common/cmd_usb.c、drivers/usb/host 编入关系、cmd_octeon_eraseenv 编译条件。

## 1. USB Mass Storage 配置

- octeon_pa220.h:22 定义 CONFIG_OCTEON_USB_OCTEON3。
- octeon_common.h:433-447 在 OCTEON USB 支持下定义 CONFIG_CMD_USB、CONFIG_USB_STORAGE、CONFIG_USB_EHCI_OCTEON2。
- octeon_common.h:455-458 配置 CONFIG_USB_XHCI 和 CONFIG_USB_XHCI_OCTEON（按板级 USB 选项条件）。
- drivers/usb/host/Makefile 按 CONFIG_USB_EHCI_OCTEON2、CONFIG_USB_XHCI_OCTEON 编入 OCTEON host controller。
- common/cmd_usb.c 在 CONFIG_USB_STORAGE 下编入 mass-storage 命令路径。
- 这说明 PA-220 源码有 USB host/mass-storage 能力；是否在实际硬件端口和最终 .config 工作，尚未验证。

## 2. USB 命令读写路径

文件：common/cmd_usb.c（704 行）。

- usb start/reset：调用 usb_init()，扫描 USB storage device 并设置 usb_stor_curr_dev。
- usb stop：调用 usb_stop()。
- usb stor：调用 usb_stor_info() 显示 USB mass-storage 信息。
- usb part：通过 usb_stor_get_dev() 获取 block_dev_desc_t，调用 print_part() 读取分区表。
- usb read：解析 addr/blk/cnt，调用 stor_dev->block_read() 将 USB block device 数据读入 RAM。
- usb write：解析 addr/blk/cnt，调用 stor_dev->block_write() 将 RAM 数据写入当前 USB mass-storage device。
- usb dev：选择当前 USB storage device。
- usb read 不修改 USB 持久化介质；usb write 是对外接 USB 块设备的持久化写入。
- 命令层使用 simple_strtoul() 解析地址/LBA/块数，没有完整的 RAM 地址区间检查；设备容量和块数检查依赖底层 block driver。
- USB mass storage 与 PA-220 内部 eMMC/SPI NOR 是不同介质；不能把 USB write 误判为内部 SPI/eMMC 写入，但它仍然会改变外接 USB 存储内容。

## 3. 与内部存储和 Web 维护的关系

- PA-220 已有 eMMC GPT、EXT4/FAT、SPI NOR 和 USB mass-storage 代码入口。
- 若使用 USB 作为离线恢复/备份介质，优先复用现有 USB block device + FAT/EXT4 只读读取能力。
- usb write/fatwrite/ext4write 均属于持久化修改；Web 初版不提供通用块写或文件系统写接口。
- USB 设备可被拔插或替换，不能将其当作可信固件来源；任何升级镜像仍需独立校验 board type、长度、CRC/签名、版本和目标分区。
- 未进行 USB 设备扫描、USB 读写或外接介质实测。

## 4. 环境擦除命令边界

- arch/mips/cpu/octeon/Makefile 仅在 CONFIG_CMD_OCTEON_ERASEENV 下编入 cmd_octeon_eraseenv.o。
- octeon_cmd_conf.h 中 CONFIG_CMD_OCTEON_ERASEENV 的定义受 CONFIG_ENV_IS_IN_FLASH 条件控制。
- PA-220 使用 CONFIG_ENV_IS_IN_SPI_FLASH，而非 CONFIG_ENV_IS_IN_FLASH；因此当前 PA-220 固定配置链不启用这个 parallel-NOR eraseenv 命令。
- 不能将该命令误认为 PA-220 SPI 环境区的清除接口；PA-220 saveenv 的 SPI redundant backend 已在 P1-A4-ENV-SPI-BACKEND.md 中审计。
- 不执行 eraseenv 或任何环境擦除操作。

## 5. P1-A4 最终结论

- 已审计：OCTEON eMMC 驱动/块接口、SPI NOR probe/read/write/erase、sf/mmc 命令、GPT、EXT4/FAT、SPI redundant environment/saveenv、OCTEON bootloaderupdate、USB mass storage。
- QCA SMEM/MIBIB/NAND/UBI/ART/CDT/bootconfig 写入路径已在 P1-A2 中逐项审计，并判定不适用于 PA-220。
- PA-220 只读访问基础：OCTEON MMC block read、SPI flash read（需确认 bus/CS）、GPT/EXT4/FAT 读取、USB storage block read。
- PA-220 持久化写入口：saveenv、sf write/erase/update、mmc write/erase/EXT_CSD configuration、gpt write、ext4write、fatwrite、usb write、bootloaderupdate、TLV EEPROM write（TLV EEPROM 细节转 P1-A5）。
- P1 阶段禁止执行以上持久化写入；本阶段未做任何内部/外部存储写操作。
- 所有构建和实机功能状态仍 NOT VERIFIED。
