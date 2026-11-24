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
