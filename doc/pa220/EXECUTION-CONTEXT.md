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

## 当前唯一主线（2026-11-28，P7-3B 原机 Linux 启动路径验证）

P7-3A 只读 MBR/EBR inventory 已验收；P7-3B planner 与 `pa220 install dry-run` 已完成软件实现和云端构建，但整体验收仍需完成原机 eMMC Linux 启动路径验证。当前不要推进任何 eMMC/SPI 安装写入。

- P7 计划：`doc/pa220/P7-PLAN.md`。
- P7-3B 实施报告：`reports/pa220/P7-3/P7-3B-IMPLEMENTATION.md`。
- P7-3B 实机报告：`reports/pa220/P7-3/P7-3B-DEVICE-ACCEPTANCE.md`。
- 本轮新增原机 Linux handoff 诊断报告：`reports/pa220/P7-3B/ORIGINAL-KERNEL-HANDOFF-DIAGNOSTIC.md`。
- 原机 SPI Stage-3 与当前重建 U-Boot FDT 审计：`reports/pa220/P7-4/boot-handoff-audit/boot-handoff-audit.json`；诊断 patch：`reports/pa220/P7-4/0001-pa220-fdt-handoff-diagnostics.patch`。

### 原机 Linux 启动现状

- 已从 eMMC p3 成功读取原机内核 `/boot/vmlinux-3.10.87-oct3-mp`（10,387,920 bytes），并通过 `bootoctlinux` 进入 Linux 3.10.87-oct3-mp。
- U-Boot 报告 coremask `0xf`，Linux 实际只识别 1 个 CPU。
- Linux 打印 `Using internal Device Tree.`，随后等待 `/dev/mmcblk0p3`；当前不能标记原机启动通过。
- 本轮命令行包含 `nousb`，因此 `usbcore: USB support disabled` 与该参数一致；不能用这一行推断内核未编译 USB 支持。
- Actions #46131655103 的只读 FDT 审计确认：当前重建 U-Boot FDT 与 OEM FDT 都包含 CN7130 eMMC host/slot 和 USB UCTL/xHCI 节点；因此当前首要问题是 Linux 没有采用传入的 PA-220 FDT，而不是重建 FDT 中完全缺少 MMC/USB 节点。
- OEM FDT 59 nodes / 10,850 bytes；重建 FDT 52 nodes / 10,277 bytes。差异包括部分 LED、风扇、EEPROM、温度节点及 root compatible / GPIO phandle 数值；详细内容以 P7-4 审计报告为准。

### 当前诊断构建与下一步

- 诊断生成器提交：`d74ca2614483fddac1a64bc50aefbb2f78159e6e`；报告已更新记录实际回归结果，见 `reports/pa220/P7-3B/ORIGINAL-KERNEL-HANDOFF-DIAGNOSTIC.md`。
- Actions [#46131655103](https://github.com/sdzbzyh/uboot-qsdk12.5-build/actions/runs/46131655103) SUCCESS；Artifact [15257646907](https://github.com/sdzbzyh/uboot-qsdk12.5-build/actions/runs/46131655103/artifacts/15257646907)。
- 诊断 BIN SHA-256：`0195a96a511caa3ac2a170bc0b2c11175bbf9ea65bf5d78540be1e1c32cc9d0d`。
- 2026-11-28 实机回归已完成：U-Boot 打印 bootinfo 1.4、`core_mask=0xf`、非零 `fdt_addr=0xfff2da0`；global/copy descriptor `cmp=0`。但 Linux 仍打印 `Using internal Device Tree.`，只启动一个 CPU，且没有 MMC host/block 初始化，最终等待 `/dev/mmcblk0p3`。
- 因此，U-Boot 侧 descriptor 一致性检查已完成，但尚未证明 Linux 实际读取了同一份 bootinfo。不能重复本轮相同 U-Boot-only 诊断，也不能猜测修改 FDT/descriptor 地址。
- 固定 U-Boot 源码的 `start_linux()` 已核实：`a3/fw_arg3` 指向 XKPHYS boot descriptor，descriptor 的 `cvmx_desc_vaddr` 指向低 2 GiB 物理 bootinfo；bootinfo major/minor 与 FDT 字段偏移符合 U-Boot ABI 1.4。U-Boot 也确实将 bootinfo 复制到该物理地址。仍未获得 PAN SDK-3.1.2 内核的确切 `prom_init()/device_tree_init()/cvmx-bootinfo.h/cvmx_phys_to_ptr()` 实现，故 Linux 仍回退 internal FDT 的根因未定。下一步只继续固定内核 ABI 源码/二进制对账，并追溯命令行中 `cpuid=0` 的来源；不生成新设备镜像、不重复实机启动。详细源码审计与待办见 `reports/pa220/P7-3B/ORIGINAL-KERNEL-HANDOFF-DIAGNOSTIC.md`。
- 保持 `nousb`，直到 FDT handoff 与 eMMC root 路径确认；之后再单独移除 `nousb` 测 USB。不要重复分区、标签、内核文件发现工作。


P4 DHCP Client/Ping 实机验证仍是独立遗留项，P4 overall 不得标记 COMPLETE。

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


## P7-2B 实机验收闭环（2026-11-27）

- 新 Stage3 U-Boot build time：Nov 27 2026 09:04:34；TFTP host 192.168.2.10，PA-220 192.168.2.2，接口 octrgmii0。
- 正常 FIT：2,518 bytes，SHA-256 `db96358852a1c47fe7788c5ecf89093daf6d26142b3e946a3f8ca7a1d29ff565`；kernel/ramdisk CRC32、SHA1、MD5 PASS；ignored hash IGNORED；SHA-256 UNSUPPORTED；signature NOT_VERIFIED；external data unsupported；default config refs RESOLVED；`hash_failures=0`。
- 损坏 FIT：2,518 bytes，SHA-256 `a2cc42cf33f94a5ef306068fb9e88a8b84e746fc672379227494aac1ea504f02`；kernel CRC32/SHA1/MD5 FAIL；ramdisk 三种 hash PASS；其他分类与预期一致；`hash_failures=1`。
- 两个向量均显示 target mapping UNMAPPED、actions NONE。仅操作 RAM 中已接收的 FIT 文件；无加载、启动、存储读取/写入或 saveenv。
- 报告：`reports/pa220/P7-2/FIT-IMPLEMENTATION.md`。P7-2B PASS；P7-2A/B/C 全部完成，下一工作单元 P7-3。


## 2026-11-28 live eMMC 启动路径补充

P7-3B dry-run live acceptance 已通过，实际根分区/内核路径与旧板级注释不一致。p2=`sysroot0`，p3=`sysroot1`；p3 `/etc/fstab` 指向 `LABEL=sysroot1`；内核 ELF 位于 p3 `/boot/vmlinux-3.10.87-oct3-mp`。旧 `run bootcmd` 因 p1 两个候选文件均不存在而失败，未执行 Linux kernel。详细证据和待核实项见 `reports/pa220/P7-3/P7-3B-DEVICE-ACCEPTANCE.md`。

## P7-3B PAN kernel bootinfo ABI compatibility build (2026-11-28)

- Exact original kernel disassembly identified a 24-byte PAN-specific bootinfo extension: extension bytes 152..175, FDT pointer at 176, extended coremask at 184. The pinned SDK 5.1.0 U-Boot structure instead places FDT at 152 and extended coremask at 160 (288-byte structure). This explains the kernel's generated cpuid=0 and its fallback to the internal device tree.
- Compatibility serializer: `patches/pa220/0006-pa220-pan-kernel-bootinfo-compat.patch`; generator: `scripts/generate-pa220-pan-kernel-bootinfo-compat-patch.py`.
- The Linux handoff reserves 312 bytes in both hplug allocation and normal heap-adjacent paths. The kernel-visible copy contains profile 0, physical core ID, UART 0, board name PA-220, FDT at 176, and a clean 128-byte extended coremask at 184 synthesized from legacy `core_mask`. Generic in-memory U-Boot bootinfo remains unchanged.
- Actions [#46134874478](https://github.com/sdzbzyh/uboot-qsdk12.5-build/actions/runs/46134874478) SUCCESS; Artifact [15258983126](https://github.com/sdzbzyh/uboot-qsdk12.5-build/actions/runs/46134874478/artifacts/15258983126).
- RAM-test image: `artifacts/pa220/builds/pa220-pan-kernel-abi-compat.bin`; size 1,402,792 bytes; SHA-256 `318c319b824603ee3080ab12687e940ec317c9216566b1986ea8a9bde9048d1d`.
- The first compile-only image from Actions #46134530956 is invalid for testing: its normal non-hplug path still reserved only 288 bytes. The corrected #46134874478 build reserves all 312 bytes in both paths. Do not use the first artifact.
- P7-3B is NOT yet accepted. The next step is one RAM-only physical test with the original eMMC kernel on p3, omitting `nousb`, and checking the PA220-PAN-ABI line, passed-FDT selection, four CPUs, USB, and eMMC root mount. No SPI/eMMC writes.


## PAN-OS 用户空间启动停滞：暂缓遗留项（2026-11-28）

- 实机已能从 eMMC p3 读取原机内核 `/boot/vmlinux-3.10.87-oct3-mp`（10,387,920 bytes），并进入 Linux/PAN-OS 启动流程；串口在 9600 baud 下最终停留于 `PA-HDF login:`，等待超过一小时未自行转为 `PA-220 login:`。
- 启动日志曾出现 `/sbin/cgroups_setup` 的 Python `TypeError: 'NoneType' object is not subscriptable`；该异常与后续停滞存在时间相关性，但尚未证明为根因。其他服务启动失败信息也尚未逐项归因。
- 结论：这是 PAN-OS 用户空间/服务初始化尚未完成的已知问题，不阻塞 U-BootKit 其他功能开发；但原机 PAN-OS 完整启动与端到端兼容性仍为 NOT ACCEPTED，不得将“内核已启动/进入 PA-HDF 提示符”表述为系统启动通过。
- 处理策略：当前主线继续推进 P7 只读解析、安装 dry-run、启动兼容和 U-BootKit Web UI；该问题列入竣工后遗留项。竣工后再以 RAM-only、rootfs 只读方式诊断 PAN-OS init/cgroups/service 启动链。
- 约束：本轮未写 SPI/eMMC，未执行 `saveenv`，未修改原机 rootfs。除非后续发现该停滞直接阻断某项当前验收，不主动回头处理。


## P7-4 当前进度补充（2026-11-28）

- 最新用户实机观察：从 eMMC p3 加载原机 kernel 后进入 `PA-HDF login:` 中间提示符，超过一小时未继续；该结果说明启动比早期 root-device wait 阶段更深入，但没有完整早期日志和准确 Stage3 SHA，故仅记录为 partial kernel/userspace progress，不作为 ABI/FDT/CPU 验收。该命令含 `nousb`，USB 未测试。详情见 `reports/pa220/P7-3/P7-3B-IMPLEMENTATION.md`，停滞本身按 D-027 暂缓。
- OEM FDT 与重建 FDT 均含 MMC 与 CN7130 USB 节点；先前 raw GPIO phandle 2/11 的差异不能直接解释为接线不同，因为 phandle 是 tree-local identifier。
- 已扩展 `scripts/pa220/boot-handoff-audit.py`：解析 GPIO phandle 实际目标路径、记录 OEM/重建 FDT SHA-256，并导出两份 DTB。已把该只读审计接入 `.github/workflows/pa220-pan-kernel-abi-compat-build.yml`，构建时用原始 SPI 备份和新鲜 U-Boot BIN 复核。最新工作流提交为 `478597e5bf4c6af1a77cbae75db708b330331e70`；等待 Actions 结果后再决定是否需要 FDT 代码修改。
- 当前不重复 PA-HDF 等待测试，不猜测修改 GPIO/FDT，不写 SPI/eMMC，不执行 `saveenv`。
