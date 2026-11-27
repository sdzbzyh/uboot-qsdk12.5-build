# P7-2B FIT 语义只读解析实施记录

- 日期：2026-11-27
- 固定 PA-220 源码：`sdzbzyh/pa220-openwrt@801717ba6a8a59ed2cfd53bf3e1b19841fecd2e8`
- 集成分支：`feature/pa220-ubootkit-web`
- 当前状态：**PASS — TARGET BUILD, FIXTURE GENERATION AND PA-220 RAM-ONLY FIT RUNTIME VALIDATION**
- 生产入口：`pa220 image preview`
- 实现位置：`u-boot/board/octeon/pa220/pa220_image.c`

## 1. 固定源码确认的 FIT 能力

- `octeon_common.h` 启用 `CONFIG_FIT`、`CONFIG_OF_LIBFDT`；PA-220 配置未启用 `CONFIG_FIT_SIGNATURE`。
- 固定 `image-fit.c` 的 `fit_image_get_data()` 只读取 image node 的 inline `data` property；不解析外置数据 `data-offset` / `data-position`。
- 固定 `calculate_hash()` 实际支持 `crc32`、`sha1`、`md5`；其他算法（包括 SHA-256）必须报告 `UNSUPPORTED`，不能因通用 SHA256 命令存在而声称 FIT hash 已验证。
- `IMAGE_ENABLE_IGNORE=1`（target build）时，非零 `uboot-ignore` 会跳过对应 hash；preview 必须报告 `IGNORED`，不可伪装成 PASS。
- FIT signature node 不等于可信签名。当前 PA-220 没有 FIT signature verification/key policy；preview 只标记 `NOT_VERIFIED`。
- `fit_image_load()` 会将数据复制到 load 地址，不属于只读 preview，严禁调用。

## 2. 已加入的生产解析逻辑

- 复用固定源码的 libfdt/FIT read-only API：`fdt_check_header`、`fdt_next_node`、`fdt_path_offset`、`fdt_first_subnode`、`fdt_next_subnode`、`fdt_getprop`，以及 `fit_check_format`、`fit_image_get_data`、`fit_image_get_node`、`fit_conf_get_node` 和 `calculate_hash`。
- 对 FDT totalsize、structure/string block 范围、块间重叠、reservation-map 终止项和地址加法溢出做边界检查；遍历整棵 structure tree，拒绝损坏结构。
- 遍历 `/images`：报告 image unit name、type/arch/os/compression、可用 load/entry、inline data 长度和 hash 子节点状态。
- 遍历 `/configurations`：报告 default configuration、kernel/ramdisk/fdt 引用及其到 `/images` 的解析结果；列出 image/configuration signature nodes，但全部标记 `NOT_VERIFIED`。
- Hash 节点状态区分 `PASS`、`FAIL`、`IGNORED`、`UNSUPPORTED`、`MALFORMED`。只支持固定源码实际实现的算法。
- 外置数据显式报告 `EXTERNAL_DATA_UNSUPPORTED`；没有 inline data 且没有外置数据引用则视为 malformed。
- 不调用 `fit_image_load`、`bootm`、`bootm_start`、`bootm_run`、`bootstage3` 或任何写入/执行路径。目标映射仍为 `UNMAPPED`。

## 3. 当前验证状态

- P7-1 既有 Host GCC / ASan / UBSan 的 5 suites 仍是旧 image parser 覆盖，不代表本轮新增 FIT 语义分支已经通过 host FIT fixtures。
- 新增 FIT 语义代码位于 target-only 分支，需先由 pinned Cavium OCTEON SDK 完整构建验证其 API/链接兼容。
- Actions #45970127218（代码编译检查点）SUCCESS：Host GCC、ASan/UBSan、Cavium GCC 4.7.0 OCTEON object、N32 relocatable link、完整 U-Boot final link 均通过。
- Actions #45970607698 SUCCESS：新增 FIT fixtures 生成、Host GCC/ASan/UBSan、固定 SDK 校验、完整 OCTEON build、生成补丁幂等性均通过。Artifact [15184393205](https://github.com/sdzbzyh/uboot-qsdk12.5-build/actions/runs/45970607698/artifacts/15184393205)。
- 正常 FIT fixture：2,518 bytes，SHA-256 `db96358852a1c47fe7788c5ecf89093daf6d26142b3e946a3f8ca7a1d29ff565`。
- Hash-fail FIT fixture：2,518 bytes，SHA-256 `a2cc42cf33f94a5ef306068fb9e88a8b84e746fc672379227494aac1ea504f02`。
- 新 U-Boot BIN：1,394,728 bytes，SHA-256 `9bf05f6915745d315023b3184cdf2668428fd5f10234d2e777e3f9372024e586`；Stage3 wrapper：1,394,792 bytes，SHA-256 `6d2b4d738934789b500bbe956e6cf828152c9ffcba8c8a66adb8528ff52ba9fe`。
- Actions 的 5 suites 是 P7-1 通用镜像解析器既有 Host/ASan/UBSan 测试，不等于新增 target-only FIT 语义路径已通过运行测试。FIT fixtures 已生成，尚待 PA-220 RAM-only 实机执行。
- 已完成的实机门：PA-220 RAM 启动该新 U-Boot，分别预览两个 FIT fixture；default configuration、kernel/ramdisk/fdt refs、CRC32/SHA1/MD5 PASS/FAIL、ignored hash、unsupported SHA-256、signature NOT_VERIFIED、external data unsupported 均已按预期验证。此测试只读取 TFTP 接收的 RAM 文件，不触碰 eMMC/SPI/NAND。
- P7-2B FIT 语义实机验收已通过；无需重复设备操作。

## 4. 安全与证据边界

- 只读取当前已完成接收的 RAM FIT 文件。
- 不读取或修改 eMMC/SPI/NAND；不调用 `saveenv`。
- 不验证 FIT signature 的真实性，不建立固件安装映射，不根据 load/entry 属性执行或复制数据。
- Hash PASS 仅证明对应 inline payload 与 FIT 中声明的摘要匹配，不等于签名可信、来源可信或 PA-220 可安全启动。
- 所有分区枚举与未来目标映射继续按 D-022 同时兼容 DOS/MBR 与 GPT；当前设备已验证的 eMMC 格式仍是 DOS/MBR。

## 5. PA-220 实机 FIT 语义验收 — PASS（2026-11-27）

设备使用 Nov 27 09:04:34 Stage3 build；TFTP host 192.168.2.10，设备地址 192.168.2.2，接口 octrgmii0。两个 fixture 均接收到 RAM 0x20000000，仅执行 `pa220 image preview`。

### 正常 fixture

- TFTP：2,518 bytes；格式 FIT/FDT；CRC32 `f128aa47`；SHA-256 `db96358852a1c47fe7788c5ecf89093daf6d26142b3e946a3f8ca7a1d29ff565`，与预期 fixture 一致。
- Kernel、ramdisk、FDT 均识别；inline data 分别为 256、28、24 bytes。
- Kernel 与 ramdisk 的 CRC32、SHA1、MD5 均 PASS；ignored hash 为 IGNORED；SHA-256 为 UNSUPPORTED；signature 为 NOT_VERIFIED。
- External-data image 明确为 EXTERNAL_DATA_UNSUPPORTED。
- default configuration conf-1 的 kernel/ramdisk/fdt 引用全部 RESOLVED。
- `FIT images=4 configurations=1 hash_failures=0`；signature capability NOT ENABLED；target mapping UNMAPPED；actions NONE。

### 损坏 hash fixture

- TFTP：2,518 bytes；格式 FIT/FDT；CRC32 `1b55a6a2`；SHA-256 `a2cc42cf33f94a5ef306068fb9e88a8b84e746fc672379227494aac1ea504f02`，与预期 fixture 一致。
- Kernel 的 CRC32、SHA1、MD5 均 FAIL；ramdisk 对应三种 hash 均 PASS。
- ignored/unsupported/signature/external-data/configuration 状态与预期一致。
- `FIT images=4 configurations=1 hash_failures=1`；target mapping UNMAPPED；actions NONE。

### 验收结论与边界

正常与损坏两个向量均由目标设备实际解析，状态分类与预期完全一致。Hash PASS 仅代表 inline payload 与声明摘要匹配，不代表签名可信或启动安全。整个测试没有调用 image load/boot 路径，没有读写 eMMC/SPI/NAND，没有执行 `saveenv`，没有持久化写入。该验收不批准任何安装映射。