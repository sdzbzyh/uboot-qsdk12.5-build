# PA-220 U-BootKit 项目文档

本目录是 PA-220 U-BootKit 移植项目的正式文档入口。项目目标是在 PA-220 原生 OCTEON U-Boot 上，系统评估并移植 uBootKit 中可适配的维护功能；不把 Qualcomm/IPQ 专有实现直接搬入 MIPS 平台。

## 首选入口

- [当前执行上下文](EXECUTION-CONTEXT.md)：新会话只需从这里恢复当前 HEAD、唯一主线、下一步和协作规则。
- [Web UI 设计目标](WEB-UI-DESIGN.md)：uBootKit Argon 风格参考、PA-220 页面规范与功能边界。
- [P6 RAM-only 接收计划](P6-PLAN.md)：接收生命周期、长度/校验/格式识别及实机测试安排。

## 文档索引

| 文档 | 内容 |
|---|---|
| [PROJECT.md](PROJECT.md) | 项目章程、目标、范围、仓库与版本基线 |
| [FEATURE-MATRIX.md](FEATURE-MATRIX.md) | uBootKit 功能分类、PA-220 适配性与验证状态 |
| [ROADMAP.md](ROADMAP.md) | 节点划分、依赖关系与完成条件 |
| [DECISIONS.md](DECISIONS.md) | 已确认的架构、安全和协作决策 |
| [TEST-PLAN.md](TEST-PLAN.md) | 编译、静态检查、RAM 启动及硬件验证策略 |
| [P5-2C 实机验收](../../reports/pa220/P5-2C/ACCEPTANCE.md) | HTTP/TCP 验收证据 |
| [P4-2 validator / TFTP guard 验收](../../reports/pa220/P4-2/IMPLEMENTATION.md) | RAM validator 与 DATA guard 的 Host/目标构建证据 |
| [P6-0 源码审查](../../reports/pa220/P6-0-SOURCE-REVIEW.md) | P6 源码审查和失败状态缺口 |

## 当前检查点

- 集成分支：`feature/pa220-ubootkit-web`
- 当前执行基线（本文件更新前）：`8105ff0cf1da7e3aaf5c2b9c6b628947eb260be6`
- P6-0 已完成；P6-1A 是下一项生产代码工作：修复 TFTP GET 开始前旧 `filesize` / `fileaddr` 失效失败状态。
- 本文档不重复历史验收数字；历史结果以各自正式报告为准。
- P6 首条接收路径仍为 TFTP GET/RRQ；实际接收必须在对应软件安全门和 live RAM reservation 核验完成后安排。
- HTTP 服务当前已验收的范围不等于完整 Web 前端已交付；最终页面要求见 [WEB-UI-DESIGN.md](WEB-UI-DESIGN.md)。

## 状态约定

- **已确认**：有源码、配置、历史提交或明确硬件资料支持。
- **已实现-待验证**：代码已存在，但尚无足够构建/实机证据。
- **可适配-待实现**：平台能力和设计路径初步成立，尚未完成实现。
- **需研究**：依赖硬件、协议、内存布局或闭源启动阶段的进一步调查。
- **不适用**：依赖 Qualcomm/IPQ 专有硬件或启动机制，不能直接作为 PA-220 功能移植。
- **实机验证通过**：必须附设备、镜像版本、步骤和原始结果；仅编译通过不算。

历史阶段记录保留为审计轨迹。新会话以“当前执行上下文”为入口，不需要重复复述已经确认完成的阶段。
