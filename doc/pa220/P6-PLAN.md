# PA-220 P6 计划：RAM-only 文件接收与校验

- 状态：**P6 已进入设计/验收准备；未开放实际接收；未执行任何文件传输**
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

## 5. P6-C：长度、校验与格式识别

首次真实接收前完成相应软件实现与测试：
- 明确最大文件长度；上限不得超过 live `__tmp_load` 可用区间，并在接收开始前验证完整 reservation。
- 每个 DATA block 继续在 memcpy 前验证；block index、wrap offset、目标地址、文件总长度使用 checked arithmetic。
- 接收结束后计算 SHA-256（首选）和/或 CRC32，输出长度、校验值、格式类型。
- 格式识别仅做只读 magic/header 检查（例如 legacy uImage、FIT/FDT、ELF）；不解析后即执行、不写介质。
- 失败状态明确区分超长、越界、超时、校验不匹配和不支持的格式；不将部分文件标记为有效。

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
