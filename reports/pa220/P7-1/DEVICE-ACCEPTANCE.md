# P7-1 实机验收报告：RAM-only Stage3 启动与只读镜像预览

- 日期：2026-11-27
- 仓库：`sdzbzyh/uboot-qsdk12.5-build`
- 分支：`feature/pa220-ubootkit-web`
- P7-1 代码检查点：`1b86368b69fe293a1a8193b76aa7d75a01d215e7`
- 验收状态：**DEVICE PASS**
- 测试范围：RAM-only Stage3 启动；legacy uImage 与内层 OCTEON bootloader 两种只读 preview。

## 1. Stage3 wrapper 接收与 RAM 启动

旧 U-Boot 版本：`U-Boot 2013.07 (Build time: Nov 25 2026 - 11:04:13)`。

接收 Actions artifact 中的 `u-boot-octeon_pa220-stage3-wrapper.uImage` 至 `0x20000000`：

- TFTP 接收大小：1,390,888 bytes（`0x153928`）。
- 格式：`legacy-uImage`。
- CRC32：`f100a85c`。
- SHA-256：`4f5f3d8561f427cbcfae44f63c5d880e9229afc4fa8130f1556e08b321781ba8`，与 Actions artifact 记录一致。
- 旧 U-Boot `iminfo 0x20000000`：识别 Legacy image；Data Size 1,390,824 bytes；Load Address / Entry Point 均为 `0x81000000`；Checksum OK。
- wrapper payload 从 `0x20000040` 复制至 `0x81000000`，长度 `0x1538e8`；`cmp.b` 报告 1,390,824 bytes 完全相同。
- `go 0x81000000` 成功启动新 Stage3。

新 U-Boot 启动版本：`U-Boot 2013.07 (Build time: Nov 27 2026 - 07:27:14)`。启动日志出现环境 CRC 无效并回退默认环境的提示；这是 RAM 启动环境状态，不影响本轮后续显式设置临时网络变量和镜像元数据。未执行 `saveenv`。

## 2. 网络与临时环境

新 U-Boot 初始未定义 `ipaddr/serverip`，故首次 ping 因 `ipaddr` 未设置而失败。随后仅在 RAM 环境中执行：

```text
setenv ipaddr 192.168.2.2
setenv serverip 192.168.2.10
setenv ethact octrgmii0
ping 192.168.2.10
```

结果：`host 192.168.2.10 is alive`，接口报告 1000 Mbps Full duplex。过程中有一次 `rgmii rx` dropped-frame 提示，但 ping 最终成功。未持久化这些变量。

## 3. 外层 legacy uImage preview

新 U-Boot 再次将相同 wrapper TFTP GET 到 `0x20000000`：

- 接收大小：1,390,888 bytes（`0x153928`）。
- 格式：`legacy-uImage`；CRC32 `f100a85c`。
- SHA-256：`4f5f3d8561f427cbcfae44f63c5d880e9229afc4fa8130f1556e08b321781ba8`。
- `fileaddr=0x20000000`、`filesize=0x153928`。
- `pa220 memory` 保持 `__tmp_load=[0x20000000,0x30000000)`。

`pa220 image preview` 实机输出：

```text
PA-220 image preview (read-only)
format: legacy uImage
size: 0x153928 (1390888 bytes)
header: 64 payload: 0x1538e8 load: 0x81000000 entry: 0x81000000
architecture: MIPS candidate
header_crc/data_crc: PASS
target mapping: UNMAPPED (no approved PA-220 install manifest)
actions: NONE; no device/partition writes, image load or execution performed
```

## 4. 内层 OCTEON bootloader preview

将 wrapper payload 从 `0x20000040` 复制至 `0x20200000`，长度 `0x1538e8`。逐字节比较结果：

```text
Total of 1390824 byte(s) were the same
```

临时设置 `fileaddr=0x20200000`、`filesize=0x1538e8`。再次检查 `pa220 memory`，descriptor 仍为 `__tmp_load=[0x20000000,0x30000000)`。

`pa220 image preview` 实机输出：

```text
PA-220 image preview (read-only)
format: OCTEON bootloader
size: 0x1538e8 (1390824 bytes)
header: 0xc0 payload: 0x153828
board_type: 20021 (compatible)
image_type: NOR bootloader (3)
link_address: 0xffffffffc0000000
header_crc/data_crc: PASS
target mapping: UNMAPPED (no approved PA-220 install manifest)
actions: NONE; no device/partition writes, image load or execution performed
```

## 5. 验收结论与边界

**P7-1 实机验收 PASS。** 下列关键路径均已在真实 PA-220 上验证：

1. Actions Stage3 wrapper 可由旧 U-Boot TFTP 接收并通过 uImage 校验。
2. wrapper payload 可复制、逐字节核对并从 RAM 地址 `0x81000000` 启动。
3. 新 U-Boot 可通过 TFTP 接收 wrapper 并正确预览 legacy uImage。
4. 内层 OCTEON bootloader 可独立定位、复制并正确预览；board type 20021 兼容标记与 header/data CRC 检查均通过。
5. `__tmp_load` descriptor 在测试期间保持不变。

所有操作仅涉及 RAM 和临时环境变量。没有执行镜像安装、镜像 load/boot（除为验证 Stage3 本身而执行的 RAM `go`）、updater、`saveenv` 或任何 eMMC/SPI/NAND 写入。解析器仍将安装目标报告为 `UNMAPPED`，动作报告为 `NONE`。

本报告不代表 FIT configuration/hash/signature 语义、ELF 加载、eMMC GPT 映射或 SPI boot-chain 取证已完成；这些属于 P7-2/P7-3 后续工作。P4 DHCP Client/Ping 独立遗留项仍未因此关闭。
