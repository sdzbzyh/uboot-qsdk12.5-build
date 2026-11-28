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

### P7-2B：FIT 语义只读解析 — PASS（目标构建 + 正常/损坏双向量实机验收）

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

## P7-3：dry-run 安装计划（进行中；尚未验收）

源码审查已发现不能直接复用固定 U-Boot 的普通分区枚举作为 P7 安全边界：DOS/EBR 路径使用有符号 int 做 sector arithmetic，递归遍历缺少 EBR visited-set/hop bound 与 extended-container 完整约束；EFI/GPT 路径只验证调用方指定的单份 Header/Entry Array，现有调用只传 primary GPT LBA，未交叉验证 backup Header/Entry Array。详细审查：`reports/pa220/P7-3/SOURCE-AUDIT.md`。

### P7-3A：bounded read-only partition inventory core

- 独立实现 sector-read callback 驱动的 MBR/EBR 与 GPT 只读解析器，不调用 block-write API。
- MBR：checked 64-bit arithmetic、4 primary entries、0x05/0x0f/0x85 extended、EBR visited-set/hop bound、extended-container containment、循环/重叠/越界/多 extended 容器 fail closed。
- GPT：Protective MBR、主/备 Header 与 Entry Array CRC、几何边界、disk GUID/关键字段一致性、完整 Entry Array 一致性、partition GUID 唯一性、范围与重叠检查。
- 自动识别；hybrid、损坏、冲突、未知格式拒绝。
- Host 合成 MBR/GPT fixture + GCC/ASan/UBSan；随后 pinned Cavium OCTEON object/N32/full link。

**P7-3A implementation checkpoint (2026-11-27):** Core files `u-boot/board/octeon/pa220/pa220_partition.c` and `u-boot/include/pa220_partition.h` are now present. Actions #45985125302 passed Host GCC, ASan/UBSan, and pinned Cavium GCC 4.7.0 target-object compilation; Artifact 15191546135 archives evidence. The test harness includes the observed PA-220 8-entry MBR/EBR layout using sparse sectors. Expanded 16-vector Host/ASan/UBSan/Cavium-object suite passed Actions #45985342164; subsequent extended read-error and GPT backup-header vectors also passed Actions #45986937723. The parser is now integrated into the production board Makefile and read-only `pa220 partition scan [mmc-device]` command. Actions #45986937723 completed a full U-Boot final link successfully (Artifact 15190844437), but emitted one maybe-uninitialized warning; commit `39ab6a789699346a0c0c6620c021d4fb8b390cd7` fixes it and clean rebuild Actions #45987282068 completed SUCCESS with no parser-specific warnings. Latest index-corrected build and RAM wrapper are in Actions #45988344341, Artifact 15192058990; payload size 0x155b48, wrapper size 0x155b88. The sparse fixture checks all seven data partitions and on-disk indices 1,2,3,5,6,7,8. P7-3A is now **ACCEPTED**: production command/Makefile integration, clean full U-Boot final link, Host/sanitizer/target validation, and PA-220 live read-only inventory all passed. The 2026-11-28 live output matched all seven expected data partitions and their on-disk indices. Device evidence: `reports/pa220/P7-3/DEVICE-ACCEPTANCE.md`. Details: `reports/pa220/P7-3/IMPLEMENTATION-PROGRESS.md`.

### P7-3B：manifest policy and dry-run plan

- 仅接受显式审核过的 PA-220 install manifest；不根据 MBR type 0x83、目录名、FIT image name 或 load/entry 地址猜测分区角色。
- 当前没有 approved PA-220 install manifest，因此所有组件必须保持 `UNMAPPED`，报告硬拒绝原因 `NO_APPROVED_MANIFEST`。
- 输出 source offset/length、组件校验状态、分区表来源、候选目标/容量（只有 manifest 可解析时）、范围/重叠检查、未决证据和最终拒绝原因。
- 只生成 dry-run 报告，不生成可执行写入脚本，不调用任何写入函数。


**P7-3B implementation checkpoint (2026-11-28; software acceptance pending):** Added the pure planner core in `u-boot/board/octeon/pa220/pa220_install_plan.c` and public types in `u-boot/include/pa220_install_plan.h`. It requires an approved, board-matched manifest; verifies component format/integrity/source bounds; requires one explicit user selection per component; enforces manifest-authorized MBR type or GPT type GUID, target-kind policy, partition capacity/device bounds, unique targets and non-overlapping source ranges. No automatic partition choice exists. Host fixture covers MBR and GPT, absent/unreviewed manifest rejection, approved synthetic policies, explicit selections, integrity, overflow, overlap, capacity and board/type mismatch. The production command `pa220 install dry-run [mmc-device]` now validates the completed RAM image against the live RAM reservation, obtains the read-only partition inventory and calls the planner with no approved manifest; expected production result is `NO_APPROVED_MANIFEST`, `UNMAPPED`, `actions=NONE`. It exposes no write API. Actions #46112768625 passed Host GCC + ASan/UBSan and a full pinned Cavium U-Boot final link; Artifact 15250561646 contains the binary, ELF and logs. Subsequent target-capacity hardening and the expanded 15-assertion suite passed Actions #46113003495: Host GCC and ASan/UBSan each report 15 assertions / 0 failures, and the pinned Cavium SDK complete U-Boot final link contains both `pa220_install_plan_build` and `pa220_install_dry_run_cmd`. Artifact 15249864977. The follow-up wrapper build also passed Actions #46113156556; Artifact 15250271914 contains the final BIN and RAM Stage3 wrapper (payload SHA-256 `3144ea1e15fd1f74464908f09ee4feeaaae62f6252a3401d2dc35dedd197d23f`; wrapper SHA-256 `ecc0ba7d35c697c11adbaa9f86e8eda2b52a9113c616b5bf02f6891714b471d8`). Full evidence and the exact next device round are in `reports/pa220/P7-3/P7-3B-IMPLEMENTATION.md`. Do not mark P7-3B fully accepted until the device returns `NO_APPROVED_MANIFEST` with MBR inventory still read-only and the original eMMC boot path is separately confirmed. The approval state is a trusted internal assertion only: production code must set it solely after matching a reviewed manifest against a firmware-pinned digest, never from an untrusted manifest field.

### P7-3 验收门

- MBR/GPT 独立正向与损坏 fixture 覆盖报告中的边界矩阵；ASan/UBSan 无错误。
- 固定 SDK 目标编译、N32 relocatable link、完整 U-Boot final link 通过。
- 对当前 PA-220 仅在软件门通过后进行必要的只读分区 inventory；不要求重复已完成的 `mmc part`/ext2ls 目录探测。
- 在有批准 manifest 之前，dry-run 结果必须为拒绝映射；P7 不写 eMMC/SPI/NAND、不调用 `saveenv`。

## 当前状态

- P7-1 parser、Host Harness、ASan/UBSan、Actions OCTEON 完整构建及真实 PA-220 实机 preview 均已通过；云端证据见 `reports/pa220/P7-1/ACCEPTANCE.md`，实机串口证据见 `reports/pa220/P7-1/DEVICE-ACCEPTANCE.md`。
- 实机已验证 wrapper TFTP 接收、uImage 校验、payload RAM-copy/compare、RAM Stage3 启动、外层 legacy uImage preview、内层 OCTEON bootloader preview 和 reservation descriptor 不变。
- P7-1 验收完成。P7-2A SPI NOR 静态取证与 P7-2C eMMC DOS/MBR 实机只读取证已完成。P7-2B FIT preview 已通过 Actions #45970127218 / #45970607698 的 Host/ASan/UBSan、Cavium OCTEON object、N32 relocatable link 和完整 U-Boot final link；FIT fixtures 已打包在 [Artifact 15184393205](https://github.com/sdzbzyh/uboot-qsdk12.5-build/actions/runs/45970607698/artifacts/15184393205)。PA-220 已完成正常与损坏两个 FIT fixture 的 RAM-only 实机语义验收，P7-2B PASS。正常 fixture 的 kernel/ramdisk CRC32、SHA1、MD5 均 PASS，ignored/unsupported/signature/external-data 状态正确，default configuration 三项引用全部 RESOLVED，hash_failures=0；损坏 fixture 的 kernel 三种 hash 均 FAIL、ramdisk 三种 hash 均 PASS，hash_failures=1。两次均 target mapping UNMAPPED、actions NONE，未执行加载/启动或持久化写入。完整实机证据见 `reports/pa220/P7-2/FIT-IMPLEMENTATION.md`。P7-2A、P7-2B、P7-2C 均已完成。P7-3 源码审计完成；P7-3A parser 与只读 `pa220 partition scan` 命令已集成；Host GCC、ASan/UBSan、Cavium GCC 4.7.0 object、完整 U-Boot final link 在 Actions #45987282068 均 PASS（Artifact 15192027629）。P7-3A live read-only inventory 已于 2026-11-28 验收通过，报告见 `reports/pa220/P7-3/DEVICE-ACCEPTANCE.md`；P7-3B 已进入实现：planner core、显式目标选择策略与只读 dry-run 命令已集成，Host/ASan/UBSan 和一次完整 OCTEON 链接通过；后续容量边界修订的最新 Actions 验证及实机只读 dry-run 尚待闭环。P7 整体仍未完成。
