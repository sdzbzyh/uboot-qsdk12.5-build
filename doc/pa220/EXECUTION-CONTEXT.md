# PA-220 当前执行上下文

> 本文件是新会话的首选入口。只记录当前有效状态、下一步和协作规则；历史阶段证据仍以各自正式报告为准，不在每次交接中重复展开。

## 当前检查点

- 集成仓库：`sdzbzyh/uboot-qsdk12.5-build`
- 分支：`feature/pa220-ubootkit-web`
- P7-1 代码/测试检查点：`1b86368b69fe293a1a8193b76aa7d75a01d215e7`（后续为 P7 文档归档更新）
- 本文件建立时 HEAD：`8105ff0cf1da7e3aaf5c2b9c6b628947eb260be6`（历史记录）
- P6-1C 验收与计划文档更新基线 HEAD（本文件本次更新前）：`7503f421e82dc16f0343b4092eb86458a181647f`
- PA-220 固定源码：`sdzbzyh/pa220-openwrt@801717ba6a8a59ed2cfd53bf3e1b19841fecd2e8`
- uBootKit 评估基线：`chenxin527/uboot-qsdk12.5-build@9ce315735ac88fd0a7c7697bc527a8616ed0a972`
- 目标：OCTEON III CN7130、MIPS64 BE、N32、Cavium SDK 5.1.0、U-Boot 2013.07、原厂 Stage 2 加载自定义 Stage 3。
- PA-220 独立构建入口：`./build-pa220.sh`；不得混入 Qualcomm `build.sh`。

## 当前唯一主线（2026-11-27，P7-2）

P7-1 parser、Host/ASan/UBSan、OCTEON 完整构建和 PA-220 实机 preview 均已通过。当前推进 P7-2：FIT 容器语义与原机 SPI/eMMC 存储布局取证。

- P7 计划：`doc/pa220/P7-PLAN.md`。
- P7-1 验收：`reports/pa220/P7-1/ACCEPTANCE.md`、`DEVICE-ACCEPTANCE.md`。
- P7-2A SPI NOR 静态分析 PASS：`reports/pa220/P7-2/SPI-IMAGE-ANALYSIS.md`。
- P7-2 源码审查：`reports/pa220/P7-2/SOURCE-REVIEW.md`。
- 原机 SPI 备份仅从 `main@da755efd49df32f3e85d231745212674737feba5` 读取；16 MiB，SHA-256 `4899a1c5ed06e5eafbb8c92625a4afae14cad399806df13979fd2c70ee1ed922`。4 个 OCTEON header 均通过 header/data CRC；识别 Pre-U-Boot、Stage 2 和 NOR bootloader；两个 FDT 分别描述 generic SPI Stage2 与 PA-220 Stage3。
- SPI 备份的 `0x250000-0x1000000` 全为 `0xff`；配置环境区 `0xe00000` / `0xe10000` 在备份中也全擦除。该镜像不提供 eMMC GPT 证据。
- 重要配置：PA-220 实际启用 SPI redundant environment（`0xe00000` / `0xe10000`），`saveenv` 会擦写 SPI NOR 64 KiB sector；板级文件顶部“RAM-only”注释与宏配置矛盾。P7-2 禁止调用 `saveenv`。
- P7-2C live eMMC 只读取证已完成并归档：`reports/pa220/P7-2/EMMC-DEVICE-ACCEPTANCE.md`。实机介质是 DOS/MBR + extended/EBR logical partitions，不是 GPT；live `mmc part` 的 8 个分区条目与完整 User Area 镜像的离线 fdisk/parted/sfdisk 记录一致。
- 已有 live 根目录探测：0:1、0:2 于 2026-11-27；0:3、0:5、0:6、0:8 于 2026-11-18。0:4 是扩展容器，0:7 是 swap。不要重复安排这些目录探测。
- 当前唯一工作单元：P7-2B FIT 只读语义解析。P7-2C 不再等待设备输出；GPT primary/backup consistency 对当前 MBR 介质不适用。所有 eMMC install target 继续 UNMAPPED。
- 本轮没有读取并归档原始 MBR/EBR sector dump；不声称已完成 EBR 原始字节级审计。若未来确需该深度证据，再单独评估；当前 P7-2C inventory acceptance 不需要增加线下操作。
- P4 DHCP Client/Ping 实机验证仍是独立遗留项，P4 overall 不得标记 COMPLETE。

## 历史检查点（P6-B 完成前，保留审计记录）

P6-1A/B/C 软件门与 P6-A live descriptor 只读预检均已通过。P6-B 的 4 KiB、32 MiB 精确上限成功路径均已实机通过；32 MiB+1 超限文件已由实机及 pcap 确认在 OACK 的 tsize=33554433 校验阶段、DATA 前拒绝，失败后 filesize/fileaddr 均不存在且 live descriptor 不变。当前唯一未完成工作为受控中断/超时失败清理实机验收。

- 实机报告：`reports/pa220/P6-B/INITIAL-GET-ACCEPTANCE.md`。
- 32 MiB 精确 reservation 边界实机验收 PASS，主机独立 SHA-256/长度复核一致；报告：`reports/pa220/P6-B/32M-EXACT-BOUNDARY-ACCEPTANCE.md`。
- 测试文件生成器：`tools/pa220/make-p6-get-fixture.py`。
- 边界测试 fixture Actions [#45938976540](https://github.com/sdzbzyh/uboot-qsdk12.5-build/actions/runs/45938976540) SUCCESS；Artifact [15171887471](https://github.com/sdzbzyh/uboot-qsdk12.5-build/actions/runs/45938976540/artifacts/15171887471) 含 4 KiB、32 MiB、32 MiB+1 三个向量及 SHA256SUMS。
- 设备 build time：`Nov 25 2026 - 11:04:13`；IP `192.168.2.2` / TFTP host `192.168.2.10`；接口 `octrgmii0`。
- live DRAM：8 GiB；`loadaddr=0x20000000`；`__tmp_load=[0x20000000,0x30000000)`（256 MiB）。
- 成功路径：4096 bytes，`filesize=0x1000`，`fileaddr=0x20000000`，格式 `legacy-uImage`，CRC32 `9e4df6c8`，SHA-256 `83b1bb6b9c006c9d38c6915a985895bfeffd41f24d969aae1b74365041028082`；主机与设备摘要一致。
- 传输中出现一次 `rgmii rx` transient/drop warning；接收最终成功。无 pcap，不声称已确认 block size/块数。
- 本轮测试结束后设备可断电；未调用 `saveenv`，未执行 loader/启动命令，未写持久化介质。

32 MiB 精确上限实机已接收成功：size=33554432，CRC32=97257c28，SHA-256=fe773a13c9bfa1759c32826302e0b9a0e2ba6c88de9841219a2337d17861f526；Windows 主机复核长度与 SHA-256 完全一致，filesize=0x2000000、fileaddr=0x20000000，descriptor 不变。证据见 `reports/pa220/P6-B/32M-EXACT-BOUNDARY-ACCEPTANCE.md`。32 MiB+1 已在 OACK tsize 校验阶段拒绝，抓包仅有 RRQ/OACK、没有 DATA；失败后 filesize/fileaddr 均不存在，descriptor 不变。完整证据见 `reports/pa220/P6-B/32M-PLUS-1-OVERLIMIT-ACCEPTANCE.md`。受控中断/超时失败清理已于 2026-11-27 通过；当前 P6-B 测试矩阵全部完成。

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

继续按 `doc/pa220/P6-PLAN.md` 推进：P6-1A/B/C 软件门均已完成。P6-A live descriptor/DRAM/reservation 只读预检已通过；当前状态以本文件顶部“当前唯一主线”和 P6-PLAN.md 第 12 节为准。

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

P6-1C 已由 Actions #45600860179 验收通过，报告见 reports/pa220/P6-1C/ACCEPTANCE.md。P6-A 已通过，P6-B 首个成功路径已实机通过；当前状态以本文件顶部“当前唯一主线”为准。

## 最新 P6-B 验收闭环（2026-11-27）

- 受控中断实机报告：`reports/pa220/P6-B/CONTROLLED-INTERRUPTION-ACCEPTANCE.md`。
- pcap：`C:\\pa220-p6-b\\pa220-p6-b-interrupt.pcapng`，10,998,760 bytes，SHA-256 `83B96A88C0AEC9983A7B429369396B715B9613CAA2312D0A42984508AB6840CA`。
- Frames 13415/13416 payload 均为 `00 04 1a 32`（ACK block 6706），相隔约 4.999 秒。
- U-Boot 报告 network interruption；失败后 filesize/fileaddr 不存在，live descriptor 未变化。
- P6-B 实机测试矩阵已验收完成。未执行 loader/boot、saveenv 或持久化写入。


## 后续工作入口（2026-11-27）

- P6-B 实机测试矩阵已全部验收，设备本轮可断电。
- 按 `doc/pa220/ROADMAP.md`，下一开发入口为 P7 固件解析与安装预览（dry-run only）。P4 DHCP Client/Ping 实机验证仍是独立遗留项，不因 P6-B 通过而标记完成。
- P7 开始前先恢复远端最新 HEAD，阅读 ROADMAP、PROJECT、FEATURE-MATRIX 及固定源码相关解析路径；先实现只读解析与目标映射报告，不写 eMMC/SPI/NAND，不执行镜像。


## P7-1 云端验收检查点（2026-11-27）

- 验收报告：`reports/pa220/P7-1/ACCEPTANCE.md`。
- Actions #45958126118 和 #45958503668：Host GCC、ASan/UBSan、Cavium OCTEON 完整构建、最终 ELF 符号和 RAM-boot wrapper 生成均 PASS。
- Artifact ID：`15179692876`；下载地址见验收报告。
- BIN SHA-256：`f981ad195a0418d142b6a1cf34cfd70327668273612e4506b40d75fa11893c7c`。
- Stage3 wrapper：`0x153928` bytes；payload `0x1538e8` bytes；SHA-256 `4f5f3d8561f427cbcfae44f63c5d880e9229afc4fa8130f1556e08b321781ba8`。
- P7-1 实机 preview 已于 2026-11-27 通过；完整记录见 `reports/pa220/P7-1/DEVICE-ACCEPTANCE.md`。P7-1 完成，P7-2/P7-3 尚未开始。


## P7-1 实机验收闭环（2026-11-27）

- 实机报告：`reports/pa220/P7-1/DEVICE-ACCEPTANCE.md`；P7-1 总验收报告已更新。
- Stage3 wrapper TFTP GET 1,390,888 bytes，SHA-256 `4f5f3d8561f427cbcfae44f63c5d880e9229afc4fa8130f1556e08b321781ba8`；与 Actions artifact 一致。
- 旧 U-Boot 验证 wrapper uImage checksum OK；payload 1,390,824 bytes 复制/比较一致，并从 RAM `0x81000000` 成功启动 Nov 27 Stage3。
- 新 U-Boot 外层 legacy uImage preview：header/data CRC PASS；内层 OCTEON preview：board_type 20021 compatible、image_type 3、header/data CRC PASS。
- 两种 preview 均为 target mapping `UNMAPPED`、actions `NONE`；live `__tmp_load=[0x20000000,0x30000000)` 未变化。
- 全程未执行 `saveenv`、updater 或持久化介质写入。P7-1 实机验收 PASS；下一工作单元 P7-2。


## P7-2A SPI NOR 静态取证检查点（2026-11-27）

- Actions [#45962029417](https://github.com/sdzbzyh/uboot-qsdk12.5-build/actions/runs/45962029417) SUCCESS；Artifact [15181630025](https://github.com/sdzbzyh/uboot-qsdk12.5-build/actions/runs/45962029417/artifacts/15181630025)。
- 只读分析脚本：`tools/pa220/analyze-p7-spi.py`；工作流：`.github/workflows/pa220-p7-2-spi-forensics.yml`。从 main sparse checkout 读取原镜像，未把二进制复制到集成分支。
- 原机 SPI 镜像 SHA-256 `4899a1c5ed06e5eafbb8c92625a4afae14cad399806df13979fd2c70ee1ed922`；offset 0、0x1d80、0x10000、0x100000 的四个 OCTEON header 均通过 header/data CRC。
- 两个 FDT：0x58030 generic_spi_stage2；0x1c9460 pan,pa-220 / cavium,tb7100。后者描述 W25Q128-compatible SPI、OCTEON MMC host、Kingfisher CPLD。
- `0x70000-0x100000` 和 `0x250000-0x1000000` 为 all-FF 区域；配置 env 主/冗余 sector 与注释中的 0xef0000 区域均 all-FF。
- 此报告只证明备份内容，不推断 eMMC GPT、不建立安装映射、不授权 SPI 写入。
