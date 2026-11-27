# P7-1 云端验收报告：只读镜像解析与预览

- 日期：2026-11-27
- 仓库：`sdzbzyh/uboot-qsdk12.5-build`
- 分支：`feature/pa220-ubootkit-web`
- 固定 PA-220 源码：`sdzbzyh/pa220-openwrt@801717ba6a8a59ed2cfd53bf3e1b19841fecd2e8`
- 状态：**CLOUD BUILD / HOST PASS；DEVICE PREVIEW PASS**

## 1. Host 验证

Actions run `45958126118` 与后续打包 run `45958503668` 均成功。

- 5 组格式测试：OCTEON bootloader header、legacy uImage、FIT/FDT、ELF32、ELF64。
- 普通 GCC：PASS。
- ASan/UBSan：PASS。
- OCTEON header 覆盖已知 Stage1 image type 7、generic board type 28、PA-220 board type 20021、未知 board 拒绝兼容标记、过短 hlen 拒绝及 CRC 错误拒绝。

## 2. OCTEON 目标构建

- 工具链：Cavium OCTEON SDK 5.1.0 / GCC 4.7.0 Build 52。
- 完整 PA-220 U-Boot build：PASS。
- P7 board Makefile / command dispatch 生成补丁：PASS，重复运行幂等性检查 PASS。
- 最终 ELF 符号检查：PASS。
  - `pa220_image_preview`：最终 ELF 中存在，符号大小 704 bytes。
  - `pa220_image_parse`：最终 ELF 中存在，符号大小 2,828 bytes。
- BIN：`u-boot-octeon_pa220.bin`，1,390,824 bytes。
- BIN SHA-256：`f981ad195a0418d142b6a1cf34cfd70327668273612e4506b40d75fa11893c7c`。
- 生成的 P7 board integration patch SHA-256：`42df44dc5a621a9da0c784ec673e71d5aa968812313da96056bd43ab4300139e`。

## 3. 实机 RAM-only 测试文件

Actions artifact：

- Artifact ID：`15179692876`
- 名称：`pa220-p7-1-image-preview-1b86368b69fe293a1a8193b76aa7d75a01d215e7`
- 下载入口：[P7-1 Actions artifact](https://github.com/sdzbzyh/uboot-qsdk12.5-build/actions/runs/45958503668)

包含：

| 文件 | 大小 | SHA-256 |
|---|---:|---|
| `u-boot-octeon_pa220.bin` | 1,390,824 | `f981ad195a0418d142b6a1cf34cfd70327668273612e4506b40d75fa11893c7c` |
| `u-boot-octeon_pa220-stage3-wrapper.uImage` | 1,390,888 | `4f5f3d8561f427cbcfae44f63c5d880e9229afc4fa8130f1556e08b321781ba8` |
| `pa220-p7-uimage-preview.bin` | 320 | `fde6c0b66983a68ae1122813e0510d00977de96d8ff29487f65f43a8fcb542ea` |

Stage3 wrapper 的 uImage payload 长度为 `0x1538e8`，外层文件长度为 `0x153928`。外层 uImage 使当前 P6 TFTP GET recognizer 能安全接收；payload 是本轮 OCTEON Stage3 BIN。实机时只复制 payload，不执行 wrapper header。

## 4. 实机验证状态

**PASS。** 真实 PA-220 已完成 RAM-only Stage3 启动、外层 legacy uImage preview、内层 OCTEON bootloader preview 和 descriptor 不变性检查。完整串口证据、摘要和解析输出见 [P7-1 实机验收报告](DEVICE-ACCEPTANCE.md)。

关键结果：

- Stage3 wrapper TFTP GET：1,390,888 bytes，SHA-256 `4f5f3d8561f427cbcfae44f63c5d880e9229afc4fa8130f1556e08b321781ba8`，与 Actions artifact 一致。
- 旧 U-Boot `iminfo` header/data checksum OK；payload 复制到 `0x81000000` 后 `cmp.b` 全部 1,390,824 bytes 相同，RAM `go` 成功启动 Nov 27 Stage3。
- 新 U-Boot 外层预览识别 legacy uImage，64-byte header、payload `0x1538e8`，header/data CRC PASS，MIPS candidate。
- 内层预览识别 OCTEON bootloader，header `0xc0`、payload `0x153828`、board type 20021 compatible、image type 3、link address `0xffffffffc0000000`，header/data CRC PASS。
- 两次 preview 均报告 target mapping `UNMAPPED`、actions `NONE`；`__tmp_load=[0x20000000,0x30000000)` descriptor 未改变。

测试只使用 RAM 与临时环境变量；未执行安装、updater、`saveenv` 或任何 eMMC/SPI/NAND 写入。

## 5. 当前限制

- FIT/FDT 目前仅验证结构边界；FIT configuration、子镜像、hash/signature 语义尚未完成。
- ELF 目前只做 header/program table/PT_LOAD 边界和候选地址预览；不加载或执行。
- P6 TFTP recognizer 尚未接受裸 OCTEON bootloader header；本次通过 legacy uImage 外壳接收 Stage3 payload。
- 所有安装目标均为 `UNMAPPED`。P7-1 不生成可执行安装动作。
- P7 整体尚未完成；P7-2 需继续取证 FIT 语义、原机 SPI boot chain 与 eMMC GPT 映射。
