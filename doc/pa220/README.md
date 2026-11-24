# PA-220 U-BootKit 项目文档

本目录是 PA-220 U-BootKit 移植项目的正式文档入口。项目目标是在 PA-220 原生 OCTEON U-Boot 上，系统评估并移植 uBootKit 中可适配的维护功能；不把 Qualcomm/IPQ 专有实现直接搬入 MIPS 平台。

## 文档索引

| 文档 | 内容 |
|---|---|
| [PROJECT.md](PROJECT.md) | 项目章程、目标、范围、仓库与版本基线 |
| [FEATURE-MATRIX.md](FEATURE-MATRIX.md) | uBootKit 功能分类、PA-220 适配性与验证状态 |
| [ROADMAP.md](ROADMAP.md) | 节点划分、依赖关系与完成条件 |
| [DECISIONS.md](DECISIONS.md) | 已确认的架构、安全和协作决策 |
| [TEST-PLAN.md](TEST-PLAN.md) | 编译、静态检查、RAM 启动及硬件验证策略 |
| [reports/P0-2026-11-18.md](reports/P0-2026-11-18.md) | P0 节点报告与仓库审计结果 |
| [P5-2C 实机验收](../../reports/pa220/P5-2C/ACCEPTANCE.md) | P5-2C 云端、Host 与 PA-220 实机 HTTP/TCP 验收及证据边界 |
| [P6 RAM-only 接收计划](P6-PLAN.md) | 以 TFTP GET 为首个路径的 RAM-only 接收预检、验收与安全边界 |
| [P4-2 validator / TFTP guard 验收](../../reports/pa220/P4-2/IMPLEMENTATION.md) | 统一 RAM validator、TFTP DATA pre-write guard 的 Host/目标构建证据及未完成事项 |

## 当前项目检查点（2026-11-24）

- 集成分支：`feature/pa220-ubootkit-web`。
- P5-2C：云端构建、Host Harness 和 PA-220 RAM 启动后的实机 HTTP/TCP 验证已完成；正式结果以 [P5-2C ACCEPTANCE](../../reports/pa220/P5-2C/ACCEPTANCE.md) 为准。
- P4-2：生产 RAM validator Host 42/161 与 OCTEON N32/full U-Boot link PASS；TFTP DATA guard Host 14/64，目标 `tftp.o` 编译并进入 `libnet.o`。正式结果以 [P4-2 IMPLEMENTATION](../../reports/pa220/P4-2/IMPLEMENTATION.md) 为准。
- 真实 TFTP 接收尚未测试；PA-220 仍保持断电，尚未做 live `__tmp_load` descriptor 只读核对。
- HTTP 仍只读，HTTP 上传与 ELF segment 写入保持 BLOCKED；DHCP/Ping 实机验证仍未完成。
- P6 已进入设计/验收准备；首个路径选为 TFTP GET。任何实际传输前，必须先做设备只读 descriptor / reserved-block 核对并确认 autoload=n；当前仍禁止实际接收和持久化写入。详见 [P6-PLAN](P6-PLAN.md)。

## 相关既有文档

- [PA-220 独立构建说明](../PA220.md)
- [PA-220 维护功能盘点与路线草案](../PA220-MAINTENANCE.md)
- [集成仓库 README](../../README.md)

## 状态约定

- **已确认**：有源码、配置、历史提交或明确的硬件资料支持。
- **已实现-待验证**：代码已存在，但尚无足够的构建/实机证据。
- **可适配-待实现**：平台能力和设计路径初步成立，尚未完成实现。
- **需研究**：依赖硬件、协议、内存布局或闭源启动阶段的进一步调查。
- **不适用**：依赖 Qualcomm/IPQ 专有硬件或启动机制，不能作为 PA-220 功能移植。
- **实机验证通过**：必须附带设备、镜像版本、测试步骤和原始结果；仅编译通过不算。

所有涉及持久化存储的功能，必须单独标注风险与验证状态。默认遵循 RAM-only 测试原则。


## P6-0 源码审查与当前交接（2026-11-24）

- 当前远端分支 HEAD（审查开始时）：`1a318fd562cdb3b475d92abdc5768b436cf154fd`；用户此前记录的 `4168cbd5...` 已是祖先检查点，P4-2 验证提交为 `20acf979...`。
- P4-2 三次 Actions（#45346000355、#45374041373、#45375848020）均已核实成功；最新 artifact 为 `14910393863`，SHA-256 与 P4-2 报告一致。
- P6-0 已完成当前分支 validator、固定 PA-220 TFTP 源码、构建入口、生成器和正式文档审查。报告：[P6-0 SOURCE REVIEW](../../reports/pa220/P6-0-SOURCE-REVIEW.md)。
- P6 当前仍为设计/预检准备；live `__tmp_load` descriptor NOT RUN，TFTP GET 实际接收 NOT RUN。设备报告为断电，本轮未要求上电。
- P6 首个路径仍为 TFTP GET；HTTP 保持只读。TFTP Server/WRQ、PUT、HTTP 上传和 ELF segment 写入均未获得实机接收验收。
- 当前执行容器没有本地仓库 checkout，因此用户 Debian 工作区的 clean/dirty 状态未核实；远端分支 SHA 和提交差异已核实。
