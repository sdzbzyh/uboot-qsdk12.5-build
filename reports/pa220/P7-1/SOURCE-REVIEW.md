# P7-1 源码审查：PA-220 镜像解析边界

- 日期：2026-11-27
- 固定源码：`sdzbzyh/pa220-openwrt@801717ba6a8a59ed2cfd53bf3e1b19841fecd2e8`
- 本报告范围：只读源码审查和 P7-1 parser 实现依据；不执行 bootloader updater。

## 1. 现有 OCTEON bootloader header

固定源码位置：

- `u-boot/arch/mips/cpu/octeon/commands/cmd_octeon_bootloader.c`
- `u-boot/arch/mips/include/asm/arch-octeon/cvmx-bootloader.h`

头部开头为 jump instruction、NOP、magic，因此 magic 位于文件 offset 8，而不是 offset 0。header 中含 `hlen`、header version、board type、data length、data CRC、link address、flags 和 image type。

源码的更新路径含有 Stage1/Stage2/Stage3 等类型以及 eMMC/SPI 设备分支；`bootloaderupdate` 会进入持久化更新路径，P7 禁止调用。

现有 `validate_header()` 对 `hlen` 有最大值检查，但未以接收文件的实际 `filesize` 建立完整的最小长度/剩余长度边界；header CRC 长度又依赖 `hlen`。因此不能把旧 `bootloadervalidate` 当成任意 RAM 文件的通用安全 parser。P7 使用独立 bounded parser：先验证 header 最小/最大长度、文件总长度、payload 长度，再读取 CRC 覆盖区间。

P7 当前按 PA-220 board type 20021 判断兼容性；generic board type 28 仅标记为通用候选，不据此推断安装目标。

## 2. 其他格式

- Legacy uImage：固定 64-byte header；P7 校验 header CRC、payload size 与 data CRC。
- FIT/FDT：P7 当前只检查 FDT header 和 offsets/sizes 的文件边界。尚未读取 FIT configuration 或宣称 hash/signature 通过。
- ELF32/ELF64：P7 只解析 header 与 program-header table，检查 PT_LOAD 的 file range、filesz/memsz 和 load-address 加法溢出；只展示 segment 信息，不调用 bootelf/load。

## 3. 目标映射证据缺口

- `pa220-spi-normal.img` 是完整原机 SPI NOR 备份，不是 eMMC GPT 镜像。
- SPI 备份可用于后续识别原机 SPI boot chain/镜像布局，但不能据此推断 eMMC 分区表。
- 在没有可信 eMMC GPT/分区表或明确 PA-220 固件 manifest 前，所有格式的安装 target 都必须显示 `UNMAPPED`。
- 不复用 IPQ/QCA uBootKit 的 HLOS/rootfs/ART/CDT 映射。

## 4. P7-1 生产实现边界

`pa220 image preview` 只读取已完成接收的 `fileaddr/filesize`，并重新调用 `octeon_load_range_check()` 确认输入位于有效 `__tmp_load` reservation。解析器不调用 `bootm`、`bootelf`、`bootloaderupdate`、`mmc write`、`sf write/update`、NAND 写入或 `saveenv`。

P6-1C 当前 TFTP GET 格式 recognizer 不包含 OCTEON bootloader header；所以 P7 parser 对该格式的 host-side 支持不意味着 TFTP GET 当前可接收该格式。要经 TFTP 传输此类镜像，须另行扩展 P6 verifier 并补齐测试。

## 5. 当前验证状态

- Host parser vectors：5 suites，GCC 与 ASan/UBSan 均 PASS。
- OCTEON target object / final U-Boot link：PASS，Actions run #45958126118；打包 wrapper 和实机 fixture 的 run #45958503668 也 PASS。最终 ELF 保留 `pa220_image_preview` 与 `pa220_image_parse`。
- 实机 preview 尚未运行，不能将云端构建通过视为设备验收。完整数据见 `reports/pa220/P7-1/ACCEPTANCE.md`。
- 真实设备 `pa220 image preview`：NOT RUN。
- 安装目标映射：UNMAPPED。
