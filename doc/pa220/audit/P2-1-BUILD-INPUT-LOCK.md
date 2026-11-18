# PA-220 P2-1 构建输入锁定记录

- 日期：2026-11-18
- 集成仓库：`sdzbzyh/uboot-qsdk12.5-build`
- 工作分支：`feature/pa220-ubootkit-web`
- 检查时 HEAD：`4f9aaafaa5549adcf7b2042c19ed6621758e7639`
- 状态：**P2-1 静态输入核查完成；工具链实机环境核查待用户提供构建机信息。**

## 1. 固定版本核对

| 输入 | 核对结果 |
|---|---|
| 集成仓库分支 | `feature/pa220-ubootkit-web` |
| 集成仓库 HEAD | `4f9aaafaa5549adcf7b2042c19ed6621758e7639`，与本次恢复提供的检查点一致 |
| uBootKit 审计仓库 | `chenxin527/uboot-qsdk12.5-build@9ce315735ac88fd0a7c7697bc527a8616ed0a972`，该 commit 可读取 |
| PA-220 原始上游 | `IronDemiGod/pa220-openwrt@801717ba6a8a59ed2cfd53bf3e1b19841fecd2e8`，该 commit 可读取 |
| 构建脚本使用的 PA-220 镜像仓库 | `sdzbzyh/pa220-openwrt`，同一 SHA `801717ba6a8a59ed2cfd53bf3e1b19841fecd2e8` 可读取，commit 对象一致 |

没有更新任何固定源码基线。

## 2. 集成构建脚本

检查文件：`build-pa220.sh`。

- 固定源码 URL：`https://github.com/sdzbzyh/pa220-openwrt.git`。
- 固定源码 commit：`801717ba6a8a59ed2cfd53bf3e1b19841fecd2e8`。
- 默认源码目录：`external/pa220-openwrt`。
- 实际构建目录：源码仓库中的 `u-boot/`。
- 可覆盖变量：`PA220_REPO_DIR`、`PA220_UBOOT_DIR`、`PA220_OUTPUT_DIR`、`OCTEON_TOOLCHAIN_DIR`。
- 显式工具链参数优先；未提供时要求交叉编译器已在 `PATH`。
- 脚本检查 `git`、`make`、`nproc`，校验 U-Boot Git HEAD 必须等于固定 commit。
- 按 `patches/pa220/*.patch` 的 shell glob 顺序应用补丁，再调用 PA-220 原生 `u-boot/build.sh`。
- 只把最终 BIN 复制到输出目录；默认目标为 `bin/pa220/u-boot-octeon_pa220.bin`。

### 可重复构建注意事项

1. 脚本只校验 Git HEAD，不检查源码工作树是否存在补丁以外的未提交修改。为了本轮“干净构建”，必须使用一个全新且不存在的 `PA220_REPO_DIR`，不能把已有开发目录当成干净输入。
2. 脚本和原生 `build.sh` 均未先执行 `make clean/distclean`。新 clone 可避免旧对象文件、旧配置和旧产物污染本轮结果。
3. 补丁应用会修改 PA-220 checkout 的工作树，但不会改变其 Git HEAD。脚本通过 `git apply --reverse --check` 识别已应用补丁；否则先 `git apply --check` 再应用。当前单补丁重复执行设计具备幂等检查。
4. 脚本不会自动保存工具链版本、完整日志、退出码、配置快照、map/ELF 或 SHA256。P2-3 必须由构建执行命令和记录步骤补足。
5. 脚本只复制 BIN；map、ELF、System.map 等构建分析文件需要从源码构建目录另行归档。

## 3. PA-220 补丁清单与顺序

当前 `patches/pa220/` 仅有一项补丁，按文件名排序首先应用：

1. `0001-pa220-read-only-maintenance-command.patch`

补丁 blob：`09dcc6bd6c7440e4de6e5d955532a53334e8eafc`。

目标文件：`u-boot/board/octeon/pa220/pa220_board.c`，在现有 CPLD 命令注册之后增加只读 `pa220` 命令：

- `pa220` / `pa220 status`：读取板型、DRAM、CPU 时钟、relocation、网络环境变量及 CPLD 状态寄存器。
- `pa220 env`：显示 SPI 环境后端及主/冗余区配置和写入警告。

该补丁没有调用 CPLD 写接口、`saveenv` 或存储写接口。补丁目标上下文与固定源码对应位置一致。尚未在用户的本地 checkout 执行 `git apply --check` 或编译；这两项应由干净构建日志提供证据。

## 4. PA-220 原生构建入口

固定源码文件：`u-boot/build.sh`。

执行顺序：

1. 进入 `u-boot/`。
2. 如传入工具链目录，将其 `bin/` 加入 `PATH`。
3. 检查 `mips64-octeon-linux-gnu-gcc`。
4. 导出 `PATH` 和 `OCTEON_MODEL=OCTEON_CN70XX`。
5. 设置 `HOSTCC=$(pwd)/tools/hostcc-gnu89`。
6. 执行 `make octeon_pa220_config`。
7. 执行 `make -j$(nproc) HOSTCC="$HOSTCC"`。
8. 检查 `u-boot-octeon_pa220.bin`。

`tools/hostcc-gnu89` 是可执行 shell wrapper，实际调用宿主机 `gcc -fcommon -std=gnu89`。因此除了 OCTEON 交叉工具链，宿主机还必须有可用的原生 GCC。原生构建脚本直接使用 `make`、`nproc` 和 `ls`；旧版 U-Boot Makefile/工具还会使用常见宿主机工具（如 awk、sed、perl 等），需要在 P2-2 实机环境检查中核实。

## 5. 配置入口与配置产物

固定源码 `u-boot/boards.cfg` 中存在：

```
octeon_pa220  mips  octeon  pa220  octeon  -  octeon_pa220:OCTEON_PA220,OCTEON_PCI_HOST=0,OCTEON_CN70XX
```

`make octeon_pa220_config` 通过旧版 `mkconfig` 生成板级配置。该 U-Boot 2013.07 构建系统不是现代 Kconfig 流程，不应预设一定会产生 Linux/U-Boot 新式 `.config`。构建记录应保存实际生成的 `include/config.h`、`include/config.mk`、`include/autoconf.mk`（若生成）及相关配置输入；P2-4 以最终对象和 ELF/map 为准，不以配置宏存在代替编入证据。

板级配置入口：
- `u-boot/include/configs/octeon_pa220.h`
- `u-boot/board/octeon/pa220/config.mk`

已核对 `octeon_pa220.h` 启用 CN70XX、OCTEON 网络/QLM、TLV EEPROM、MMC/eMMC、SPI、文件系统和多项命令配置；具体最终编入情况留待 P2-4。板级 `config.mk` 设置 `CONFIG_SYS_TEXT_BASE = 0xC0000000`，并添加 OCTEON2 编译选项。

## 6. 工具链要求与当前未知项

源码要求：

- Linux 构建主机；
- 宿主机 `git`、`make`、`nproc`、原生 `gcc`；
- OCTEON SDK 交叉编译器 `mips64-octeon-linux-gnu-gcc`；
- 与该工具链配套的 linker、objcopy、ar、nm、objdump 等二进制工具；
- 宿主机常用构建工具。

历史日志曾出现 `/home/debian/OCTEON-SDK/tools-gcc-4.7` 和 GCC include 路径 `mips64-octeon-linux-gnu/4.7.0`，但这只是仓库中旧日志的历史线索，不能证明该路径和工具链现在仍存在，也不能替代完整版本输出。当前会话无法访问用户的实际 Linux 构建主机或本地 SDK，因此**工具链当前可用性、完整版本、宿主机发行版和依赖均未验证**。

## 7. 输出与归档

原生构建日志中可见的产物命名规则为：

- BIN：`u-boot-octeon_pa220.bin`
- ELF：`u-boot-octeon_pa220`
- linker map：`u-boot-octeon_pa220.map`
- System.map：`System-octeon_pa220.map`

集成脚本默认只复制 BIN 到 `bin/pa220/u-boot-octeon_pa220.bin`。P2-3 应额外保存上述文件（若实际生成）、最终配置快照、完整日志和 SHA256。实际产物以本轮新构建为准。

## 8. P2-1 结论与下一步

- 固定源码 commit、审计 commit、集成分支 HEAD、构建脚本、补丁清单、原生配置入口和输出命名已完成 GitHub 静态核查。
- 当前补丁只有一项；没有发现第二项隐藏的 PA-220 patch。
- 当前尚未验证用户构建机和 OCTEON SDK 工具链，不能声称 P2-2 完成。
- 下一步先完成 P2-2：用户提供实际构建机和工具链路径/版本输出；再决定是否需要对构建脚本作最小、独立、可审查的可重复构建改进，然后执行全新 checkout 的干净构建。
- 不执行 `saveenv`、EEPROM/TLV 写入、CPLD 写入、SPI/eMMC/NAND/USB block 写入或固件安装。P1 确认的 `__tmp_load` 边界和统一 RAM validator 要求保持不变；validator 未实现不阻塞本阶段基础构建。
