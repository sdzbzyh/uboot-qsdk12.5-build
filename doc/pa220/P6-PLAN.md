# PA-220 P6 计划：RAM-only 文件接收与校验

- 状态：**P6-1A/P6-1B 软件门 ACCEPTED；P6 整体未开放实际接收；未执行任何文件传输**
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

## 3. P6-A：设备只读 RAM 布局预检（实际传输前硬门）

设备目前保持断电。重新上电后先只执行只读命令并记录原始串口输出：

- `version`
- `bdinfo`
- `pa220 memory`
- `namedprint`
- `freeprint`
- `printenv loadaddr autoload ipaddr serverip ethact ethprime`

核对：
1. 当前运行镜像版本/构建标识与待测 RAM 启动镜像一致。
2. `__tmp_load` descriptor 存在，名称、base、size 与 validator 使用的 globals 一致。
3. descriptor 位于实际 DRAM，不跨 boot-bus hole，不覆盖 U-Boot code/data、relocation、stack、GD、malloc、exception、FDT、environment、Linux reserved 等区域。
4. 当前 `loadaddr` 规范化后落在 `__tmp_load` 内；测试文件的完整 reservation 不越界。
5. `autoload=n` 已由只读 `printenv` 确认。若不是 n，不执行 DHCP/TFTP，也不在本阶段自行改变或保存环境。

在以上核对完成前，禁止执行 `tftpboot`、`tftpsrv`、`tftpput` 或任何文件接收命令。不能仅凭历史 `0x20000000` 或 Stage3 地址 `0x81000000` 推定当前缓冲区安全。

## 4. P6-B：首个 RAM-only TFTP GET 验收

仅在 P6-A 通过并获得明确开始测试的确认后执行：

- 使用维护主机提供的、非固件可执行内容的小型固定测试文件；首轮建议 4 KiB，之后再逐级增加。
- 传输前记录主机端文件长度和 SHA-256；接收后在设备端核对文件长度与 CRC/SHA。若当前镜像没有可用 SHA-256 命令，先实现只读校验命令并做 Host/目标验证，不以 CRC 代替 SHA 的证据。
- 传输前记录 `loadaddr`、`__tmp_load` base/size；传输后记录 `filesize`、`load_addr`、接收结束状态和校验值。
- 覆盖一个合法小文件、精确边界、越界拒绝和中断/超时失败；拒绝/中断后不得执行或启动残缺数据。
- 仅写 RAM；不运行 `bootm`、`bootelf`、`booti`、`bootcmd`，不执行安装命令。

P6-B 只验证 TFTP GET，不代表 TFTP Server/WRQ、PUT、HTTP upload 或 ELF loader 写入安全。

## 5. P6-C：校验与格式识别（P6-1C 后续软件门）

P6-1B 已完成最大文件长度、完整 reservation、DATA block/offset/总长度 checked arithmetic，以及超时/中断/越界/失败状态管理。首次真实接收前仍须完成：

- 接收结束后计算 SHA-256（首选）和/或 CRC32，输出长度、校验值、格式类型。
- 格式识别仅做只读 magic/header 检查（例如 legacy uImage、FIT/FDT、ELF）；不解析后即执行、不写介质。
- 校验不匹配或格式不支持时明确标记 FAILED，清除/作废部分接收结果，不得发布有效文件元数据。
- 完成 P6-1C Host 与 OCTEON 验证后，才进入 P6-A live descriptor 只读预检；之后 P6-B 才考虑真实 RAM-only TFTP GET。

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

### 固定测试文件（设计，不代表已传输）

- 文件长度：4,096 bytes。
- 内容：重复 ASCII 字符串 `PA220-P6-TFTP-GET-TEST\\n`，截断到恰好 4,096 bytes。
- CRC32（IEEE）：`e32b0c57`。
- SHA-256：`dac1fcd877c5ef356b2cdd44f6202f154a2d393cd458975b521ef6cfc05b8356`。
- 目标 block size：512 bytes，必须从实际协商结果/接收端日志确认；如果实际值不是 512，先停止并重新计算预期 block 数，不得套用旧预期。
- 512-byte block 下预期：8 个完整 DATA block + 第 9 个零长度终止 DATA block；最终文件长度 4,096 bytes。

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

P6-1B 已完成生产实现、Host GCC/ASan/UBSan、OCTEON object、N32 relocatable link、完整 U-Boot final link 和独立补丁重复生成验证。P6-1B 仍不授权真实 TFTP GET；实际接收继续等待 P6-1C 校验/格式识别和 P6-A live descriptor 只读预检。

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

完成 P6-1B 及后续 P6 软件接收生命周期和校验安全门后，按需要安排设备只读预检及 RAM-only 接收测试。最终 Web 页面另按 `doc/pa220/WEB-UI-DESIGN.md` 实施，视觉参考 uBootKit Argon 风格；Web UI 不因视觉参考而开放未经验证的写功能。


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

P6-1A 仅完成 GET 开始前旧元数据失效，不构成真实 TFTP GET 授权；完整文件 reservation、最大长度、失败状态、校验/格式识别及 live descriptor 预检仍未完成。

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

- 未执行真实 TFTP GET、未传输文件、未调用 saveenv，未写入 SPI/eMMC/NAND/EEPROM/CPLD/flash。
- 下一软件工作单元为 P6-1C：接收后 SHA-256/CRC32 与只读格式识别；随后执行 P6-A live __tmp_load descriptor 只读预检。P6-B 真实 RAM-only GET 需等待全部软件门和用户明确开始确认。