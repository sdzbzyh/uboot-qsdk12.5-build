# P1-A4：存储后端审计（第二批：GPT 分区与 EXT4/FAT 文件系统）

- 状态：**本批源码静态审计完成；P1-A4 整体进行中；未编译、未实机验证**
- 日期：2026-11-18
- uBootKit：chenxin527/uboot-qsdk12.5-build@9ce315735ac88fd0a7c7697bc527a8616ed0a972
- PA-220：sdzbzyh/pa220-openwrt@801717ba6a8a59ed2cfd53bf3e1b19841fecd2e8
- 前置：P1-A4-EMMC-SPI-CORE.md
- 范围：cmd_gpt.c、disk/part_efi.c、cmd_ext4.c、ext4_write.c、cmd_fat.c、fat_write.c；对照 PA-220 固定源码和配置。

## 1. GPT 分区表

### 1.1 PA-220 构建配置
- octeon_pa220.h 启用 CONFIG_PARTITION_UUIDS、CONFIG_EFI_PARTITION、CONFIG_CMD_PART、CONFIG_CMD_GPT。
- GPT 命令在 common/cmd_gpt.c；GPT parser/reader/writer 在 disk/part_efi.c。
- PA-220 GPT 设备是 eMMC block device，接口为 block_dev_desc_t + block_read/block_write。
- 这些配置说明源码构建入口存在，不代表已在设备上执行或验证。

### 1.2 PA-220 gpt 命令
- common/cmd_gpt.c:20-180 的 extract_env()/extract_val()/set_gpt_info() 解析 disk GUID 和分区描述字符串。
- 分区字段包括 uuid、name、size、可选 start；size/start 以字节解析后除以 block size 转换为 LBA。
- common/cmd_gpt.c:220-270 的 gpt_mmc_default() 找到 mmc 设备、解析布局，然后调用 gpt_restore()。
- **错误处理缺陷：** gpt_mmc_default() 调用 gpt_restore() 后没有检查其返回值，始终释放内存并返回 0；底层 GPT 写失败可能被上层报告成功。
- do_gpt()（约 274-317）只支持 `gpt write mmc <dev> <partitions_list>`；不提供 uBootKit 新版的 gpt verify 子命令。
- PA-220 的 gpt write 不是只读检查，是直接重建并写入分区表。

### 1.3 PA-220 gpt_restore() 写入顺序
- disk/part_efi.c:293-344 的 write_gpt_table()：
  1. set_protective_mbr() 写 LBA0 protective MBR；
  2. block_write() 写主 GPT header 到 LBA1；
  3. block_write() 写主 GPT entry array 从 LBA2 开始；
  4. block_write() 写备份 GPT entry array 到磁盘末尾；
  5. block_write() 写备份 GPT header。
- disk/part_efi.c:446-480 的 gpt_restore() 分配 GPT header/entry buffer、填充 header 和 entries，再调用 write_gpt_table()。
- 这些写入直接进入 eMMC block device；中途失败可能留下 primary/backup GPT 不一致，没有事务回滚。
- set_gpt_info() 的字符串解析、大小换算和分区布局需进一步检查整数溢出、分区重叠、越过设备末尾、未对齐大小和 malloc/calloc 失败处理；不能仅凭 GPT CRC 生成成功认定布局安全。

### 1.4 uBootKit 对照
- uBootKit common/cmd_gpt.c 提供 write 和 verify 两类操作；verify 可比较 GPT header/entries 和预期布局。
- PA-220 旧版仅提供 write，且忽略 gpt_restore() 返回值；若要适配，应先补齐只读 GPT 检查和错误传播，不应直接复用旧 write 命令。
- 两版 disk/part_efi.c 都使用 block_dev_desc_t、block_write，GPT 核心算法有较高结构相似度；但 PA-220 2013.07 版本更旧，不能直接替换整文件。
- 适配判断：只读 GPT 读取/校验可借鉴现有 PA-220 part_efi.c；写入功能不进入 Web/自动升级流程。

## 2. EXT4 文件系统

### 2.1 配置和命令
- PA-220 octeon_pa220.h 定义 CONFIG_CMD_EXT2、CONFIG_CMD_EXT4、CONFIG_CMD_EXT4_WRITE。
- common/cmd_ext4.c 在 CONFIG_CMD_EXT4_WRITE 下编入 ext4write；ext4ls/ext4load 为读路径。
- fs/ext4/Makefile 以 CONFIG_FS_EXT4 编入 ext4fs.o/ext4_common.o/dev.o，以 CONFIG_EXT4_WRITE 编入 ext4_write.o/ext4_journal.o/crc16.o。
- 最终 .config 中 CONFIG_FS_EXT4/CONFIG_EXT4_WRITE 的展开结果尚未读取，编译状态 NOT VERIFIED。

### 2.2 PA-220 EXT4 写入调用链
- common/cmd_ext4.c:56-109 的 do_ext4_write() 解析设备/分区、RAM 地址、文件名、size，调用 ext4fs_set_blk_dev()、ext4fs_mount()，最后调用 ext4fs_write()。
- 参数地址和长度通过 simple_strtoul() 解析；命令层未见对 RAM 源缓冲区的完整地址范围检查。
- fs/ext4/ext4_write.c:837-995 的 ext4fs_write()：
  - 若目标文件已存在，先调用 ext4fs_delete_file() 删除旧文件；
  - 检查剩余空间；
  - 更新父目录项并分配 inode/data blocks；
  - 通过 ext4fs_put_metadata()/ext4fs_write_file() 写 inode 和文件内容；
  - 调用 ext4fs_update() 写回 journal、superblock、block bitmaps、inode tables、group descriptor。
- ext4fs_update()（约 30-58）通过 put_ext4() 写回所有关键元数据；put_ext4() 最终使用 block_write。
- 这是 eMMC 文件系统持久化写入，不是仅写入新文件数据；它会更新多个元数据结构。
- 覆盖已有文件时先删除旧文件，之后新文件写入若失败可能留下文件丢失/文件系统不一致状态；没有原子临时文件+rename 回滚流程。

### 2.3 uBootKit EXT4 API 差异
- 两边 ext4fs_write() 均存在，但 read/open/size API 已有版本差异：
  - PA-220 ext4fs_open() 返回文件长度，ext4fs_read(buf, len) 使用旧签名；
  - uBootKit ext4fs_open(filename, &file_len)、ext4fs_read(buf, len, &actread)、ext4fs_size() 使用较新的接口。
- uBootKit common/cmd_ext4.c 将命令接入 generic fs helper；PA-220 使用旧 ext4fs_set_blk_dev()/mount/read/write 路径。
- 因此只读页面/文件操作不能直接复制新版 FS API；应以 PA-220 现有 EXT4 实现为底层。

## 3. FAT 文件系统

### 3.1 配置和命令
- PA-220 octeon_pa220.h 定义 CONFIG_CMD_FAT 和 CONFIG_FAT_WRITE。
- common/cmd_fat.c 在 CONFIG_FAT_WRITE 下编入 fatwrite；fatload/fatls/fatinfo 为读路径。
- fs/fat/Makefile 在 CONFIG_FS_FAT 下编入 fat.o/file.o，在 CONFIG_FAT_WRITE 下编入 fat_write.o。
- 最终 .config 的 CONFIG_FS_FAT 状态仍需构建确认。

### 3.2 PA-220 FAT 写入调用链
- common/cmd_fat.c:86-136 的 do_fat_fswrite() 通过 get_device_and_partition() 获取分区，fat_set_blk_dev() 绑定 block device，调用 file_fat_write(filename, addr, count)。
- fs/fat/fat_write.c:31-43 的 disk_write() 调用 cur_dev->block_write()；其边界检查使用 cur_part_info.start + block + nr_blocks 与分区末端比较，需审查加法溢出。
- file_fat_write()（1077 起）进入 do_fat_write()，查找或创建目录项，分配 cluster，写文件内容，flush FAT buffer，再写目录表。
- set_cluster()（约 548-575）使用 disk_write() 将数据写入分区；flush_fat_buffer() 写入 FAT 表（有两份 FAT 时分别更新）。
- 这是 eMMC 文件系统持久化写入；更新内容包括文件数据、FAT 表和目录项。
- 覆盖现有文件时会清理旧 FAT cluster；写入失败可能造成旧文件数据/目录状态丢失。
- PA-220 file_fat_write() 只支持整文件写入，无 offset 参数；uBootKit 新版接口为 file_fat_write(filename, buffer, offset, maxsize, actwrite)，两者 ABI 不同。

### 3.3 文件系统适配边界
- EXT2/EXT4/FAT 的读取路径可用于加载 eMMC 上的 PA-220 kernel/initramfs 文件；现有 boot command 已使用 ext2load。
- ext4write/fatwrite 会修改 eMMC 文件系统，不应由 Web 或升级流程自动调用。
- uBootKit 的 UBI/UBIFS/JFFS2/NAND 文件系统能力与 PA-220 的 eMMC GPT + EXT2/EXT4/FAT 布局不同；PA-220 没有 QCA NAND/SMEM 机制，不能按 IPQ rootfs 分区流程移植。
- PA-220 固定源码配置启用 CONFIG_CMD_CRAMFS（只读）、EXT2/EXT4/FAT 命令；本批没有确认 UBIFS/UBI/NAND 文件系统在 PA-220 的实际编入。
- 对 PA-220 的 Web 维护初版，只建议只读列目录/读取受限文件；任何 ext4write/fatwrite 均需独立授权和备份流程。

## 4. 本批结论矩阵

| 功能 | PA-220 源码/配置 | 适配结论 | 持久化风险 |
|---|---|---|---|
| GPT read/verify | EFI_PARTITION、PARTITION_UUIDS、CMD_GPT；旧 part_efi.c | 可复用只读读取；补只读 verify | 无写时只读 |
| GPT write | gpt write + gpt_restore + block_write | 不开放 Web；先修复错误传播和布局验证 | 写 LBA0/1/2/盘尾 GPT |
| EXT2/EXT4 read | ext2load/ext4load、ext4fs_read | 复用 PA-220 旧 FS | RAM 写入目标地址 |
| EXT4 write | CONFIG_CMD_EXT4_WRITE，ext4fs_write/put_ext4 | 不开放 Web | 写文件数据、inode、bitmap、superblock、journal |
| FAT read | fatload/fatls/fatinfo | 复用 PA-220 旧 FS | RAM 写入目标地址 |
| FAT write | CONFIG_FAT_WRITE，file_fat_write | 不开放 Web | 写文件数据、FAT 表、目录项 |

## 5. 本批未完成

- 尚未审计环境变量 SPI 后端与 saveenv 主/冗余区完整错误处理；A1/A2 已确认写入行为，本阶段仍需检查底层实现细节。
- 尚未审计固件格式、镜像分区映射和 bootloader update 的 PA-220 专用路径。
- 未执行 GPT write、mmc erase/write、ext4write、fatwrite、sf write/erase/update、saveenv。
- 未编译或实机验证。
