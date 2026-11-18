# PA-220 P2-3 干净构建与产物静态核查（2026-11-18）

- 集成仓库：`sdzbzyh/uboot-qsdk12.5-build`
- 工作分支：`feature/pa220-ubootkit-web`
- PA-220 固定源码：`801717ba6a8a59ed2cfd53bf3e1b19841fecd2e8`
- 工具链：`/home/debian/OCTEON-SDK/tools-gcc-4.7`，Cavium GCC 4.7.0 / Binutils 2.23.52
- 构建退出码：`0`
- 状态：**干净 checkout 上的完整构建成功；产物静态核查已取得主要证据；运行时/硬件验证未进行。**

## 1. 构建输入与可重复性

本轮使用新建源码目录 `/home/debian/PA220_P2-3_clean_20261118`，检出固定源码 commit，检出后工作区干净。仅应用集成仓库中的补丁：

- `0001-pa220-read-only-maintenance-command.patch`
- Git blob SHA：`09dcc6bd6c7440e4de6e5d955532a53334e8eafc`
- 应用后仅修改 `u-boot/board/octeon/pa220/pa220_board.c`，新增 86 行；`git diff --check` 通过。
- 构建入口：集成仓库 `build-pa220.sh`，调用固定源码原生 `u-boot/build.sh`。
- 工具链目录通过参数显式传入；构建输出独立保存。

构建退出码为 0。完整日志由用户上传至本目录的 `build.log`；构建退出码记录为 `build-exit-code.txt`。配置快照包括 `config.h`、`config.mk`、`autoconf.mk` 和 `board-config.mk`。

## 2. 主要产物及 SHA256

本地审计输出记录的文件大小与 SHA256：

| 文件 | 大小 | SHA256 |
|---|---:|---|
| `u-boot-octeon_pa220`（ELF） | 约 7.4 MiB | `3545fdadfbf22ae0d57c43388c05f6ffe87070a043aa152034871dc5c73e3ebe` |
| `u-boot-octeon_pa220.bin` | 1,357,960 bytes | `6dfe3e8f945c43d8959155afc0859ea8b7a0c32f11f91a1e8b008b6499ec8f8b` |
| `u-boot-octeon_pa220.map` | 564 KiB | `10a174489fe19e5a39c1ebe909fa6d913deed881c229002c0376fdf05e6e93e2` |

BIN 头部审计输出：
- Header image type：`NOR`
- 数据长度：`1,357,768` bytes
- Data CRC：`0x350a32e0`
- Header CRC：`0x0c902183`
- BIN 总长比数据长度多 192 bytes，符合该构建头格式的记录；具体字段语义仍以对应头格式实现为准。

构建统计：
- text：1,327,836 bytes
- data：30,108 bytes
- bss：1,671,340 bytes
- text + data + bss：3,029,284 bytes（`0x2e3924`）

`CONFIG_BOARD_SIZE_LIMIT=0x00200000`，BIN 数据长度小于 2 MiB 限制。

## 3. ELF 与地址布局

审计输出确认：
- ELF32、MSB big-endian、MIPS N32；
- ELF flags 包含 `octeon` 与 `mips64r2`；
- ELF 类型为 EXEC；
- Entry point：`0xc0000000`；
- 唯一 LOAD 段：VMA/PhysAddr `0xc0000000`，FileSiz `0x14b888`，MemSiz `0x2e39ac`，权限 RWE；
- `.text` 从 `0xc0000000` 开始；
- `.bss` 地址 `0xc014b900`，大小 `0x1980ac`；
- `uboot_end_data=0xc014b888`；
- `uboot_end=0xc02e39b0`。

注意：LOAD 段结束地址按 MemSiz 计算为 `0xc02e39ac`，而符号 `uboot_end` 为 `0xc02e39b0`，存在 4-byte 差异。当前将其列为链接布局复核项，不据此推断构建失败；需要结合链接脚本、符号定义和实际装载/重定位规则确认。

## 4. 新增只读命令是否进入产物

ELF 符号表及 linker map 均包含：
- `pa220_env_value`
- `do_pa220`
- `_u_boot_list_2_cmd_2_pa220`

map 中 `_u_boot_list_2_cmd_2_pa220` 地址为 `0xc014b220`。BIN strings 可见：
- `PA-220 maintenance status`
- `PA-220 environment storage`
- `WARNING : saveenv writes to SPI NOR.`
- `PA-220 read-only maintenance information`

因此可以确认新增 `pa220` 命令代码和命令表项已进入最终 ELF/BIN。命令在目标板上的实际执行结果尚未验证。

## 5. 构建警告

完整构建输出没有报告致命错误，最终退出码为 0。日志中存在重复的宏重定义 warning，主要涉及：
- `CONFIG_OCTEON_CN70XX`
- `CONFIG_CMD_EXPORTENV`
- `CONFIG_CMD_IMPORTENV`
- `CONFIG_SYS_MEMTEST_START`
- `CONFIG_SYS_MEMTEST_END`

这些 warning 来自旧版配置头与生成配置/默认配置之间的重复定义。当前不为消除 warning 擅自改动固定源码；后续可在独立任务中检查定义值是否一致。它们不改变本轮“构建成功”的事实，但也不应从报告中隐藏。

## 6. 映射文件注意事项

- `u-boot-octeon_pa220.map` 是链接器生成的 map。
- 构建目录中的原始 `System-octeon_pa220.map` 在用户检查时为 0 bytes（空文件）。
- `System-octeon_pa220.from-ELF.map` 是之后从 ELF 重新生成的符号映射文件，约 99 KiB；它不是构建脚本成功生成的原始 System.map，必须保留这个来源区别。
- 当前归档没有 `SHA256SUMS` 汇总文件；SHA256 值来自用户本地审计报告。后续归档应补齐独立校验清单，并确认 GitHub 上下载的二进制文件哈希与本地值一致。

## 7. 配置事实与安全边界

上传的 `autoconf.mk` 显示环境配置包括 SPI 环境主区偏移 `0xe00000`、SPI CS 0、环境大小 8 KiB、sector 64 KiB；同时启用了 `CONFIG_CMD_SAVEENV`。这只是构建配置事实，不是执行写入的许可。

本轮没有在设备上运行镜像，没有执行 `saveenv`、EEPROM/TLV 写入、CPLD 写入、SPI/eMMC/NAND/USB block 写入或固件安装。所有后续测试继续遵守 RAM-only 和只读验证约束。

## 8. 尚未完成的核查

1. 核对最终配置与 map/对象，确认 OCTEON ELF64 loader 相关命令/对象及 `load_elf64_image`、Cache API、named-block API 的实际编入情况；不得只根据配置宏推断。
2. 解释 LOAD 段 MemSiz 与 `uboot_end` 的 4-byte 差异。
3. 确认原始 `System.map` 为空的原因；当前 ELF 派生 map 只作为补充。
4. 生成并归档 SHA256SUMS，独立复核上传后 BIN/ELF/map 的哈希。
5. 在设备上通过原厂 Stage2/TFTP 将镜像加载到 RAM 测试；不刷写任何持久化介质。
6. 启动后收集启动日志、`bdinfo`、指定 `printenv` 变量、`pa220 status`、`pa220 env`；按既定计划只读核查 `__tmp_load` named block 与 TLV EEPROM。

**阶段结论：P2-3 完整构建通过。产物已有静态核查证据，但 P2 整体及实机验证均未完成；不得将本报告解读为镜像已验证可启动或可安全写入介质。**
