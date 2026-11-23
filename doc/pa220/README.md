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

## 当前项目检查点（2026-11-23）

- 集成分支：`feature/pa220-ubootkit-web`。
- P5-2C：云端构建、Host Harness 和 PA-220 RAM 启动后的实机 HTTP/TCP 验证已完成；正式结果以 [P5-2C ACCEPTANCE](../../reports/pa220/P5-2C/ACCEPTANCE.md) 为准。
- 实机证据目录：[P5-2C physical-evidence](../../reports/pa220/P5-2C/physical-evidence/)。其中 `pa220-serial.log` 是会话记录重建件，不是原始 UART 录制。
- 下一项工作应先处理统一 RAM 写入前范围验证器；在其完成并验证前，不得开放 TFTP/HTTP 文件接收或 ELF segment 写入。

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