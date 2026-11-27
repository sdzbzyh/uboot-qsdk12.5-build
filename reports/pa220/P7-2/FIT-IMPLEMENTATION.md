# P7-2B FIT 语义只读解析实施记录

- 日期：2026-11-27
- 固定 PA-220 源码：`sdzbzyh/pa220-openwrt@801717ba6a8a59ed2cfd53bf3e1b19841fecd2e8`
- 集成分支：`feature/pa220-ubootkit-web`
- 当前状态：**IMPLEMENTATION STAGED — TARGET BUILD/ACTIONS VALIDATION PENDING**
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
- 本轮代码 push 自动触发 Actions workflow `pa220-p7-image-preview`，run #45970127218；当前记录时状态为 `in_progress`，不能写 PASS。
- 下一软件门：检查该 Actions 的 Host/ASan/UBSan 与完整 OCTEON build；若编译/链接通过，再增加独立 FIT fixture vectors，覆盖 inline data、default configuration、kernel/ramdisk/fdt refs、CRC32/SHA1/MD5 PASS/FAIL、ignored hash、unsupported algorithm、signature NOT_VERIFIED、external data unsupported、malformed tree/offset。
- FIT 语义测试通过前，P7-2B 不验收；不安排设备线下操作。

## 4. 安全与证据边界

- 只读取当前已完成接收的 RAM FIT 文件。
- 不读取或修改 eMMC/SPI/NAND；不调用 `saveenv`。
- 不验证 FIT signature 的真实性，不建立固件安装映射，不根据 load/entry 属性执行或复制数据。
- Hash PASS 仅证明对应 inline payload 与 FIT 中声明的摘要匹配，不等于签名可信、来源可信或 PA-220 可安全启动。
- 所有分区枚举与未来目标映射继续按 D-022 同时兼容 DOS/MBR 与 GPT；当前设备已验证的 eMMC 格式仍是 DOS/MBR。
