# PA-220 当前执行上下文

> 本文件是新会话的首选入口。只记录当前有效状态、下一步和协作规则；历史阶段证据仍以各自正式报告为准，不在每次交接中重复展开。

## 当前检查点

- 集成仓库：`sdzbzyh/uboot-qsdk12.5-build`
- 分支：`feature/pa220-ubootkit-web`
- 本文件建立时 HEAD：`8105ff0cf1da7e3aaf5c2b9c6b628947eb260be6`（历史记录；当前执行 HEAD 见最新提交）
- PA-220 固定源码：`sdzbzyh/pa220-openwrt@801717ba6a8a59ed2cfd53bf3e1b19841fecd2e8`
- uBootKit 评估基线：`chenxin527/uboot-qsdk12.5-build@9ce315735ac88fd0a7c7697bc527a8616ed0a972`
- 目标：OCTEON III CN7130、MIPS64 BE、N32、Cavium SDK 5.1.0、U-Boot 2013.07、原厂 Stage 2 加载自定义 Stage 3。
- PA-220 独立构建入口：`./build-pa220.sh`；不得混入 Qualcomm `build.sh`。

## 当前唯一主线

P6 RAM-only 文件接收。首条接收路径为 TFTP GET/RRQ。

P6-0 源码审查已完成。P6-1A（TFTP GET 开始前失效旧 `filesize` / `fileaddr`）已完成生产实现、Host/ASan/UBSan、OCTEON object、N32 relocatable link、完整 U-Boot final link 和补丁生成器重复运行验证。

- 实现提交：`dafb893a7f3a0637159b87b59def1618184efdad`
- 最终验收 Actions：[#45588666611](https://github.com/sdzbzyh/uboot-qsdk12.5-build/actions/runs/45588666611)，结论 SUCCESS。
- 正式验收报告：`reports/pa220/P6-1A/ACCEPTANCE.md`。
- 本轮未执行真实 TFTP GET、未传输文件、未调用 `saveenv`。P6 整体仍未开放实际接收。

下一唯一工作单元：P6-1B，完成完整文件 reservation、最大总长度和接收失败状态管理的生产设计/Host/目标验证；不要跳到真实传输。

### P6-1A 设计要点

固定源码 `u-boot/common/cmd_nvedit.c` 中：

- `setenv(name, NULL)` 或空值表示删除。
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

继续按 `doc/pa220/P6-PLAN.md` 推进：完整文件 reservation、最大长度、checked block/offset arithmetic、超时/中断失败状态、CRC32、SHA-256、只读格式识别、数据同步和失败数据无效化。不要把单 DATA block validator 当作完整文件生命周期保证。

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
