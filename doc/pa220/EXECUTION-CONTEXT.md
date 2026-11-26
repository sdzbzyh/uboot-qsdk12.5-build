# PA-220 当前执行上下文

> 本文件是新会话的首选入口。只记录当前有效状态、下一步和协作规则；历史阶段证据仍以各自正式报告为准，不在每次交接中重复展开。

## 当前检查点

- 集成仓库：`sdzbzyh/uboot-qsdk12.5-build`
- 分支：`feature/pa220-ubootkit-web`
- 本文件建立时 HEAD：`8105ff0cf1da7e3aaf5c2b9c6b628947eb260be6`（历史记录）
- P6-1C 验收与计划文档更新基线 HEAD（本文件本次更新前）：`7503f421e82dc16f0343b4092eb86458a181647f`
- PA-220 固定源码：`sdzbzyh/pa220-openwrt@801717ba6a8a59ed2cfd53bf3e1b19841fecd2e8`
- uBootKit 评估基线：`chenxin527/uboot-qsdk12.5-build@9ce315735ac88fd0a7c7697bc527a8616ed0a972`
- 目标：OCTEON III CN7130、MIPS64 BE、N32、Cavium SDK 5.1.0、U-Boot 2013.07、原厂 Stage 2 加载自定义 Stage 3。
- PA-220 独立构建入口：`./build-pa220.sh`；不得混入 Qualcomm `build.sh`。

## 当前唯一主线

P6-A live DRAM descriptor / reservation read-only preflight 已 PASS；下一工作单元为 P6-B 首个 RAM-only TFTP GET 的测试准备与用户确认。

- P6-1A / P6-1B / P6-1C 软件门均已验收，历史证据保留在各自 ACCEPTANCE 报告，不在此重复。
- P6-A 实机预检报告：`reports/pa220/P6-A/READONLY-PREFLIGHT.md`。
- 设备实际报告 8 GiB DRAM；`__tmp_load=[0x20000000,0x30000000)`，256 MiB；`loadaddr=0x20000000`；`autoload=n`。
- MIPS/CN70XX 地址域已与源码 DDR map 对齐；Linux reserved、low/FDT/exception、free block 和高端 U-Boot reservation 均不与 load block 重叠。
- P6-1B 的 32 MiB 完整接收 reservation 可容纳于 live 256 MiB descriptor；生产 validator 在 RRQ 前仍会再次核对 live descriptor 与 reservation globals，任何不一致均 fail closed。
- 尚未执行真实 TFTP GET、未传输文件、未修改网络环境变量、未调用 saveenv、未写持久化介质。

P6-B 仍需单独明确确认后才开始。当前 `ipaddr` / `serverip` 未定义不影响 P6-A；它们是设备作为 TFTP client 时的网络参数，与最终由 PA-220 DHCP Server 给维护电脑分配地址是不同需求。

### P6-1A 设计要点

固定源码 `u-boot/common/cmd_nvedit.c` 中：

- 只有 `setenv(name, NULL)` 表示删除；空字符串不是删除请求，不得用它代替 `NULL`。
- 删除成功返回 0；删除失败（包括变量不存在）返回 1。
- `hdelete_r()` 对不存在的变量返回 0，因此必须先用 `getenv()` 判断变量是否存在。
- 对存在的变量删除时，必须检查 `setenv()` 返回值。
- `filesize` 与 `fileaddr` 必须分别尝试失效；第一项失败也必须继续尝试第二项。
- 只有确认两个变量均不存在时才允许 GET 进入 RRQ/DATA 生命周期。
- 任一失效失败均 fail closed；不调用 `saveenv`。
- 只影响 TFTP GET/RRQ；不得改变 PUT 或 Server/WRQ 行为。
- 成功接收后仍由正常 `NETLOOP_SUCCESS` 路径发布新元数据。

Host 测试直接编译生产 helper，并从同一生成器注入生产 GET gate 代码；使用 mock 环境接口和失败注入，覆盖两项删除成功、各自失败、首项失败仍执行第二项、禁止启动 GET、成功失效后允许 GET、不调用 saveenv。正常 NETLOOP_SUCCESS 元数据发布路径未修改；PUT 与 Server/WRQ 分支未被补丁触及。

### 后续 P6 软件门

继续按 `doc/pa220/P6-PLAN.md` 推进：P6-1A/B/C 软件门均已完成。下一步只做 P6-A live descriptor、DRAM 范围和完整 reservation 的只读预检；本轮不执行 TFTP GET。

## 验证与补丁规则

- 任何修改前先重新读取远端分支 HEAD；不得覆盖其他会话提交。
- 固定源码和 SDK 必须使用上面的精确版本。
- 生产补丁由可重复生成器从真实源码差异生成；统一 diff 必须由 `git diff --binary` 产生。禁止手工编写或手算 hunk 行数。
- 建立独立 P6 Actions 工作流；源码/测试/生成器/build 变化触发，纯文档变更不触发目标构建。
- 报告实际 Host、ASan/UBSan、OCTEON object、N32 link、final ELF/map 和 Actions 结果；没有真实运行证据不得写 PASS。
- 复用已有有效验证证据，不为“证明历史完成”重复跑回归。

## 实机测试安排

用户已授权：项目推进过程中如确有必要，可以安排实机测试；完成一轮测试后设备断电。

执行原则：
- 不因旧交接提示中的 blanket 禁止语句而机械阻断低风险、必要的只读或 RAM-only 测试；按当前代码、设备状态和实际风险制定最小步骤。
- 先完成对应软件 Host/目标验证，再安排与该阶段直接相关的实机测试。
- RAM-only TFTP GET 仍须先确认 live `__tmp_load` descriptor、完整 reservation、目标地址/长度和接收失败状态可靠；不允许猜测固定地址。
- 每轮给出明确命令、预期结果、停止条件和串口/网络证据要求；完成该轮后指导设备断电。
- 持久化介质写入、环境持久化、EEPROM/CPLD 修改、刷写、擦除等破坏性操作不包含在上述常规实机测试授权中；确实需要时再说明具体对象、风险和回滚方案，取得针对该操作的明确同意。
- 不把 RAM-only 接收自动扩展为 PUT、WRQ、HTTP upload 或 ELF segment 写入。

## 最终维护网络交付要求

最终版本必须由 PA-220 在隔离的 MGT 维护接口上提供 DHCP Server，为直连维护电脑自动分配地址；不能只实现 U-Boot 自己作为 DHCP Client 获取地址。还需审查并实现对上游 DHCP OFFER/ACK 的隔离/过滤，避免上游 DHCP 报文干扰维护电脑。PA-220 管理地址、地址池、接口绑定及过滤策略应在网络阶段单独设计、Host/目标验证和实机验收；不得把此功能混入 P6-A/P6-B，也不得为了启动维护服务自动调用 saveenv。

## Web UI 目标

最终 PA-220 Web UI 必须以 uBootKit 的 Argon 风格页面作为视觉/交互参考，要求见 [WEB-UI-DESIGN.md](WEB-UI-DESIGN.md)。P5-2C 的 HTTP/TCP 服务验收不等于完整前端已经完成。Web 前端作为独立后续工作流规划，不得为了追求视觉一致性而开放命令执行、环境变量持久化修改、升级/刷写、重启或其他未经审查的写接口。

## 新会话交接规则

新提示词只写：
1. 本文件链接；
2. 当前远端 HEAD；
3. 当前唯一未完成工作单元；
4. 本轮需要的具体动作；
5. 需要用户提供的现场输入（仅在确实需要时）。

不要把已完成阶段、旧测试数字、所有历史安全条款整段复制进提示词。历史资料按需从本文件链接的正式文档和报告读取。

### P6-1B：完整文件 reservation 与接收失败状态 — SOFTWARE ACCEPTED（2026-11-25）

- 固定 PA-220 源码 `801717ba6a8a59ed2cfd53bf3e1b19841fecd2e8`，集成验证提交 `fc6f60ca28418f74a418456cd1144a5c4f0f017a`。
- 默认最大文件长度 32 MiB（`CONFIG_PA220_TFTP_MAX_FILE_SIZE` 可覆盖）；在 RRQ 发送前对整个最大半开区间执行 P4-2 validator reservation。
- DATA 写入前校验连续 offset、剩余 reservation 和长度；提交接收字节数时再次 checked arithmetic；TFTP `tsize` 采用有上限的十进制解析。
- 状态明确区分 IDLE/ACTIVE/COMPLETE/FAILED。reservation、越界/乱序、超长、写入失败、TFTP ERROR、超时、ICMP 中断、完成长度/tsize 不一致以及 `do_tftpb()` 返回时的 Ctrl-C/abort 均进入失败状态并清零部分长度、再次失效旧元数据。
- PUT 与 Server/WRQ 不调用 GET helper；PUT 的旧 tsize 解析和行为保留；不调用 `saveenv`。
- Host 14 cases / 26 assertions / 0 failures；GCC 与 ASan/UBSan 均 PASS。
- Cavium SDK 5.1.0 / GCC 4.7.0；OCTEON `tftp.o`、`cmd_net.o`、N32 relocatable link、完整 U-Boot final link 均 PASS。
- Actions [#45595798808](https://github.com/sdzbzyh/uboot-qsdk12.5-build/actions/runs/45595798808) SUCCESS；完整证据见 `reports/pa220/P6-1B/ACCEPTANCE.md`。
- 独立补丁由 `git diff --binary` 生成：TFTP 接收状态补丁 SHA-256 `e3dba0c7fdfce6364a93998d94ea823a8d0307e92656b51a4619e7ffeca509b5`；GET abort hook 补丁 SHA-256 `71a1e403dd0988add42c8b229332dc6379e65c437de3c1e057158977398c8ccc`。
- 未执行真实 TFTP GET、未传输文件、未调用 `saveenv`、未写入任何持久化介质。

P6-1C 已由 Actions #45600860179 验收通过，报告见 reports/pa220/P6-1C/ACCEPTANCE.md。下一步 P6-A：只读检查 live __tmp_load descriptor、DRAM 范围和完整 reservation。未完成 P6-A 前不执行真实 TFTP GET。