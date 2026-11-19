# PA-220 U-BootKit 测试计划

## 1. 测试阶段

### A. 静态审计

- 核对 PA-220 固定源码 commit 和集成补丁。
- 检查配置项、命令注册、驱动依赖、链接地址、镜像头和内存布局。
- 检查补丁能否在干净源码树应用、重复执行是否幂等。
- 对所有新增命令审查副作用：环境写入、SPI/eMMC 写入、CPLD 写入、复位、看门狗和网络状态修改。

### B. 构建验证

建议命令：

```sh
./build-pa220.sh /path/to/tools-gcc-4.7 2>&1 | tee pa220-build.log
```

记录：
- 集成仓库 commit 与分支。
- PA-220 源码 commit。
- OCTEON 工具链版本与路径。
- 补丁列表及应用结果。
- 完整构建日志、退出码、产物大小和 SHA-256。
- 镜像头的 board type、image type、长度和 CRC。

### C. RAM 启动验证

- 通过原厂 U-Boot TFTP 加载镜像到已确认的 RAM 测试地址。
- 按设备安装资料执行必要的 I-cache 操作和跳转。
- 确认 pa-220# 提示符、串口速率、DRAM 信息和 autoboot 行为。
- 记录串口完整日志。
- 不执行 saveenv，不执行 sf erase/write/update，不写 CPLD。

### D. 外设只读验证

逐项测试：
- MGT Ethernet 链路、DHCP/TFTP/Ping。
- eMMC 容量、分区表、只读文件访问。
- SPI NOR JEDEC ID、容量和只读内容读取。
- CPLD 只读寄存器、板载 EEPROM/MAC。
- USB 控制器枚举与 Mass Storage。
- 温度/LED 状态读取。

任何探测命令都必须先检查是否有写寄存器或清状态副作用。

### E. 服务与异常路径验证

- Web 服务启动/退出后 CLI 和 autoboot 能恢复。
- 错误请求、超长请求、断开连接、超时和重复连接不导致崩溃。
- 文件上传只写 RAM 缓冲区；长度上限、CRC/SHA 错误和空间不足均有安全失败路径。
- Web 初期没有持久化写操作入口。

### F. 存储写入验证（后续独立授权）

- 只在专门的可恢复测试阶段进行。
- 写前展示设备、分区、偏移、长度和备份状态。
- 对写入范围做整数溢出和边界检查。
- 写后读回并校验。
- 预先演练恢复流程。
- SPI NOR 和 CPLD flash-select 绝不混入普通固件安装流程。

## 2. 结果状态

每个测试用例只能使用以下结果：
- NOT RUN：未执行。
- PASS：按记录步骤执行且结果符合预期。
- FAIL：结果不符合预期。
- BLOCKED：因缺少硬件、工具链或资料无法执行。
- NOT APPLICABLE：该平台无对应硬件/功能。

## 3. 证据要求

每项 PASS 必须保留：
- 测试设备/硬件修订信息（可识别范围内）。
- 软件 commit、镜像 SHA-256 和工具链版本。
- 操作命令与原始串口/终端输出。
- 测试前提、预期结果、实际结果。
- 是否触及持久化存储。

无原始结果的口头判断不得作为实机通过证据。

## 4. P2 实际结果（2026-11-19）

详细证据：`reports/P2-COMPLETION-2026-11-19.md`。

| 测试项 | 结果 | 说明 |
|---|---|---|
| 固定输入与工具链 | PASS | 固定源码/补丁/工具链已核对；最小 OCTEON 目标编译与链接通过 |
| 干净构建 | PASS | 固定源码全新 checkout 构建退出码 0 |
| ELF/BIN/map 产物 | PASS | 文件生成；大小、头信息、关键符号和 SHA256 已记录 |
| ELF loader / Cache / named-block 编入 | PASS | 对象文件、linker map 和最终 ELF 符号表均有直接证据 |
| 新版 Stage3 RAM 启动 | PASS | 原厂 Stage2/TFTP 加载后成功进入 U-Boot 命令行 |
| 旧版 RAM 启动对照 | PASS | 旧版 U-Boot 同样可从 RAM 启动 |
| `__tmp_load` 运行时 descriptor | NOT RUN | 转入 P3 只读诊断；未确认前不得开放任意地址上传 |
| `pa220 status/env` 设备运行结果 | NOT RUN | 转入 P3 只读诊断 |
| TLV EEPROM/I²C 只读窗口 | NOT RUN | 转入 P3；禁止任何写操作 |
| RAM 镜像启动后 CRC 变化 | OBSERVATION | 新旧 U-Boot 均观察到变化；不是失败判据，原因未定，暂不追查 |
| LOAD MemSiz 与 `uboot_end` 4-byte 差异 | DEFERRED | 非阻断性链接布局疑点，后续统一处理 |

注：`OBSERVATION` 与 `DEFERRED` 是阶段问题记录，不替代第 2 节规定的测试结果状态。P2 已收尾；P3 继续补齐只读运行时诊断。任何持久化介质写入仍需独立阶段审查和明确授权。

## 5. P3 实际结果（2026-11-19）

详细原始输出、构建产物和地址区间核算见 `reports/P3-START-2026-11-19.md`。

| 测试项 | 结果 | 说明 |
|---|---|---|
| P3 补丁应用 | PASS | 固定 PA-220 commit 上 `git apply --check` 成功；两个 hunk 行数已核验 |
| P3 构建 | PASS | OCTEON SDK GCC 4.7.0 构建成功，镜像头 CRC/长度及 ELF/MAP/BIN SHA256 已记录 |
| P3 ELF/MAP | PASS | 新命令已进入最终 ELF；入口和 LOAD 起点保持 0xc0000000 |
| P3 Stage3 RAM 启动 | PASS | 原厂 Stage2/TFTP 加载 P3 镜像后成功进入 U-Boot CLI |
| `help pa220` | PASS | 正常列出 status/env/memory |
| `pa220 status` | PASS | 板型、DRAM、CPU clock、Ethernet、CPLD/PHY/LED 只读状态正常显示 |
| `pa220 env` | PASS | SPI NOR 环境后端和布局信息正常显示；未执行 saveenv |
| `pa220 memory` | PASS | loadaddr 与 `__tmp_load` base 均为 0x20000000；size 为 0x10000000 |
| `bdinfo` / `meminfo` | PASS | DRAM 容量均报告 8 GiB；memstart/address-domain 关系仍待解释 |
| `namedprint` / `freeprint` | PASS | 13 个 named blocks、两个 free blocks；关键相邻边界经半开区间核算一致 |
| TLV EEPROM/I2C 只读内容 | NOT RUN | 下一批只读检查；先确认命令语义和有效访问窗口 |
| 网络/存储扩展只读诊断 | NOT RUN | P3 后续工作 |
| 任意地址 RAM 上传安全性 | BLOCKED | cached/uncached/physical alias 与统一 pre-write validator 尚未完成；不得开放上传 |

本轮没有执行任何持久化介质写入、named block 分配/释放或 DRAM 测试。P3 整体仍进行中。

## 6. P3 只读诊断源码审计（2026-11-19）

详细记录：`reports/P3-READONLY-AUDIT-2026-11-19.md`。

| 测试/审计项 | 结果 | 说明 |
|---|---|---|
| 板级 descriptor/MAC 缓存 API | PASS（源码审计） | 可读 global data；无需重新访问 EEPROM |
| 板级初始化副作用边界 | PASS（源码审计） | CPLD LED、USB GPIO、QLM 等初始化写路径已识别 |
| CORE/IO/DDR 时钟 API | PASS（源码审计） | API 返回 global data 值，不是独立频率测量 |
| 网络环境配置 | PASS（既有实机结果） | ethact/ethprime/IP 配置已显示 |
| 网络物理 link 状态 | NOT VERIFIED | cvmx_helper_link_get() 会更新 link LED 并检查 SFP，不作为纯只读查询 |
| SPI NOR probe | NOT RUN | Winbond W25Q128 配置下 sf probe 仅列为后续只读候选 |
| eMMC 初始化/信息查询 | BLOCKED | mmc_init/startup 可能通过 CMD6 修改 EXT_CSD 配置字段 |
| USB Mass Storage 枚举 | NOT RUN | USB 电源 GPIO 默认拉低；usb start 初始化副作用尚未审完 |
| 新增 board/clock 命令 | NOT BUILT | 下一增量候选 |
| P3 整体 | IN PROGRESS | 不重复既有 status/env/memory 和 bootmem 测试 |

本轮仅源码审计和文档记录；未构建、未运行新设备命令、未访问或修改持久化介质。
