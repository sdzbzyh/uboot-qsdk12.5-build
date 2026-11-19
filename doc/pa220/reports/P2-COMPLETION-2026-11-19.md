# PA-220 P2 阶段收尾报告（2026-11-19）

- 集成仓库：`sdzbzyh/uboot-qsdk12.5-build`
- 工作分支：`feature/pa220-ubootkit-web`
- 收尾前检查点：`dc7b7f5f370a8a122bb92b0b2ba462fcb3d6940b`
- PA-220 固定源码：`801717ba6a8a59ed2cfd53bf3e1b19841fecd2e8`
- uBootKit 审计基线：`9ce315735ac88fd0a7c7697bc527a8616ed0a972`
- 独立干净构建目录：`/home/debian/PA220_P2-3_clean_20261118`
- 结论：**P2 主线完成；未完成的非阻断性只读核查转入 P3，不阻塞后续开发。**

## 1. P2-1 / P2-2：输入与工具链

- P2-1 固定源码、集成分支、补丁和构建入口核查完成，见 `P2-1-BUILD-INPUT-LOCK.md`。
- P2-2 Debian 13.7 x86_64 构建主机及 Cavium GCC 4.7.0 / Binutils 2.23.52 核查完成，MIPS64r2 OCTEON 大端目标编译和基本链接测试通过，见 `P2-2-HOST-TOOLCHAIN.md`。

## 2. P2-3：干净构建与产物

全新固定源码 checkout 上应用唯一 PA-220 只读维护补丁，完整构建退出码为 0。ELF、BIN、linker map 均生成。

| 产物 | 大小 | SHA256 |
|---|---:|---|
| `u-boot-octeon_pa220` | 约 7.4 MiB | `3545fdadfbf22ae0d57c43388c05f6ffe87070a043aa152034871dc5c73e3ebe` |
| `u-boot-octeon_pa220.map` | 564 KiB | `10a174489fe19e5a39c1ebe909fa6d913deed881c229002c0376fdf05e6e93e2` |
| `u-boot-octeon_pa220.bin` | 1,357,960 bytes | `6dfe3e8f945c43d8959155afc0859ea8b7a0c32f11f91a1e8b008b6499ec8f8b` |

用户在 2026-11-19 再次计算的三项 SHA256 与 P2-3 原始审计报告一致。

ELF 复核：
- ELF32、big-endian、MIPS；flags 包含 `octeon`、`mips64r2`、`abi2`。
- Entry point：`0xc0000000`。
- LOAD：VMA/PhysAddr `0xc0000000`，FileSiz `0x14b888`，MemSiz `0x2e39ac`，RWE。
- `uboot_end_data=0xc014b888`；`uboot_end=0xc02e39b0`。
- LOAD MemSiz 结束地址与 `uboot_end` 仍相差 4 bytes，作为非阻断性链接布局疑点记录，不影响当前继续推进。

## 3. P2-4：关键对象/符号实际编入

用户于 2026-11-19 提供的对象、map、ELF 输出确认：

- 实际对象存在：`arch/mips/cpu/octeon/commands/cmd_elf64.o`、`arch/mips/cpu/octeon/cvmx/cvmx-bootmem.o`。
- `cmd_elf64.o` 符号：`do_bootelf`、`do_bootelf_exec`、`load_elf64_image`。
- `cvmx-bootmem.o` 符号：`cvmx_bootmem_find_named_block`、`__cvmx_bootmem_find_named_block_flags`。
- 最终 ELF/map 中确认：`load_elf64_image`、`do_bootelf`、`flush_cache`、`flush_dcache_range`、`invalidate_dcache_range`、`cvmx_bootmem_find_named_block`、`octeon_configure_load_memory`、`relocate_code`、`uboot_end_data`、`uboot_end`。
- `CONFIG_CMD_OCTEON_ELF=y` 与对象/符号证据一致；本结论基于最终对象和 ELF/map，不仅依赖配置宏。

**P2-4 关键静态核查完成。** 未为缺少的某些 `nm` 输出继续扩大排查，因为 ELF 符号表和 linker map 已提供直接证据。

## 4. RAM 启动验证与异常记录

- 新构建 Stage3 镜像由原厂 Stage2 经 TFTP 加载到 RAM，启动前 `bootloadervalidate` 通过，随后成功启动并进入 U-Boot 命令行。
- 用户使用原版旧 U-Boot 做对照；旧版同样能够从 RAM 启动。
- 新旧 U-Boot 启动后，RAM 中已加载镜像的数据 CRC 均出现变化。因此现有证据不支持将该现象归因于本项目新增代码。
- 旧版镜像分段排查至少确认一处变化：`0x81000fb4–0x81000fb7` 从 `00 00 00 00` 变为 `ff ff ff ff`。其他未取得启动前逐段对照的区域不作判断。
- 该现象没有证明镜像启动失败或持久化存储被写入；不再继续重复 CRC 扫描，后续若有需要再结合旧版实际二进制定位。

## 5. 转入 P3 的待办（非 P2 阻塞项）

以下项目记录后转入 P3 只读诊断，不阻塞主线：
1. 通过运行时只读命令确认 `__tmp_load` named-block descriptor 的 base/size，并与相关 globals 对照；若当前 CLI 不支持，作为 P3 只读诊断功能实现。
2. 执行 `bdinfo`、相关环境变量只读查看及 `pa220 status` / `pa220 env`，记录真实 DRAM、relocation、板级只读状态。
3. 按既定只读方式核对 TLV EEPROM/I²C 窗口；不执行 EEPROM/TLV 写操作。
4. 解释 LOAD MemSiz 与 `uboot_end` 的 4-byte 差异；确认原始 `System-octeon_pa220.map` 为空的原因；如有需要再补独立 SHA256SUMS 文件。

这些项目只在不影响当前阶段交付时继续推进；不为它们重复构建或扩大当前验证范围。

## 6. 安全边界

本阶段没有执行 `saveenv`、EEPROM/TLV 写入、CPLD 写入、SPI/eMMC/NAND/USB block 写入或固件安装。后续 RAM-only 测试继续遵守该边界。P1-A5 定义的运行时 `__tmp_load` 范围验证尚未实现前，不得把新 U-Boot 的任意地址 TFTP/HTTP 上传或任意 ELF segment 加载描述为安全可用。

## 7. 阶段结论

P2-1、P2-2、P2-3 干净构建与基本 RAM 启动、P2-4 关键对象/符号静态核查均已取得足以继续主线的证据。P2 阶段收尾，进入 P3 CLI/只读诊断完善。所有尚未完成的硬件只读检查和链接布局疑点按第 5 节记录，不能误写为 PASS，也不作为 P3 开发的前置阻塞。
