# P7-2C 实机验收报告：eMMC DOS/MBR 与分区文件系统只读盘点

- 日期：2026-11-27
- 仓库：`sdzbzyh/uboot-qsdk12.5-build`
- 分支：`feature/pa220-ubootkit-web`
- 设备：PA-220 / U-Boot 2013.07 / Octeon MMC/SD0
- 验收状态：**PASS — live eMMC MBR inventory and read-only filesystem directory probes**
- 范围修正：实机介质是 **DOS/MBR + extended/EBR logical partitions，不是 GPT**。原计划中的 GPT primary/backup consistency 项不适用，不得继续按 GPT 目标验收。

## 1. 设备与 eMMC 身份

2026-11-27 实机串口记录：

- U-Boot：`U-Boot 2013.07 (Build Nov 27 2026 07:27:14)`
- `mmc list`：`Octeon MMC/SD0: 0`
- 当前介质：MMC device 0
- `mmc info`：
  - Name：Octeon MMC/SD0
  - Type：MMC
  - Version：MMC v5.0/v5.01
  - Manufacturer ID：`0x13`
  - OEM ID：`0x4e`
  - Product：`R1J58L`
  - Bus frequency：26,000,000 Hz
  - Bus width：8-bit DDR
  - Read block length：512 bytes
  - Capacity：29,595,009,024 bytes / 57,802,752 blocks
  - Erase group size：1024
  - Pre EOL：Normal
  - Lifetime A/B：0–10

该容量与已归档完整 User Area 镜像大小完全一致。用户本地镜像 `G:\\pa220-mmcblk0.img` 的 SHA-256 为 `9dcb8796e28bb4498db44eeb2e29a29e9345bd5b41cd42eb9f02f7a783c6d5fb`，与仓库 main 分支 `emmc-info.txt` 中记录相同。无需重新上传 29.6 GB 镜像。

## 2. Live 分区表：DOS/MBR

实机 `mmc part` 已成功解析 DOS 分区表。磁盘共 57,802,752 个 512-byte blocks，8 个分区条目如下：

| U-Boot 分区 | Start LBA | Sectors | MBR type | 说明 |
|---|---:|---:|---:|---|
| 0:1 | 16 | 3,952 | 0x83 | primary Linux |
| 0:2 | 3,968 | 8,000,064 | 0x83 | primary Linux |
| 0:3 | 8,004,032 | 8,000,064 | 0x83 | primary Linux |
| 0:4 | 16,004,096 | 41,798,656 | 0x05 | extended container |
| 0:5 | 16,004,112 | 24,000,048 | 0x83 | logical Linux |
| 0:6 | 40,004,176 | 8,000,048 | 0x83 | logical Linux |
| 0:7 | 48,004,240 | 3,952 | 0x82 | logical Linux swap |
| 0:8 | 48,008,208 | 9,794,544 | 0x83 | logical Linux |

实机分区表与完整 eMMC 镜像的离线 `fdisk`、`parted`、`sfdisk` 记录一致：

- `main/emmc-fdisk.txt`
- `main/emmc-parted.txt`
- `main/emmc-sfdisk.txt`
- `main/emmc-info.txt`

`mmc part` 能列出扩展分区中的逻辑分区，证明当前 U-Boot 的 DOS/MBR 分区枚举路径可用。分区 4 是扩展容器，不是可挂载文件系统；分区 7 是 swap。

**格式判定：** 主分区表为 DOS/MBR。不存在本轮需要验证的 GPT primary/backup header，因此 GPT header CRC、entry CRC、主备 GPT 一致性不属于本设备当前分区布局的验收项。

## 3. 文件系统类型与目录探测

离线完整镜像的 `parted` 文件系统识别结果：

| 分区 | 离线识别类型 | Live U-Boot 只读目录探测 |
|---|---|---|
| 0:1 | ext2 | `ext2ls mmc 0:1 /` 成功 |
| 0:2 | ext3 | `ext2ls mmc 0:2 /` 成功 |
| 0:3 | ext3 | `ext2ls mmc 0:3 /` 成功（2026-11-18） |
| 0:4 | extended | 容器，不作为文件系统探测 |
| 0:5 | ext3 | `ext2ls mmc 0:5 /` 成功（2026-11-18） |
| 0:6 | ext3 | `ext2ls mmc 0:6 /` 成功（2026-11-18） |
| 0:7 | Linux swap | 不运行目录列表 |
| 0:8 | ext3 | `ext2ls mmc 0:8 /` 成功（2026-11-18） |

已确认的目录内容摘要：

- 0:1：根目录仅显示 `.`、`..`、`lost+found`。
- 0:2：根目录显示 `.`、`..`、`lost+found`、`dev`、`var`。
- 0:3：根目录包含 Linux 根文件系统目录，历史实机输出明确包含 `usr`、`var`、`boot`、`etc` 等。
- 0:5：根目录包含 `cfgdb`、`mgmt`、`certificates` 等设备管理数据目录。
- 0:6：根目录包含 `images`、`base`、`releases` 等镜像/版本相关目录。
- 0:8：根目录包含 `cores`、`logdb` 等运行记录目录。

2026-11-27 本轮再次执行 0:1 与 0:2 的目录读取，输出与既有布局相符。0:3、0:5、0:6、0:8 的实机目录读取已于 2026-11-18 完成；本报告复用历史串口结果，不要求重复执行。

这些结果证明 U-Boot 的只读 ext2 目录访问路径能够读取相应分区的根目录；不等同于全盘文件系统 fsck、journal replay、文件内容哈希或完整目录树审计。

## 4. 备份镜像与 live 设备交叉核对

完整 User Area 镜像：

- 长度：29,595,009,024 bytes
- SHA-256：`9dcb8796e28bb4498db44eeb2e29a29e9345bd5b41cd42eb9f02f7a783c6d5fb`
- 分区表：DOS/MBR
- 离线分区起始 LBA、长度、类型与 live `mmc part` 输出一致。

仓库 main 分支已保存分区盘点文本，不需要将完整二进制镜像复制到集成分支。离线报告中的文件系统创建/挂载/写入时间只代表镜像采集时记录的元数据，不作为当前运行状态或设备角色的单独证明。

## 5. 证据边界与未做事项

已完成：

- live eMMC 设备识别、容量和基础健康字段读取；
- live DOS/MBR 分区表枚举，包括扩展分区中的逻辑分区；
- 主要 ext2/ext3 分区根目录只读访问；
- live 分区布局与同一设备 User Area 完整镜像的离线分区表交叉核对。

未执行/不声称：

- 未读取并归档 MBR/每个 EBR 的原始 512-byte sector dump 或逐扇区 SHA-256；
- 未做 EBR 链条原始字节级一致性审计；
- 不存在 GPT，因此未做 GPT primary/backup header/entry-array 一致性验证；
- 未做文件系统修复、journal replay、fsck 或任何写操作；
- 未建立固件组件到 eMMC 分区的安装映射。

因此，P7-2C 的正确交付是 **live DOS/MBR + logical partition inventory accepted**；所有 eMMC 固件安装目标仍为 `UNMAPPED`。P8 写入事务仍需独立设计、备份/恢复审查和专项授权。

## 6. 写入安全边界

本轮及历史目录探测均为只读操作。没有执行：

- `mmc write` / `mmc erase`
- `gpt write`
- `ext4write` / `fatwrite`
- `saveenv`
- SPI/NAND updater、擦除或写入命令

`saveenv` 在本板配置中会写 SPI NOR redundant environment，不能视为 RAM-only 操作。
