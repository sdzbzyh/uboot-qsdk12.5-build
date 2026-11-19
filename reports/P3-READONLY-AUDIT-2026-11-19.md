# PA-220 P3 board/clock 只读诊断实机验证（2026-11-19）

## 1. 验证范围与前提

- 项目阶段：P3 CLI / 只读诊断完善。
- 补丁：`patches/pa220/0002-pa220-board-clock-readonly-diagnostics.patch`。
- 固定 PA-220 源码：`IronDemiGod/pa220-openwrt` commit `801717ba6a8a59ed2cfd53bf3e1b19841fecd2e8`。
- 集成仓库分支：`feature/pa220-ubootkit-web`。
- 本轮开始时，0002 已应用、已构建，新镜像已通过原厂 Stage2/TFTP 成功 RAM 启动。
- 本轮不重新构建、不重新 RAM 启动，不重复 status/env/memory、bootmem、TLV 等既有测试。
- 本轮仅执行 `help pa220`、`pa220 board`、`pa220 clock`。

## 2. 实机结果

| 测试项 | 结果 | 证据/说明 |
|---|---|---|
| `help pa220` | PASS | 正常列出 status/env/memory/board/clock 五个子命令 |
| `pa220 board` 命令执行 | PASS | 正常返回提示符，无崩溃或错误 |
| Board type | PASS | 20021（PA220） |
| Revision | PASS | 1.1 |
| Serial | PASS | 012801031163 |
| Board descriptor 状态 | PASS | 显示 from initialized descriptor |
| MAC base/count 显示 | PASS | 08:30:6b:e1:be:00 / 255，格式正常 |
| MAC descriptor 来源判定 | NOT RUN | 输出仅说明可能来自 EEPROM 或 generated fallback；没有单独显示 MAC fallback 标志，不能据此断言来源 |
| `pa220 clock` 命令执行 | PASS | 正常返回提示符，无崩溃或错误 |
| CORE 单位换算 | PASS | 1,000,000,000 Hz = 1000 MHz |
| SCLK 单位换算 | PASS | 500,000,000 Hz = 500 MHz |
| DDR 单位换算 | PASS | 800,000,000 Hz = 800 MHz base |
| DDR 数据速率换算 | PASS | 800 MHz × 2 = 1600 MT/s |
| 时钟数据语义提示 | PASS | 明确标注 initialized global-data/configuration values, not independent measurements |
| DDR 来源提示 | PASS | 显示 valid EEPROM DDR clock descriptor, otherwise PA-220 fallback; source is not retained |
| 异常/越界/格式化问题 | PASS（观察范围内） | 三条命令输出完整、格式正常并返回 CLI；本次黑盒输出不能替代全面内存安全证明 |

## 3. 完整原始串口输出

完整逐字记录另存于：

`reports/P3-BOARD-CLOCK-2026-11-19-serial.log`

## 4. 只读与安全边界

本轮执行的三个命令为诊断显示命令。根据补丁实现，board 命令读取初始化期间缓存于 global data 的 board/MAC descriptor；clock 命令读取 global-data clock 值。没有要求重新扫描 EEPROM，也没有执行任何持久化写入。

本轮没有执行：
- `saveenv`；
- EEPROM/TLV、CPLD 写入；
- SPI NOR/eMMC/NAND 写入或擦除；
- `bootloaderupdate`；
- `namedalloc` / `namedfree`；
- `mtest`；
- 任意地址 RAM 上传。

本轮验证不构成任意地址 TFTP/HTTP/ELF 接收安全性证明。该能力仍保持 BLOCKED，直到地址别名、边界及统一 pre-write validator 完成独立审计。

## 5. 结论

0002 新增的 `help pa220`、`pa220 board`、`pa220 clock` 在当前 RAM 启动镜像上完成实机验证，命令执行与显示结果均 PASS。MAC descriptor 的实际来源没有由当前输出单独证明，记为 NOT RUN；不影响命令基本功能通过。

本报告覆盖 board/clock 的实机验证；与同日 P3 已完成的其他证据合并后，**P3 CLI/只读诊断阶段于 2026-11-19 正式 COMPLETE**。其他检查的状态不因本报告而改变：Ethernet 物理 link NOT VERIFIED，eMMC 初始化 BLOCKED，USB 枚举 NOT VERIFIED；统一 RAM 写入 validator 尚未实现，任意地址 TFTP/HTTP/ELF 接收继续 BLOCKED。

P3 全阶段证据还包括 `pa220 status/env/memory`、`bdinfo`、`printenv loadaddr`、named/free bootmem、TLV EEPROM 五个 tuple/checksum/END/终止偏移只读核查及 SPI NOR W25Q128BV 只读识别。详见集成仓库 `doc/pa220/TEST-PLAN.md` 第 7 节。原始 board/clock 串口日志为本仓库 `reports/P3-BOARD-CLOCK-2026-11-19-serial.log`。

P3 COMPLETE 仅表示本阶段已实现范围和证据闭环，不表示所有可选硬件诊断均 PASS，也不表示 P1 非阻断风险已解决。


## 6. 后续历史证据审计覆盖说明（2026-11-19）

本报告仅记录 0002 board/clock 三条命令的实机结果和其 24 行原始串口日志；它不单独证明 P3 其他测试的原始证据均已归档。此前报告中的 P3 COMPLETE 是当日阶段判断。P3 全历史证据审计目前 **IN PROGRESS**，在账本与最终审计报告明确完成前，不得将本次历史审计标记 COMPLETE，也不得开始 P4 实际开发。详见 `doc/pa220/reports/P3-HISTORICAL-EVIDENCE-AUDIT-2026-11-19.md` 与 `P3-HISTORICAL-AUDIT-CHECKPOINT.md`。
