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
- 随后用 4 KiB 以内的有效 legacy uImage fixture 做一次 RAM-only 实机预览；不执行 boot/load，不写任何介质。

注意：P6-1C 当前 TFTP GET recognizer 接受 legacy uImage、FIT/FDT、ELF32/ELF64；OCTEON bootloader header 虽已纳入 P7 parser，但尚未纳入 P6-1C 接收格式门。因此当前 TFTP GET 路径不能接收 OCTEON bootloader-header 文件。若后续需要经 TFTP 预览该格式，必须单独扩展并测试 P6 verifier，不能假定本次 P7 parser 已使其可接收。

## P7-2：容器语义与固件布局取证

- 对 FIT 解析 /images、/configurations、默认 configuration、子镜像 data 与 hash/signature 节点；明确区分“结构可读”“hash 通过”“签名可信”。
- 审查 OCTEON bootloader Stage1/2/3、eMMC 和 SPI 更新代码的真实目标选择及写入路径；不得调用 updater。
- 如需分析 `pa220-spi-normal.img`，只把它作为原机 SPI NOR 备份取证；不得把 SPI 镜像误当作 eMMC GPT 证据。
- 对 eMMC 目标映射，必须取得可信 GPT/分区布局证据；缺少设备只读数据时保持 UNMAPPED。

## P7-3：dry-run 安装计划

- 将已验证的镜像组件映射到有证据支持的目标分区/区域。
- 输出来源 offset/length、目标设备/分区、目标 offset/容量、覆盖范围、校验结果、未决项和明确拒绝原因。
- 对未知、歧义、越界、容量不足、签名策略未满足的项目拒绝生成可执行计划。
- 只生成报告；不调用任何写入函数。

## 当前状态

- P7-1 parser、Host Harness、Actions OCTEON 构建状态以本文件对应报告和最新 Actions run 为准。
- 实机 preview 尚未执行。
- P7 整体未完成；P7-2/P7-3 尚未开始。
