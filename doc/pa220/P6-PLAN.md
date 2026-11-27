# PA-220 P6 计划：RAM-only 文件接收与校验

- 状态：**P6-1A/P6-1B/P6-1C 软件门 ACCEPTED；P6-A live descriptor/DRAM/reservation 只读预检 PASS；P6-B 4 KiB、32 MiB 精确上限成功路径 PASS；32 MiB+1 在 OACK tsize 阶段拒绝及失败清理 PASS；仅受控中断/超时失败清理待验收**
- 日期：2026-11-24
- 集成分支：`feature/pa220-ubootkit-web`
- 固定 PA-220 源码：`sdzbzyh/pa220-openwrt@801717ba6a8a59ed2cfd53bf3e1b19841fecd2e8`
- P4-2 前置证据：[IMPLEMENTATION.md](../../reports/pa220/P4-2/IMPLEMENTATION.md)
- 最新 P4-2 Actions：[run #45375848020](https://github.com/sdzbzyh/uboot-qsdk12.5-build/actions/runs/45375848020)

## 1. P6 目标与边界

P6 只负责将一个文件接收到经验证的 RAM 区间，并对接收结果进行长度、校验和格式识别。P6 不安装固件、不写持久化介质、不修改启动环境、不执行接收到的镜像。

首个验收路径选为 PA-220 原生 TFTP GET（设备作为 TFTP client，从维护主机读取文件）。该路径已有固定源码实现，且 DATA block 唯一 RAM 写入点已接入统一 validator 并通过 Host/目标编译门。TFTP Server/WRQ、TFTP PUT、HTTP upload 分开处理，不纳入首个接收测试。

## 2. 已满足的软件前置条件

- `octeon_load_range_check(addr, len)`：生产代码 Host 42 scenarios / 161 assertions / 0 failures；GCC 与 ASan/UBSan PASS。
- TFTP DATA `store_block()` pre-write guard：Host 14 scenarios / 64 assertions / 0 failures；GCC 与 ASan/UBSan PASS。
- 修改后的固定 PA-220 `tftp.c` 编译为 `tftp.o` 并进入 `libnet.o`。
- OCTEON validator object、N32 relocatable link、完整 U-Boot final link PASS。
- TFTP receive-side 唯一 RAM memcpy、GET/Server DATA 共用路径、PUT 仅读 RAM 的源码审计已完成。

这些结果只证明 Host 逻辑测试和目标编译/链接，不证明设备运行时 descriptor 正确，也不证明网络接收成功。

## 3. P6-A：设备只读 RAM 布局预检 — PASS（2026-11-26）

实机原始输出和逐项地址核对见 [P6-A READONLY PREFLIGHT](../../reports/pa220/P6-A/READONLY-PREFLIGHT.md)。

确认结果：

- 当前设备运行 U-Boot build time `Nov 25 2026 - 11:04:13`，与 P6-1C Actions #45600860179 完整目标构建窗口相符。
- `bdinfo`：`memstart=0x80000000`，`memsize=0x200000000`（8 GiB）。
- `pa220 memory` / `namedprint`：`loadaddr=0x20000000`，`__tmp_load` base=`0x20000000`，size=`0x10000000`（256 MiB）。
- `autoload=n`；`ipaddr/serverip` 未定义，和本阶段只读预检无关。
- 按 CN70XX DDR0 / boot-bus hole / DDR1+ 映射解释，`0x20000000` 是物理 DDR1 起点的 identity-mapped 地址；`memstart=0x80000000` 是 cached alias，不构成地址冲突。
- Linux reserved、低 1 MiB/FDT/exception/idle/MAC、free block 和高端 `__uboot_code_data` 均与 `__tmp_load` 分离。
- P6-1B 默认 32 MiB 完整 reservation 从 `0x20000000` 起可容纳于 256 MiB descriptor。

结论：P6-A 只读 RAM 布局和完整 reservation 可行性通过。生产 validator 在 P6-B 的 RRQ 前仍会重新读取 descriptor，并核对 `load_reserved_addr/load_reserved_size`；若运行时不一致会在接收开始前 fail closed。P6-A 不等于授权实际传输。

本轮只执行只读命令；没有执行 TFTP、`setenv`、`saveenv`、flash/storage 写入、reset 或 reboot。

## 4. P6-B：首个 RAM-only TFTP GET — SUCCESS PATH PASS（2026-11-27）

首个 4 KiB 合法 legacy-uImage 已在实机完成 TFTP GET。完整证据见 [P6-B INITIAL GET ACCEPTANCE](../../reports/pa220/P6-B/INITIAL-GET-ACCEPTANCE.md)。

- 设备 U-Boot 2013.07，build time `Nov 25 2026 - 11:04:13`；`ipaddr=192.168.2.2`，`serverip=192.168.2.10`，`ethact=octrgmii0`。
- live DRAM 8 GiB；`loadaddr=0x20000000`；`__tmp_load` base `0x20000000`，size `0x10000000`（256 MiB）。
- 接收 4096 bytes；格式 `legacy-uImage`；CRC32 `9e4df6c8`；SHA-256 `83b1bb6b9c006c9d38c6915a985895bfeffd41f24d969aae1b74365041028082`。
- 设备发布 `filesize=0x1000`、`fileaddr=0x20000000`；主机 SHA-256 与设备一致；descriptor 未变化。
- 传输中出现一次 `Receive error (code 11: rgmii rx), dropping`，随后传输成功。保留为 transient RX warning，不代表链路稳定性已验收。
- 无 pcap，不推断协商 block size、DATA block 数量或终止块形态。

### 32 MiB 精确 reservation 上限实机结果 — PASS（2026-11-27）

完整串口与主机复核证据见 [P6-B 32 MiB EXACT BOUNDARY ACCEPTANCE](../../reports/pa220/P6-B/32M-EXACT-BOUNDARY-ACCEPTANCE.md)。

- 设备成功接收 33,554,432 bytes（32 MiB），输出 `format=legacy-uImage`、CRC32 `97257c28`、SHA-256 `fe773a13c9bfa1759c32826302e0b9a0e2ba6c88de9841219a2337d17861f526`。
- 设备发布 `filesize=0x2000000`、`fileaddr=0x20000000`；`__tmp_load` 仍为 base `0x20000000`、size `0x10000000`。
- Windows PowerShell 独立复核文件长度 `33554432`，SHA-256 与设备报告完全一致。
- 因此 32 MiB 配置上限的精确边界接收 PASS；此结果不覆盖 32 MiB+1 拒绝和失败清理。

### 下一轮 reservation 边界测试向量（已由生成器支持）

生产默认上限为 32 MiB，live `__tmp_load` 为 256 MiB。无需改动生产配置即可验证实际最大 reservation：

- 精确上限：`python3 tools/pa220/make-p6-get-fixture.py --size 33554432`，生成 32 MiB 合法 legacy-uImage；预期完整接收成功，设备 `filesize=2000000`（hex），格式识别成功。
  - 预期 CRC32：`97257c28`
  - 预期 SHA-256：`fe773a13c9bfa1759c32826302e0b9a0e2ba6c88de9841219a2337d17861f526`
- 超上限 1 byte：`python3 tools/pa220/make-p6-get-fixture.py --size 33554433`，生成 32 MiB + 1 byte 文件；预期 TFTP GET 失败且 `filesize`/`fileaddr` 均不发布。
  - 预期 CRC32：`d9aa897f`
  - 预期 SHA-256：`b92c884a7f921c6065d1e8a7bf696aa26d6ef0ef61d17effa71b7a2656e828d2`
- 超限拒绝可能发生在服务器 OACK 的 `tsize` 检查，也可能在 DATA block 超出 reservation 时发生；必须用 pcap/服务端日志确认实际拒绝阶段，不得把前者描述成 DATA 写入越界测试。

- GitHub Actions [#45938976540](https://github.com/sdzbzyh/uboot-qsdk12.5-build/actions/runs/45938976540) SUCCESS：生成器语法、4 KiB、32 MiB 精确上限、32 MiB+1 四项检查均 PASS。
- Artifact [15171887471](https://github.com/sdzbzyh/uboot-qsdk12.5-build/actions/runs/45938976540/artifacts/15171887471) 包含三个二进制文件及 `SHA256SUMS`；ZIP SHA-256：`026873dea7a154d0b455b671998d9ccf75ceebc797ebc38cff2a0d3a2a4cd205`。
- 每次 GET 后只读检查 `printenv filesize fileaddr` 和 `pa220 memory`。超限失败后两项应不存在，descriptor 不变。不得执行任何 loader/启动命令。
- 中断/超时测试另行安排受控延迟/停止服务器的方式；不得依赖随机拔网线或不可控时机按 Ctrl-C。

**P6-B 目前 4 KiB、32 MiB 精确上限成功路径、32 MiB+1 OACK tsize 超限拒绝及失败清理均已通过实机验收；整体仍未验收。** 32 MiB+1 抓包仅见 RRQ 与 OACK（服务器返回 `tsize=33554433`），未见 DATA；设备在 tsize 校验阶段拒绝。失败后 `filesize`/`fileaddr` 均不存在，live descriptor 不变。完整证据见 [P6-B 32 MiB+1 OVERLIMIT ACCEPTANCE](../../reports/pa220/P6-B/32M-PLUS-1-OVERLIMIT-ACCEPTANCE.md)。仅受控中断/超时失败清理仍待实机验证。未执行 loader、启动命令、`saveenv` 或持久化写入。

P6-B 只验证 TFTP GET，不代表 TFTP Server/WRQ、PUT、HTTP upload 或 ELF loader 写入安全。

## 5. P6-C：校验与格式识别（P6-1C SOFTWARE ACCEPTED）

P6-1A、P6-1B、P6-1C 的软件门均已完成。P6-1C 的实现范围与边界如下：

- 接收结束后使用固定源码中的 sha256_csum_wd() 和 crc32_wd() 计算 SHA-256/CRC32，输出长度、digest 和格式类型；当前 TFTP GET 不协商可信预期 digest，因此这些值用于可信主机端比对，不单独构成真实性认证。
- 格式识别仅做只读 magic/header 检查：legacy uImage、FIT/FDT、ELF32/ELF64；检查文件长度、固定 header 边界、声明大小、FDT block 范围和 ELF program/section table 范围；不调用 loader、不执行、不写介质。
- 长度/状态检查失败、格式不支持或 header 越界时明确进入 FAILED，清除/作废接收结果并失效 filesize/fileaddr，不得发布有效文件元数据。
- P6-1C Host GCC/ASan/UBSan、OCTEON object、N32 relocatable link 和完整 U-Boot final link 已验收；下一步进入 P6-A live descriptor 只读预检。P6-B 真实 RAM-only TFTP GET 仍须另行确认。

## 6. 明确不在 P6 首轮范围内

- HTTP POST/upload、multipart、chunked transfer：HTTP 当前保持 GET/HEAD 只读；需要独立流式 body parser、长度上限、reservation 和 Host/目标测试。
- TFTP Server/WRQ 与 TFTP PUT：另行审计/验收；PUT 的本地 RAM source range/read disclosure 不属于 receive guard 覆盖范围。
- ELF64 PT_LOAD/section fallback：仍需单独的 source/destination range、tail-width 和 cache synchronization 审计。
- eMMC/SPI/NAND/CPLD/EEPROM/flash 写入：完全禁止，另属高风险阶段。

## 7. 失败与安全规则

- 不执行 `saveenv`，不修改持久化环境。
- 不写 SPI/eMMC/NAND/CPLD/EEPROM，不擦除 flash，不 reset/reboot/power-cycle。
- 接收到的文件均视为不可信数据；长度、校验和格式检查完成前不得加载/执行。
- 若运行时 descriptor 与构建/历史预期不一致，立即停止，不猜测地址或调整固定地址绕过 validator。


## 8. P6-0 源码审查结果与首个测试向量（2026-11-24）

源码审查、验证证据和设备预检命令详见 [P6-0 SOURCE REVIEW](../../reports/pa220/P6-0-SOURCE-REVIEW.md)。本节补充首个 GET 的可复现数据向量，并明确实际接收前的完整软件门。

### 固定测试文件（P6-B 实机已使用；生成器可复现）

- 文件 `pa220-p6-get-4096.bin`：4096 bytes，64-byte legacy uImage header + 4032-byte inert ASCII payload（重复 `PA220-P6-TFTP-GET-TEST\n` 截断）。
- Header：magic `0x27051956`，timestamp 0，ih_size 4032，load/entry 0，ih_dcrc `705c16e0`，ih_os=5 Linux，ih_arch=5 MIPS，ih_type=3 RAMDisk，ih_comp=0 none，name `PA220-P6-GET-TEST`，ih_hcrc `b1aa12e3`。
- 完整文件 CRC32 `9e4df6c8`；SHA-256 `83b1bb6b9c006c9d38c6915a985895bfeffd41f24d969aae1b74365041028082`。
- 生成器：[`tools/pa220/make-p6-get-fixture.py`](../../tools/pa220/make-p6-get-fixture.py)，自动断言长度、header/data CRC32 和全文件指纹。
- 该 header 可被 P6-1C 识别为 legacy uImage；payload 是惰性测试文本。格式识别不代表签名或可启动性验证，严禁启动。
- 原 D-016 的 `ih_os=0/ih_type=0` invalid header 与 P6-1C 拒绝行为冲突，已由此向量取代。

### 目标地址和完整区间

从 live `__tmp_load` descriptor 读取 base=B、size=S。目标 T 必须由已确认的 descriptor 范围选择，规范化到 physical domain 后使用 checked arithmetic 验证完整半开区间 `[T,T+4096)`；必须满足 `T >= B`、`T-B <= S` 且 `4096 <= S-(T-B)`。完整区间还必须与 U-Boot、stack、GD、malloc、FDT、exception 及所有已知 reserved blocks 分离。每个 DATA block 仍须经过现有 validator。禁止把 `0x81000000`、`0x20000000` 或任何历史地址当作默认安全目标。

### 实际接收前的顺序门

P6-A 只读设备预检通过并不单独授权接收。首个真实 GET 还必须等待 P6 软件门完成：最大总长度和完整 reservation、接收完成/有效状态、超时/中断/越界失败路径、CRC32/SHA-256、只读格式识别均需完成 Host 与 OCTEON 验证。失败路径必须清除或显式作废接收结果，不能让部分数据被标记为有效。获得用户明确确认后才执行 GET。

接收后使用 PA-220 的数据写入同步语义（`flush_dcache_range()` / `syncw`）再执行校验；不要对每个 block 调用全局 `flush_cache()`。收到的数据永不自动执行或写入持久化介质。


---

## 9. 当前执行覆盖（2026-11-25）

本节优先于本计划前文中与当前授权不一致的旧状态说明；旧内容保留为当时决策和审计记录。

### P6-1A：TFTP GET 开始前失效旧接收元数据 — ACCEPTED

- 生产 helper `tftp_invalidate_get_metadata()` 独立尝试删除 `filesize` 与 `fileaddr`，最后重新读取确认两项均不存在。
- 只在 `TftpStart()` 的 GET/RRQ 分支、RRQ 启动之前调用；失效失败设置 `NETLOOP_FAIL` 并拒绝 GET。
- 不调用 `saveenv`；PUT 与 `TftpStartServer()` 的 Server/WRQ 路径不受补丁影响；正常成功接收后的元数据发布路径未修改。
- Host 测试直接编译生产 helper，并注入同一生成器中的生产 GET gate 代码：6 cases / 19 assertions / 0 failures；普通 GCC 和 ASan/UBSan 均通过。
- 独立补丁生成器已在同一修改后源码树重复运行，输出 `P6-1A_ALREADY_GENERATED=verified`，patch SHA-256 保持一致。
- 固定 PA-220 源码 OCTEON `tftp.o`、N32 relocatable link、完整 U-Boot final link 均通过。
- 正式证据：[#45588666611](https://github.com/sdzbzyh/uboot-qsdk12.5-build/actions/runs/45588666611)；完整记录见 [P6-1A ACCEPTANCE](../../reports/pa220/P6-1A/ACCEPTANCE.md)。

### P6-1B：完整文件 reservation 与接收失败状态 — SOFTWARE ACCEPTED

P6-1B 已完成生产实现、Host GCC/ASan/UBSan、OCTEON object、N32 relocatable link、完整 U-Boot final link 和独立补丁重复生成验证。P6-1B 不授权真实 TFTP GET；后续接收仍须经过 P6-1A/B/C 软件门和 P6-A live descriptor 只读预检。

### P6-1C：接收后 SHA-256/CRC32 与只读格式识别 — SOFTWARE ACCEPTED

- Actions [#45600860179](https://github.com/sdzbzyh/uboot-qsdk12.5-build/actions/runs/45600860179)：SUCCESS；验证提交 b366a278ec15445a60f12441739d172b94392e3e。
- Host GCC：17 cases / 31 assertions / 0 failures；ASan/UBSan：17 / 31 / 0。
- 生产代码调用 flush_dcache_range()、crc32_wd()、sha256_csum_wd()；只读识别 legacy uImage、FIT/FDT、ELF32/ELF64，并检查长度及 header/table 边界。
- 不支持/损坏格式和长度状态失败进入 FAILED，清零接收长度并失效 filesize/fileaddr；有效状态只在 digest 计算及格式识别后发布。
- Cavium SDK 5.1.0 / GCC 4.7.0；OCTEON tftp.o、N32 relocatable link、完整 U-Boot final link 均 PASS。
- P6-1C patch SHA-256：34941096eef9983fe7f951bb29c88e88e31f7d29d330ea4118fd83fcf5a9b32b；tftp.o SHA-256：bec5e494f7e5efee894fa23e902ead5894c468fc40159e25569daeeec549d433。
- 完整证据见 reports/pa220/P6-1C/ACCEPTANCE.md。**截至 P6-1C 软件验收时**尚未执行真实 TFTP GET；随后 P6-B 首个成功路径已由本计划第 4 节记录。


### 实机测试安排更新

用户已明确授权：项目推进过程中确有必要时，可以安排实机测试；完成一轮测试后设备断电。此前章节中“本阶段不要求上电”仅代表当时状态，不再构成后续测试的永久限制。

- 只读 descriptor 预检可在软件准备需要且具备条件时主动安排。
- RAM-only TFTP GET 仍须先完成相应 Host/目标软件门，并证明 live `__tmp_load`、完整 reservation、目标地址和长度安全。
- 每轮测试前提供清晰步骤、停止条件和证据采集要求；每轮结束后断电。
- 用户的常规实机测试授权不包含持久化写入。任何 saveenv、EEPROM/CPLD/flash/eMMC/NAND 写入、擦除或刷写仍须单独说明并取得明确同意。
- 不把一次 RAM-only GET 授权扩展到 PUT、WRQ、HTTP upload 或 ELF segment 写入。

### 安全边界采用风险驱动方式

不机械沿用旧交接提示中的 blanket 禁止项；按当前阶段选择最小必要权限和风险最低的验证方式。若旧限制妨碍低风险只读/RAM-only 验证，应以本节和 `doc/pa220/EXECUTION-CONTEXT.md` 的最新项目决策为准。破坏性操作仍需单独审批。

### 当前后续交付

P6-1A、P6-1B、P6-1C 软件门均已完成；下一步先安排 P6-A live descriptor/DRAM/reservation 只读预检。P6-A 通过并获得单独确认后，才考虑 RAM-only TFTP GET。最终 Web 页面另按 `doc/pa220/WEB-UI-DESIGN.md` 实施，视觉参考 uBootKit Argon 风格；Web UI 不因视觉参考而开放未经验证的写功能。


## 10. P6-1A 实施记录（2026-11-25）

### 固定源码 API 语义
固定 PA-220 源码 `u-boot/common/cmd_nvedit.c` 的 `_do_env_set()` 删除分支对 `hdelete_r()` 返回值取反；`setenv(name, NULL)` 在变量存在且删除成功时返回 0，变量不存在或删除失败时返回 1。因此生产 helper 先用 `getenv()` 检查变量是否存在，再检查删除返回值。

### 实现范围
- 生产 helper：`tftp_invalidate_get_metadata()`，分别尝试删除 `filesize` 与 `fileaddr`，不因第一项失败而跳过第二项；最后再次确认两项均不存在。
- GET/RRQ：仅在 `TftpStart()` 的 GET 分支、设置 RRQ 状态及发送请求之前调用 helper。任一失效失败时设置 `NETLOOP_FAIL` 并返回，不启动 RRQ。
- PUT 与 Server/WRQ：不调用 helper，不改变发送和服务行为。
- 不调用 `saveenv`；正常成功接收后的元数据发布路径不变。
- 生成器在固定源码工作树中生成 P6-1A 独立 unified diff，所有 diff 由 `git diff --binary` 产生。

### 验收结果

- Actions [#45588666611](https://github.com/sdzbzyh/uboot-qsdk12.5-build/actions/runs/45588666611)：SUCCESS；验证提交 `2ef6771739c2020a7bb3446c71f3a070c0b42901`。
- Host GCC：6 cases / 19 assertions / 0 failures；ASan/UBSan：6 / 19 / 0。
- 补丁生成器在已修改源码树重复运行成功，`P6-1A_ALREADY_GENERATED=verified`，补丁 SHA-256 一致。
- Cavium SDK 5.1.0 / GCC 4.7.0 已校验；TFTP OCTEON object、N32 relocatable link、完整 U-Boot final link 均 PASS。
- P6-1A patch SHA-256：`040c24b8fff0ab76dc83a8479d9760852b5ba685d38622141466c7fc95c54330`。
- 完整测试、SDK、对象、链接产物哈希见 `reports/pa220/P6-1A/ACCEPTANCE.md`。

在 P6-1A 验收时，完整文件 reservation、最大长度和失败状态仍未完成；这些软件门现已由 P6-1B 完成。校验/格式识别由 P6-1C 继续，live descriptor 预检由 P6-A 完成；在这些步骤结束前仍不授权真实 TFTP GET。

## 11. P6-1B 实施与验收记录（2026-11-25）

### 生产实现

- 默认最大文件长度为 32 MiB，通过 CONFIG_PA220_TFTP_MAX_FILE_SIZE 可配置；GET 在发送 RRQ 前使用 P4-2 validator 对完整最大区间进行 reservation。
- DATA 写入前检查 block 对应 offset 必须等于已接收总长度，并检查 len 不超过 reservation 剩余空间；写入后以再次 checked 的 commit 更新总长度。
- TFTP tsize 以有界十进制解析器处理，拒绝非数字、空值和超过最大长度的值；最终完成时比较 tsize、NetBootFileXferSize 和实际累计长度。
- 接收状态包含 IDLE、ACTIVE、COMPLETE、FAILED。失败路径覆盖 reservation 失败、metadata gate 失败、越界/乱序/超长、目标写入失败、TFTP ERROR、超时/restart、ICMP 中断、首块错误、完成长度不一致及 do_tftpb 返回时的 Ctrl-C/abort。
- 失败统一清零 NetBootFileXferSize、清除接收状态并再次失效 filesize/fileaddr；不会把部分文件标记为成功。PUT 与 Server/WRQ 不进入 GET helper；PUT 的 tsize 解析行为保留；不调用 saveenv。

### 验收证据

- Actions [#45595798808](https://github.com/sdzbzyh/uboot-qsdk12.5-build/actions/runs/45595798808)：SUCCESS；验证提交 fc6f60ca28418f74a418456cd1144a5c4f0f017a。
- Host Harness 直接注入生产 GET_STATE_HELPERS：14 cases / 26 assertions / 0 failures；GCC 和 ASan/UBSan 均通过。
- Cavium SDK 5.1.0，GCC 4.7.0；SDK archive SHA-256：cd8cd3232078094c89accf32a82a13e7847e3466c94e9b5e12dd11b741102d99。
- OCTEON tftp.o 与 N32 relocatable object 均为 ELF32 big-endian、ABI2；完整 U-Boot final link 成功。
- P6-1B patch SHA-256：e3dba0c7fdfce6364a93998d94ea823a8d0307e92656b51a4619e7ffeca509b5；GET abort patch SHA-256：71a1e403dd0988add42c8b229332dc6379e65c437de3c1e057158977398c8ccc。两者均由 git diff --binary 生成。
- tftp.o SHA-256：538c0bebdb0133d9e6e67228295c8558a672ea90d8a2a7bd3dc2d0d834ccbc29；cmd_net.o：4412f25a65989c4007655eb446ca91df53801a905596ef78d1ecea108b12820a；N32 relocatable：d33743c508344d97bc9d0050cb08cc79095b771f5d2d7e83ef1583d1b6610aa7。
- 完整 U-Boot：e383384dc26606ccf1a18c6d07b6217dc7f3cabd6699e132f92c6691e19286b9；map：cec340e456ac8e29afcc29c73c1d639afdc575e1139593d11c67cee99f94495c；System map：e7f19bcee44b0859b0588e058480662be62b5fe3b3a2cb865fa27abfbcc40a76；BIN：d4779c0a55a47891af2865cd315b3a827a990cece7e0c10201ab14e5e1a97aec。
- 构建日志存在 PA-220 基线配置中既有宏重定义 warnings；无新增 P6-1B 编译错误，目标与最终链接均成功。

### 未执行事项与下一步

- **截至 P6-1B 软件验收时**未执行真实 TFTP GET；随后 P6-B 首个成功路径已由本计划第 4 节记录。该阶段未调用 saveenv，未写入 SPI/eMMC/NAND/EEPROM/CPLD/flash。
- P6-A 与 P6-B 的前述实机检查点均已完成；当前下一工作单元仅为受控中断/超时失败清理实机验收。