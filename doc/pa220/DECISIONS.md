# PA-220 U-BootKit 决策记录

本文件记录已经形成的项目约束。新增或修改决策时，注明日期、理由和影响范围。

## D-001：PA-220 使用独立 U-Boot 源码树

- 日期：2026-11-14
- 决策：不把 PA-220 OCTEON 板级代码直接混入 Qualcomm QSDK U-Boot 2016 构建目标。
- 理由：PA-220 使用 Cavium OCTEON SDK 5.1/U-Boot 2013.07、MIPS64 工具链，与 Qualcomm IPQ 构建链不同。
- 影响：通过独立的 build-pa220.sh 构建；原有 build.sh 和 Qualcomm 设备目标保持不变。

## D-002：固定 PA-220 上游源码基线

- 日期：2026-11-14
- 决策：PA-220 U-Boot 子树固定在 801717ba6a8a59ed2cfd53bf3e1b19841fecd2e8。
- 理由：保证补丁和构建结果可重复。
- 影响：升级源码必须作为独立变更评估，更新基线、补丁和测试记录。

## D-003：保留 Qualcomm 主线

- 日期：2026-11-14
- 决策：不修改集成仓库 main；PA-220 工作放在独立开发分支。
- 影响：本 P0 文档提交落在 feature/pa220-ubootkit-web，不得将其自动合并到 main。

## D-004：默认 RAM-only 测试

- 日期：2026-11-14
- 决策：使用原厂 U-Boot TFTP 将自定义 U-Boot 加载至 RAM 执行，优先于 SPI 持久化安装。
- 理由：PA-220 自定义 U-Boot 是 Stage 3，仍需原厂前级完成 DRAM 初始化；RAM 测试不改变持久化启动链。
- 禁止项：未获独立批准前不执行 saveenv、sf erase/write/update 或 CPLD 写操作。

## D-005：区分链接地址与 RAM 加载地址

- 日期：2026-11-14
- 决策：0xC0000000 是配置中的链接/镜像头地址；0x81000000 是安装资料中的 TFTP RAM 加载地址。
- 影响：文档、命令和测试记录不得混用二者。

## D-006：环境变量持久化位于 SPI NOR

- 日期：2026-11-14
- 决策：当前 PA-220 配置的环境主区为 0xe00000，冗余区为 0xe10000；saveenv 会写 SPI。
- 影响：查看环境可以作为诊断；保存环境不能被描述为 RAM-only 操作。

## D-007：先实现只读维护，再引入写操作

- 日期：2026-11-17
- 决策：现有 pa220、pa220 status、pa220 env 补丁定位为只读信息命令。
- 影响：Web 初版只读；上传先进入 RAM；eMMC 安装与 SPI 维护必须拆分为独立节点。

## D-008：当前 P0 文档承接分支

- 日期：2026-11-18
- 决策：本轮 P0 文档落在 feature/pa220-ubootkit-web，审计发现该分支相对 feature/pa220-octeon 前进 7 个提交，并包含维护命令补丁、补丁应用逻辑和维护路线草案。
- 影响：这不改变既有分支的历史，也不表示 feature/pa220-ubootkit-web 已成为唯一长期分支。后续应在 P1 节点明确开发分支整合策略。

## D-009：功能状态必须分层标记

- 日期：2026-11-18
- 决策：区分源码配置、补丁已实现、构建通过、RAM 启动通过和实机功能通过。
- 理由：配置存在或编译成功均不能证明硬件功能正确。

## D-010：RAM 接收和 ELF segment 写入统一受运行时 __tmp_load 边界约束

- 日期：2026-11-18
- 决策：TFTP block、HTTP chunk、完整 HTTP payload reservation 和 ELF segment 目标写入统一使用一个写入前范围验证入口；唯一权威范围来自运行时 `cvmx_bootmem_find_named_block("__tmp_load")` descriptor。
- 理由：`loadaddr`/旧 globals/固定地址都不能独立证明 named block 仍存在或仍有效；TFTP 当前 memcpy 前无检查，ELF64 loader 当前也没有 segment 目标范围保护。
- 实施要求：每次查询立即复制 static descriptor 字段；检查 descriptor 与 globals 一致；使用 checked add/multiply 和减法式半开区间检查；必须在 memcpy/memset 前拒绝越界；不把硬编码 RAM 地址写成最终安全地址。
- 影响：P2/P6 实现前，不能把新 U-Boot 的 TFTP/HTTP 上传或 bootelf 任意 ELF 镜像加载标记为安全可用。

## D-011：按 PA-220 实际 Cache API 语义区分数据同步与代码 Cache 维护

- 日期：2026-11-18
- 决策：PA-220 的 `flush_cache()` 视为全局同步并失效 I/D-cache 的操作；`flush_dcache_range()` 视为执行 `syncw` 的写入完成同步；`invalidate_dcache_range()` 当前为空实现。不得按函数名或其他 MIPS 平台实现推断其语义。
- 理由：固定 PA-220 `arch/mips/cpu/octeon/cpu.c` 与 `cvmx-asm.h` 已确认实际指令和宏定义；`flush_cache(start,size)` 忽略地址/长度参数。
- 影响：TFTP/HTTP payload 完成同步与 ELF/bootm 代码装载后的 I/D-cache 处理必须分别设计；不应对每个网络 chunk 调用全局 `flush_cache()`。任何后续优化必须保留可审查的同步/指令 Cache 一致性保证。


## D-012：P5 第一版 Web 服务采用显式前台只读生命周期

- 日期：2026-11-20
- 决策：PA-220 第一版 Web 仅由用户在 CLI 显式执行 `pa220 web` 启动；服务前台运行，Ctrl-C 退出并统一清理网络/TCP 状态。首版不自动进入 Web、不接管 autoboot、不后台常驻。
- 理由：PA-220 U-Boot 2013.07 的 `NetLoop()` 是阻塞式协议状态机，UDP/ARP handler 与 timeout callback 为单例全局状态；不能直接复用 uBootKit 2016 的 failsafe/TCP 生命周期。
- 影响：P5 必须单独实现 TCP 接收分派和服务轮询/cleanup；Web 与 DHCP/Ping/TFTP 等网络命令按串行使用，不并行抢占 NetLoop。首版仅允许 GET/HEAD 和经过审查的只读 handler；不提供命令执行、环境/存储/CPLD 写入、上传、安装或重启。
- 状态：架构约束持续有效；实现及验收状态由 D-013 和 `reports/pa220/P5-2C/ACCEPTANCE.md` 更新。P5-2C 已完成只读 Web 的 Host/目标构建/RAM 实机 HTTP 验收。


## D-013：P5-2C 实机验收边界与 P6 上传安全门

- 日期：2026-11-23
- 决策：P5-2C 的云端构建、Host Harness 和 PA-220 RAM 启动后的 HTTP/TCP 实机测试均已通过；正式证据见 `reports/pa220/P5-2C/ACCEPTANCE.md`。
- 证据限定：本轮没有原始 UART 录制，归档的 `pa220-serial.log` 是明确标注的会话重建件；abort 流程是 FIN（无 HTTP payload）→ HTTP 400 → RST+ACK，随后新连接 GET 获得 200，不得描述为 RST 中断部分 HTTP 请求。
- 决策：P5 第一版仍保持显式前台、单连接、单请求、只读 GET/HEAD 服务；不得增加任意命令执行、环境写入、CPLD/存储写入、上传、安装或重启。
- 决策：任何 TFTP/HTTP 文件接收或 ELF PT_LOAD/section 写入之前，必须先实现并验证统一 `octeon_load_range_check(uint64_t addr, uint64_t len)`，唯一权威范围来自运行时 `__tmp_load` named-block descriptor；验证器通过前接收功能保持 BLOCKED。
- 影响：下一工作单元先完成 validator 设计、Host 边界测试、OCTEON 目标编译/最终链接和只读运行时 descriptor 核对；通过后再进入 P6 RAM-only 接收。任何持久化写入仍属于独立高风险节点并需用户明确授权。
- 状态：P5-2C ACCEPTED；P6 validator prerequisite NOT YET IMPLEMENTED/ACCEPTED。


## D-014：统一 RAM validator 的 descriptor 快照与保留区证明

- 日期：2026-11-24
- 决策：生产 validator 只允许当前 cvmx_bootmem_find_named_block("__tmp_load") descriptor 指定的物理半开区间；每次调用立即复制 static descriptor 的字段，并连续读取两次确认 base/size/name 未变化。
- 理由：固定源码的 __cvmx_bootmem_find_named_block_flags() 返回函数内部 static descriptor，API 明确非 re-entrant；底层 named-block 查找锁在该函数复制字段前已释放，不能保存返回指针或声称它是原始 named-block 表条目。
- 保留区证明：board_octeon.c 将 U-Boot 的 bi_uboot_ram_addr/bi_uboot_ram_used_size 以 __uboot_code_data named block 再次登记；该 reservation 覆盖代码/data、malloc、stack、bd、gd 和 boot parameters。validator 要求该 named block 与 gd->bd 精确一致，并排除低 1 MiB、Linux reserved、FDT、exception、bootloader environment 等已知 named blocks。
- 地址策略：bootmem descriptor 使用 physical base；输入统一规范化 physical/identity、KSEG0/KSEG1、XKPHYS CCA aliases。拒绝不支持的地址域、KSEG 跨段、XKPHYS 保留位、零长度、加法溢出及半开区间越界。
- 并发前提：当前 U-Boot 前台单核命令模型下，次级核心停留在 idle loop，不存在并发 bootmem writer；若引入异步/多核 bootmem 修改，必须提供锁保护的原子 descriptor snapshot，不得继续依赖当前双读假设。
- 状态：生产 validator 已实现；Host 32 scenarios / 121 assertions / 0 failures（普通 GCC、ASan/UBSan）；OCTEON 目标构建待 Actions。TFTP/HTTP/ELF 写入调用点尚未接入，继续 BLOCKED。


## D-015：P6 首个 RAM-only 接收路径与当前门控（2026-11-24）

- 决策：P6 首个真实接收路径固定为 PA-220 原生 TFTP GET/RRQ；不改为 HTTP upload。
- 决策：P4-2 validator/TFTP DATA guard 的 Host 和 OCTEON 构建通过，不自动授权任何实际接收。实际 GET 必须等待 live `__tmp_load` descriptor/reserved-block 只读预检、完整文件 reservation/最大长度/校验/格式识别/失败有效状态管理及用户明确确认。
- 决策：TFTP Server/WRQ、PUT、HTTP upload、ELF PT_LOAD/section 写入分开验收；HTTP 保持只读。
- 证据：Actions #45346000355、#45374041373、#45375848020；最新 P4-2 验证提交 `20acf9790b6ee15ed758db7006eaee69af0f2bf2`；P6-0 审查报告 `reports/pa220/P6-0-SOURCE-REVIEW.md`。
- 状态：P6 设计/预检准备；live descriptor 与实际接收 NOT RUN。所有持久化写入继续禁止。

## D-016：首个 TFTP GET 测试向量（设计，不代表已执行）

- 决策：首个测试文件采用 4,096-byte ASCII 数据，由重复字符串 `PA220-P6-TFTP-GET-TEST\\n` 截断生成；预期 CRC32 `e32b0c57`、SHA-256 `dac1fcd877c5ef356b2cdd44f6202f154a2d393cd458975b521ef6cfc05b8356`。
- 目标 block size 为 512 bytes；确认协商值后，预期 8 个完整 DATA block 加 1 个零长度终止 block。
- 目标地址必须由 live `__tmp_load` descriptor 推导并验证完整半开区间；不使用历史固定地址作为安全依据。
- 状态：测试向量仅完成设计，未传输。


## D-017：按实际风险安排必要的实机测试

- 日期：2026-11-25
- 决策：不机械沿用早期交接提示中的 blanket 禁止语句；根据当前实现、设备状态和测试目的选择最小风险的只读或 RAM-only 实机测试。用户已授权在项目推进确有需要时安排测试，每轮结束后设备断电。
- 执行要求：先完成对应的软件 Host/目标验证；每轮测试明确命令、预期结果、停止条件和证据采集方式；完成一轮后断电。
- 持久化写入、环境保存、EEPROM/CPLD 修改、flash 擦写/刷写等破坏性操作不属于常规测试授权。确有必要时必须单独说明具体对象、风险和回滚方案，并取得针对该操作的明确同意。
- 影响：只读 descriptor 预检和满足软件门后的 RAM-only TFTP GET 可按项目进度主动安排，不必因旧提示词要求“设备断电”而无限期推迟。

## D-018：PA-220 Web UI 采用 uBootKit Argon 风格作为最终视觉目标

- 日期：2026-11-25
- 决策：PA-220 最终 Web UI 的视觉语言、导航结构和响应式体验以 uBootKit Argon 风格为参考；详细规范见 `WEB-UI-DESIGN.md`。
- 适配原则：只复用视觉/交互模式，不复制 IPQ 专属实现和危险业务权限。PA-220 的 Web 初版保持只读；命令执行、环境持久化修改、升级/刷写、重启等不因参考页面存在而开放。
- 影响：HTTP/TCP 协议服务验收与完整 Web UI 验收分开记录；最终项目交付必须包含实际页面，而不只是 API。


## D-019：TFTP GET 开始前失效旧接收元数据

- 日期：2026-11-25
- 决策：TFTP GET/RRQ 在发送请求前，必须先使旧 `filesize` 与 `fileaddr` 同时失效；两项删除独立尝试，任一删除失败或最终仍有变量存在即拒绝 GET。
- 理由：后续 DATA/超时/越界失败可能留下部分新数据；旧元数据若残留，会将部分新数据误认为上一轮完整有效文件。
- 实现：固定源码 `setenv(name, NULL)` 对不存在变量也返回非零，故必须先 `getenv()` 判断存在；不调用 `saveenv`。
- 影响：只作用于 `TftpStart()` 的 GET 分支；PUT 与 `TftpStartServer()` 的 WRQ 行为不变。成功接收仍由正常成功路径发布新元数据。
- 状态：P6-1A 软件验收通过（Host GCC、ASan/UBSan、补丁生成器重复运行、OCTEON TFTP object、N32 relocatable link、完整 U-Boot final link）；证据见 `reports/pa220/P6-1A/ACCEPTANCE.md` 与 Actions #45588666611。不授权真实文件接收。

## D-020：P6-1B 完整文件 reservation 与 GET 失败状态（2026-11-25）

- 决策：TFTP GET/RRQ 在发送前对最大文件完整区间做一次 reservation；默认上限 32 MiB，由 CONFIG_PA220_TFTP_MAX_FILE_SIZE 配置。reservation 使用 P4-2 validator，不能只逐 DATA block 临时验证。
- 决策：每个 DATA block 必须按连续 offset 接收，长度不得超过 reservation 剩余空间；写入后累计总长度再次 checked，完成时核对 NetBootFileXferSize、累计字节数和服务器 tsize。
- 决策：接收失败状态显式区分 IDLE/ACTIVE/COMPLETE/FAILED。超时、TFTP ERROR、ICMP、越界/乱序、超长、目标写入失败、首块错误、tsize/总长度不匹配以及 do_tftpb 返回时的中断均使部分结果失效并清零长度。
- 决策：通过 do_tftpb() 的 GET-only abort hook 处理 Ctrl-C/NetLoop 非成功返回；不改 TFTP PUT/WRQ 行为，不调用 saveenv。
- 证据：Host 14 cases / 26 assertions / 0 failures（GCC、ASan/UBSan）；Cavium SDK 5.1.0 OCTEON object、N32 relocatable link、完整 U-Boot final link PASS；Actions #45595798808。
- 状态：P6-1B SOFTWARE ACCEPTED；不授权真实 TFTP GET。下一步 P6-1C 校验和/格式识别，之后 P6-A live descriptor 只读预检。

## D-021：P6-B 首个真实 TFTP GET 成功路径与修正后的测试向量

- 日期：2026-11-27
- 首个真实 GET 使用 4096-byte 可识别 legacy uImage header + inert ASCII payload；字段采用 Linux/MIPS/RAMDisk/none，避免使用会被 P6-1C 正确拒绝的 invalid/unsupported header。
- 固定向量：`ih_size=4032`，`ih_dcrc=705c16e0`，`ih_hcrc=b1aa12e3`；全文件 CRC32 `9e4df6c8`；SHA-256 `83b1bb6b9c006c9d38c6915a985895bfeffd41f24d969aae1b74365041028082`。生成器：`tools/pa220/make-p6-get-fixture.py`。
- 实机接收 4096 bytes；设备 `filesize=0x1000`、`fileaddr=0x20000000`，识别 `legacy-uImage`；主机与设备 SHA-256 一致；live descriptor 未改变。
- 出现一次 `rgmii rx` transient/drop warning，随后传输完成。无 pcap，不推断 block size 或 DATA block 数量。
- 状态：P6-B PARTIAL — initial success path accepted；边界拒绝及中断/超时失败清理仍待验收。未执行 loader、`saveenv` 或持久化写入。
- 本决策取代 D-016 的无效测试向量；D-016 保留为历史设计记录，不再作为测试文件规范。

## D-022：存储分区表同时兼容 DOS/MBR 与 GPT

- 日期：2026-11-27
- 决策：PA-220 U-BootKit 后续的介质识别、分区枚举、镜像布局分析和 dry-run 目标映射必须同时支持 DOS/MBR（含 extended/EBR logical partitions）与 GPT（含 Protective MBR、主/备 Header 和 Entry Array 校验）。
- 理由：PA-220 当前实机 eMMC 已确认使用 DOS/MBR；源码同时启用 DOS 与 EFI/GPT 分区支持。未来设备或镜像可能使用 GPT，不能把当前设备布局当作全局假设。
- 实施要求：按实际介质内容自动识别；统一输出 source format、分区索引、起始 LBA、扇区数、type/GUID、name、attributes 和可用 UUID。MBR 必须验证 EBR 链与 extended container 边界；GPT 必须验证 CRC、LBA 边界和主备结构一致性。Hybrid MBR、签名冲突、损坏或不一致必须 fail closed 并报告，不修复、不写入。
- 验证要求：MBR 与 GPT 各自建立独立 host fixture；GPT 测试覆盖主备一致、CRC 错误、越界、重叠和不一致，MBR 测试覆盖 primary、extended/EBR、循环链和越界。真实 PA-220 MBR 证据不替代 GPT 测试。
- 影响：当前 PA-220 eMMC 仍按已验证的 DOS/MBR 解释；所有后续通用分区代码与文档必须保留两种格式兼容。
