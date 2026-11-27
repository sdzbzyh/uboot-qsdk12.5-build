# PA-220 P7 计划：固件解析与安装预览（dry-run only）

- 日期：2026-11-27
- 固定源码：`sdzbzyh/pa220-openwrt@801717ba6a8a59ed2cfd53bf3e1b19841fecd2e8`
- 集成分支：`feature/pa220-ubootkit-web`
- 当前范围：只读解析、兼容性检查和安装目标预览；P7 不执行写入。

## 总原则

1. 只解析已完成接收且 `fileaddr/filesize` 元数据有效的 RAM 文件。
2. 先用 `octeon_load_range_check()` 重新确认文件区间属于有效 `__tmp_load` reservation。
3. 解析只返回格式、边界、校验、架构/板型信息及候选 segment；不调用任何 image load、boot、flash、MMC、NAND、GPT 写入或环境保存路径。
4. 目标映射必须来自 PA-220 固定源码、实测分区表或可核验的固件 manifest。证据不足时明确输出 `UNMAPPED`，不得借用 IPQ/QCA 的 HLOS/rootfs/ART/CDT 分区名称。
5. P7 dry-run 的成功不等于安装授权。P8 eMMC 和 P10 SPI 仍是独立风险阶段。

## 分区表兼容性要求（持续有效）

后续所有介质识别、分区枚举、固件目标映射和 dry-run 计划必须同时兼容 DOS/MBR 与 GPT；不得将当前 PA-220 eMMC 的 MBR 事实硬编码成所有设备/镜像的唯一格式，也不得因固件编译了 GPT 支持就假定实际介质为 GPT。

- **自动识别**：读取介质实际分区表签名和结构后选择解析路径；识别结果必须携带来源格式（`DOS/MBR` 或 `GPT`）。格式不明、混合/冲突或结构损坏时拒绝生成目标映射。
- **DOS/MBR**：解析 4 个 primary entries；对 extended 类型沿 EBR 链枚举 logical partitions；检查 512-byte sector 范围、起始/长度溢出、容器边界、EBR 自环/循环和越界。Extended container 不作为 filesystem target。
- **GPT**：检查 Protective MBR、主 Header 和 Entry Array CRC、revision/header size、current/backup LBA、usable LBA bounds、disk GUID、entry-array bounds/CRC；读取备份 Header/Entry Array 并比较关键字段与条目内容。不能只因 `CONFIG_EFI_PARTITION` 存在就声称 GPT 验证通过。
- **混合/损坏情况**：Hybrid MBR、MBR/GPT 签名冲突、主备 GPT 不一致、CRC 错误、重复/重叠/越界条目必须显式分类为 ambiguous/corrupt；只报告，不自动修复或写入。
- **统一表示**：将两种格式映射到同一只读分区描述（source format、index、start LBA、sector count、type/GUID、name、attributes、可用时的 UUID），上层 FIT/镜像预览和目标映射只消费该统一表示。
- **读写隔离**：所有验证器只允许 block read；`gpt write` 是写入/恢复路径，不是验证命令。当前 PA-220 live eMMC 已确认 MBR；未来 GPT 设备必须另有 GPT fixture/介质验证，不能把本机 MBR 结果当作 GPT 验收证据。

## P7-1：只读镜像解析入口

生产入口：`pa220 image preview`

当前解析器：`u-boot/board/octeon/pa220/pa220_image.c`

- OCTEON bootloader header：识别 magic（header offset 8）、header/data 长度、版本、image type、board type、link address；执行有界 header CRC 和 payload CRC 校验；限定 header 192..512 bytes、总镜像不超过 2 MiB。
- Legacy uImage：要求 64-byte header、声明 payload 长度与文件长度完全一致；校验 header CRC 和 data CRC；报告 load/entry 与 MIPS 架构候选。
- FIT/FDT：当前只验证 FDT header、总长度、结构/字符串/reservation offsets 的边界；尚不解析 FIT configurations、子镜像、hash/signature 语义。
- ELF32/ELF64：验证 ELF header、program/section table 边界；检查 PT_LOAD 的 file/memory 长度、文件区间和地址加法溢出；最多展示前 8 个 segment，不写入或执行。
- 所有格式的 target mapping 当前均为 `UNMAPPED`，因为尚无经过核验的 PA-220 install manifest。
- 命令只读取 `fileaddr/filesize`，重新检查 RAM reservation 并 flush cache；没有设备分区读取或写入。

**验收门：**
- Host GCC + ASan/UBSan 格式向量通过。
- Pinned Cavium GCC 4.7.0 OCTEON 对象/完整 U-Boot 链接通过，最终 ELF 保留 `pa220_image_preview` 和 `pa220_image_parse`。
- 随后执行 RAM-only 实机验证：
1. 使用 Actions artifact 中的 `u-boot-octeon_pa220-stage3-wrapper.uImage`。它是合法 legacy uImage 外壳，payload 是本轮构建的原始 OCTEON Stage3 BIN。
2. 旧 U-Boot 将 wrapper TFTP GET 到 `0x20000000`（有效 `__tmp_load` 范围）；从 wrapper offset `0x40` 将 payload 用 `cp.b` 复制到已验证的 RAM 启动地址 `0x81000000`，用 `cmp.b` 校验后执行 `go 0x81000000`。这是 RAM 启动，不是 flash 写入。
3. 新 P7 U-Boot 再次 TFTP GET 同一 wrapper，运行 `pa220 image preview` 验证 uImage header/data CRC。
4. 将 wrapper 内层 OCTEON Stage3 payload 复制到 `__tmp_load` 内的另一个 RAM 地址（例如 `0x20200000`），临时设置 `fileaddr/filesize`，再次运行 `pa220 image preview`，验证 OCTEON header、board type 和 CRC。
5. 检查 `pa220 memory`，确认 descriptor 不变；不执行 `saveenv`、bootloader updater 或任何介质写入。

注意：P6-1C 当前 TFTP GET recognizer 接受 legacy uImage、FIT/FDT、ELF32/ELF64；OCTEON bootloader header 虽已纳入 P7 parser，但尚未纳入 P6-1C 接收格式门。因此当前 TFTP GET 路径不能接收 OCTEON bootloader-header 文件。若后续需要经 TFTP 预览该格式，必须单独扩展并测试 P6 verifier，不能假定本次 P7 parser 已使其可接收。

## P7-2：容器语义与固件布局取证

### P7-2A：SPI NOR 原机备份静态取证 — PASS

- 报告：`reports/pa220/P7-2/SPI-IMAGE-ANALYSIS.md`。
- 原始镜像只从仓库 `main@da755efd49df32f3e85d231745212674737feba5` 读取；16 MiB，SHA-256 `4899a1c5ed06e5eafbb8c92625a4afae14cad399806df13979fd2c70ee1ed922`。
- 4 个 OCTEON header 的 header/data CRC 均通过：Pre-U-Boot 两份（offset 0、0x1d80）、Stage 2（0x10000）、NOR bootloader（0x100000）。
- 发现两个嵌入 FDT：Stage2 generic SPI tree、PA-220 Stage3 tree；后者描述 W25Q128-compatible SPI、OCTEON MMC host 与 Kingfisher CPLD。
- `0x70000-0x100000` 与 `0x250000-0x1000000` 为全 `0xff` 区域；配置环境 sector `0xe00000` / `0xe10000` 在该备份中也全擦除。
- 这只是 SPI NOR 证据，不提供 eMMC GPT/分区布局，也不授权写入。

### P7-2B：FIT 语义只读解析 — TARGET BUILD PASS；实机语义验收待执行

- 生产代码已在 `u-boot/board/octeon/pa220/pa220_image.c` 加入 `/images`、`/configurations`、default configuration、kernel/ramdisk/fdt 引用、inline `data` 和 hash 子节点预览。
- 严格区分结构合法、数据 hash PASS/FAIL/IGNORED/UNSUPPORTED 和签名可信状态。
- 固定源码仅支持 inline `data` 的 `fit_image_get_data()`；外置数据 `data-offset/data-position` 显式标记 `EXTERNAL_DATA_UNSUPPORTED`。
- 当前 PA-220 配置未启用 `CONFIG_FIT_SIGNATURE`；不能把 FIT signature 节点存在误报为已验证签名。
- 不调用 `fit_image_load()`、bootm 或任何会把子镜像复制到 load 地址的路径。

### P7-2C：eMMC DOS/MBR 实机只读取证 — PASS（范围已纠正）

- 实机报告：`reports/pa220/P7-2/EMMC-DEVICE-ACCEPTANCE.md`。
- 设备为 Octeon MMC/SD0（device 0），eMMC R1J58L，MMC v5.0/v5.01，8-bit DDR，容量 29,595,009,024 bytes / 57,802,752 blocks，Pre EOL Normal，lifetime A/B 0–10。
- `mmc part` 实机识别 DOS/MBR，不是 GPT；8 个条目包括 3 个 primary Linux、1 个 extended container、3 个 logical Linux 和 1 个 logical swap。Start LBA/sector count 与同一 User Area 完整镜像离线 fdisk/parted/sfdisk 记录一致。
- Live ext2ls 根目录读取已覆盖 0:1、0:2（2026-11-27）及 0:3、0:5、0:6、0:8（2026-11-18）；0:4 是扩展容器，0:7 是 swap，不按文件系统目录探测。
- 原计划的 GPT primary/backup header/entry consistency 不适用；未读取并归档原始 MBR/EBR sector dump，因此不声称逐扇区 EBR 链审计完成。
- 所有 eMMC 固件安装目标继续为 `UNMAPPED`。不执行 `gpt write`、`mmc write/erase`、filesystem write、`saveenv` 或 updater。

## P7-3：dry-run 安装计划

- 将已验证的镜像组件映射到有证据支持的目标分区/区域。
- 输出来源 offset/length、目标设备/分区、目标 offset/容量、覆盖范围、校验结果、未决项和明确拒绝原因。
- 对未知、歧义、越界、容量不足、签名策略未满足的项目拒绝生成可执行计划。
- 只生成报告；不调用任何写入函数。

## 当前状态

- P7-1 parser、Host Harness、ASan/UBSan、Actions OCTEON 完整构建及真实 PA-220 实机 preview 均已通过；云端证据见 `reports/pa220/P7-1/ACCEPTANCE.md`，实机串口证据见 `reports/pa220/P7-1/DEVICE-ACCEPTANCE.md`。
- 实机已验证 wrapper TFTP 接收、uImage 校验、payload RAM-copy/compare、RAM Stage3 启动、外层 legacy uImage preview、内层 OCTEON bootloader preview 和 reservation descriptor 不变。
- P7-1 验收完成。P7-2A SPI NOR 静态取证与 P7-2C eMMC DOS/MBR 实机只读取证已完成。P7-2B FIT preview 已通过 Actions #45970127218 / #45970607698 的 Host/ASan/UBSan、Cavium OCTEON object、N32 relocatable link 和完整 U-Boot final link；FIT fixtures 已打包在 [Artifact 15184393205](https://github.com/sdzbzyh/uboot-qsdk12.5-build/actions/runs/45970607698/artifacts/15184393205)。尚未做 PA-220 实机 FIT 语义输出验收，不能标记 P7-2B PASS。P7 整体仍未完成。
