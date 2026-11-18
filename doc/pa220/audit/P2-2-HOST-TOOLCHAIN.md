# PA-220 P2-2 Linux 构建主机与 OCTEON SDK 工具链核查

- 日期：2026-11-18
- 集成仓库：`sdzbzyh/uboot-qsdk12.5-build`
- 工作分支：`feature/pa220-ubootkit-web`
- 核查时集成仓库 HEAD：`80a20ef3647139fc08b9f26e8c3177e997fc0dcd`
- 固定 PA-220 源码：`801717ba6a8a59ed2cfd53bf3e1b19841fecd2e8`（不变）
- 固定 uBootKit 审计源码：`9ce315735ac88fd0a7c7697bc527a8616ed0a972`（不变）
- 状态：**P2-2 主机与工具链核查完成；允许进入 P2-3 干净构建准备。此结论不代表 PA-220 U-Boot 已完整构建或实机验证。**

## 1. 核查范围与安全边界

本阶段只检查用户实际 Linux 构建主机、OCTEON SDK 工具链、宿主构建依赖，以及通过临时 C 源文件验证目标编译和基本链接能力。

- 未修改 PA-220 源码、uBootKit 源码、SDK 文件或项目补丁。
- 编译/链接测试在 `/tmp` 临时目录完成，测试后已删除，输出 `TEMP_TEST_CLEANED=YES`。
- 未执行 PA-220 全量构建、固件安装或任何设备操作。
- 未执行 `saveenv`、EEPROM/TLV 写入、CPLD 写入、SPI/eMMC/NAND/USB block 写入。

## 2. Linux 主机环境

| 项目 | 用户实际输出 |
|---|---|
| 发行版 | Debian GNU/Linux 13 (trixie)，`DEBIAN_VERSION_FULL=13.7` |
| 内核 | `6.12.115+deb13.1-amd64`（Debian build `6.12.115-2`） |
| 主机架构 | `x86_64` |
| 宿主 GCC | Debian `14.2.0-19` |
| 宿主 G++ | Debian `14.2.0-19` |
| GNU Make | `4.4.1` |
| Git | `2.47.3` |
| `nproc` | `8` |

以下命令均由用户的 `command -v` 输出确认存在：`gcc`、`g++`、`make`、`git`、`nproc`、`awk`、`sed`、`perl`、`python3`、`bison`、`flex`、`dtc`。

P2-1 已核对原生构建入口还使用 `ls`；本轮补充依赖清单没有单独列出 `ls`、`cp`、`mkdir`、`rm` 等 coreutils 命令。它们的可用性应在 P2-3 执行构建脚本前由实际命令运行自然验证，不把未检查项写成已验证。

## 3. OCTEON SDK 工具链

### 3.1 实际路径和 GCC

- 用户当前实际工具链目录：`/home/debian/OCTEON-SDK/tools-gcc-4.7`。
- 交叉 GCC：`bin/mips64-octeon-linux-gnu-gcc`，符号链接指向 `mips64-octeon-linux-gnu-gcc-4.7.0`。
- GCC 版本：`4.7.0 (Cavium Inc. Version: SDK_BUILD build 52)`。
- GCC target / `-dumpmachine`：`mips64-octeon-linux-gnu`。
- GCC `-print-sysroot`：`/home/debian/OCTEON-SDK/tools-gcc-4.7/mips64-octeon-linux-gnu/sys-root`（以规范化前的 GCC 输出路径为准）。
- sysroot 根目录、`lib`、`usr/lib` 均由 `ls -ld` 确认存在。
- `-print-libgcc-file-name` 指向 SDK 内的 `libgcc.a`；`-print-file-name=crt1.o` 与 `crti.o` 返回 SDK sysroot 下的预期路径。该轮只记录 GCC 路径解析结果，没有逐个验证这些运行库文件的存在/内容。

### 3.2 Binutils 和交叉工具

配套 Binutils 版本为 `GNU Binutils (Cavium Inc. Version: SDK_BUILD build 52) 2.23.52`。已取得 `ld.bfd`、`objcopy`、`ar`、`nm`、`objdump`、`readelf` 的版本输出。

以下交叉工具均通过可执行文件检查：

- `mips64-octeon-linux-gnu-gcc`
- `mips64-octeon-linux-gnu-g++`
- `mips64-octeon-linux-gnu-ld`
- `mips64-octeon-linux-gnu-ar`
- `mips64-octeon-linux-gnu-as`
- `mips64-octeon-linux-gnu-nm`
- `mips64-octeon-linux-gnu-objcopy`
- `mips64-octeon-linux-gnu-objdump`
- `mips64-octeon-linux-gnu-strip`
- `mips64-octeon-linux-gnu-ranlib`
- `mips64-octeon-linux-gnu-readelf`

SDK 的 `bin/` 目录还包含另一套 `mipsisa64-octeon-elf-*` 工具链。PA-220 固定构建入口要求的是 `mips64-octeon-linux-gnu-*`，本项目不要混用两套前缀。

### 3.3 交叉 GCC 宿主可执行文件观察

对交叉 GCC 符号链接目标执行了 `file` 和 `readelf -l`。输出显示该编译器驱动是 ELF 可执行文件；程序头未显示 `INTERP` 项。此前 `ldd` 输出“不是动态可执行文件”。

该现象没有阻止 GCC 正常运行、编译或调用链接器。当前不将 `ldd` 这条输出单独判定为工具链故障；也不据此推断其所有运行时依赖均已完整核验。

## 4. 实际编译验证

### 4.1 宿主 GNU89 参数检查

用户执行：

```sh
printf 'int main(void) { return 0; }\\n' | gcc -fcommon -std=gnu89 -x c -fsyntax-only -
```

结果：`HOSTCC_FLAGS_OK=0`。

这证明宿主 GCC 14.2 接受 PA-220 host wrapper 所需的 `-fcommon -std=gnu89` 参数。此处是参数/语法检查；完整 U-Boot host tools 与实际 `tools/hostcc-gnu89` 的集成仍由 P2-3 全量构建验证。

### 4.2 MIPS64 OCTEON 目标文件编译

临时 C 文件通过：

```sh
mips64-octeon-linux-gnu-gcc -c test.c -o test.o
```

结果：`CROSS_COMPILE_EXIT=0`。

`file test.o` 与 `readelf -h test.o` 确认：

- ELF64；
- 2's complement, big endian（MSB）；
- MIPS；
- flags：`0x808b0007`，包含 `octeon`、`mips64r2`；
- `objdump -f` 识别为 `elf64-tradbigmips`、`mips:octeon`。

### 4.3 基本链接测试

对同一测试对象执行：

```sh
mips64-octeon-linux-gnu-gcc -nostdlib -Wl,-e,pa220_test_function test.o -o test.elf
```

结果：`COMPILE_RC=0`、`LINK_RC=0`。

最终文件被识别为 ELF64 MSB MIPS64 Release 2 executable，`readelf` flags 仍包含 `octeon` 和 `mips64r2`，`objdump` 识别为 `mips:octeon`。该测试证明 GCC 驱动、汇编器和链接器能够协同完成最小目标文件编译及 `-nostdlib` 基本链接。

**测试限制：**链接使用 `-nostdlib` 和显式入口点，没有验证目标 libc、完整 CRT 启动对象、SDK 的全部运行库组合，也没有验证 U-Boot 自身的链接脚本、重定位、板级对象或最终 BIN 生成。

## 5. 兼容性判断

| 核查项 | 结论 |
|---|---|
| Debian 13 主机可启动交叉 GCC | 已验证 |
| OCTEON GCC 目标编译 | 已验证 |
| 目标 ELF64 / 大端 / MIPS64r2 / OCTEON 属性 | 已验证 |
| GCC 驱动调用汇编器和链接器 | 已验证（最小测试） |
| 宿主 GNU89 参数 | 已验证 |
| 完整 PA-220 U-Boot host tools 构建 | 未验证 |
| 目标 libc / CRT 完整链接 | 未验证 |
| 所有 GCC runtime / sysroot 文件完整性 | 未完全验证 |
| 完整 PA-220 U-Boot 构建与产物 | 未验证，属于 P2-3/P2-4 |
| RAM 启动和设备功能 | 未验证，属于后续运行时测试 |

**结论：P2-2 通过。** 当前没有证据要求更换 Debian 主机或 OCTEON SDK。用户当前工具链已具备进入 P2-3 干净构建的基础条件。最终兼容性仍以 P2-3 的真实 U-Boot 构建结果为准。

## 6. P2-3 执行前约束

1. 使用全新且不存在的 `PA220_REPO_DIR`；不得复用现有开发目录或旧构建目录。
2. 检出 PA-220 固定 commit `801717ba6a8a59ed2cfd53bf3e1b19841fecd2e8`，不改变上游仓库或固定基线。
3. 使用集成仓库当前唯一 PA-220 补丁，并保存 `git apply --check` / 应用结果。
4. 使用本轮确认的工具链路径作为本机输入，但构建脚本仍通过显式参数/环境变量接收路径，不把用户主机绝对路径写死为通用默认值。
5. 保存完整构建日志、退出码、工具链版本、实际配置文件、BIN/ELF/map/System.map（若生成）、`uboot_end`、产物大小和 SHA256。
6. 若构建失败，按首个真实错误定位，不把未验证的硬件假设变成构建补丁。
7. 本阶段仍禁止 `saveenv`、EEPROM/TLV 写入、CPLD 写入、SPI/eMMC/NAND/USB block 写入和固件安装。
8. 不影响构建且可在构建后通过只读运行时命令确认的事项继续记录；构建后提醒用户执行并反馈。

## 7. 最终状态

- P2-1：已完成，见 `P2-1-BUILD-INPUT-LOCK.md`。
- P2-2：**已完成**，本报告记录当前主机和 SDK 实际证据。
- P2-3：待执行，不属于本报告完成范围。
- P2-4：待执行，不属于本报告完成范围。
- P2 整体：未完成。
