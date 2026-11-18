# P1-A2：升级模块与持久化写入路径审计（最后一组）

- 状态：**源码静态审计完成；未移植、未编译、未实机验证**
- 日期：2026-11-18
- uBootKit：chenxin527/uboot-qsdk12.5-build@9ce315735ac88fd0a7c7697bc527a8616ed0a972
- PA-220：sdzbzyh/pa220-openwrt@801717ba6a8a59ed2cfd53bf3e1b19841fecd2e8
- 文档分支：feature/pa220-ubootkit-web
- 范围：upgrade.c、board/qca/arm/common/failsafe.c、fw_dec.c、untar.c、lib/bootconfig.c；追踪所有由 Web 上传流程触发的升级/启动/持久化路径。
- 本报告完成 P1-A2 最后一组 upgrade 审计；P1-A2 业务模块源码审计范围至此覆盖用户指定的全部模块。

## 1. 构建入口和全局状态

### 1.1 Makefile
- u-boot-2016/Makefile:704：`libs-$(CONFIG_HTTPD) += failsafe/`。
- failsafe/Makefile:3-6：非 SPL 时编入 modules/、fs.o、failsafe.o。
- failsafe/modules/Makefile:11：upgrade.o 为 obj-y。
- board/qca/arm/common/Makefile:19：`obj-$(CONFIG_HTTPD) += failsafe.o fw_dec.o untar.o`。
- 因此升级 handler、镜像识别/验证/写入实现和 untar 命令均受 CONFIG_HTTPD 构建条件控制。没有目标 .config/构建日志，CONFIG_HTTPD 实际启用与编译成功仍 NOT VERIFIED。

### 1.2 全局状态
upgrade.c:
- `auto_action_pending`（49）：HTTP 服务退出后是否执行自动动作。
- `upload_data`（50）：上传数据指针。
- `upgrade_type`（51）：升级类型。
- `fs_upload_id`、`upload_data_id`、`upload_size`（53-55）：上传会话识别和文件大小。
board/qca/arm/common/failsafe.c:
- `runcmd` 指向 failsafe_write_image() 栈上的命令列表；最多 10 条，每条最多 255 字符。
- `fw_type`、`info`、`resp`、`factory_fw_kernel_size` 等为模块级全局状态。
这些变量不是每个上传任务独立的持久化对象；并发 HTTP 上传/结果会话是否被 HTTPD 串行化，本次未证明，标记 NOT VERIFIED。

## 2. 前端两阶段上传流程

固定前端 failsafe/fsdata/main.js:
- UploadManager.upload()（约 1900-1975）：把所选文件作为 FormData，字段名由页面配置决定，POST 到 /upload；MIBIB 页面特殊 POST /mibib/reload。
- upgrade.c:36-47 的 upgrade_types 映射 firmware/uboot/art/cdt/ptable/simg/initramfs。
- upload_handler()（57-152）NEW 时分配 session、启动 IPQ LED 状态；RESPONDING 时从表单查找 upgrade type，记录全局上传指针/长度和随机 ID，然后调用 failsafe_validate_image()；CLOSED 时按校验结果更新 LED。该阶段本身不调用 failsafe_write_image()。
- UploadManager.continue()（约 1978-2040）：用户在上传校验成功后点击继续，POST /result，FormData 字段 auto_reboot=true/false。
- result_handler()（154-260）NEW 时读取 auto_reboot；RESPONDING 时检查 upload_data_id == fs_upload_id。若匹配且不是 INITRAMFS，调用 failsafe_write_image()；若为 INITRAMFS，则跳过写入，标记成功并强制 auto_reboot=true。CLOSED 时计算 auto_action_pending，释放 session，并关闭 TCP 连接。
- failsafe_loop()（failsafe.c:147-160）在 auto_action_pending 时退出；do_httpd()（failsafe.c:232-245）随后执行 boot_from_mem()（INITRAMFS）或 do_reset()（其他成功且 auto_reboot 的升级）。
- 关键边界：上传成功响应只代表 failsafe_validate_image() 通过，不代表写入已经发生；真正写入发生在第二次 /result 请求。/result 是持久化写入触发点。

## 3. 镜像识别与验证路径（写入前）

### 3.1 check_fw_type()
- 文件：board/qca/arm/common/fw_dec.c:29-76。
- 识别 CDT、ELF、FIT、Legacy Image、MIBIB NOR/NAND、SIMG NAND、Sysupgrade、UBI、GPT 等 magic。
- 依赖 IPQ 专用 get_mibib_and_ptable_addr()；部分 FIT 类型还读取 QSDK header offset 并识别 GL.iNet/JDCloud。
- **严重边界问题：** 函数接收 size 但未使用；直接读取 addr、addr+0x4、+0x40、+0x5C、+0x65、+0x100、+0x1FE、+0x800 等偏移。短上传文件可能触发越界读取。不能把 magic 识别视为安全的长度验证。
- MD5 由 failsafe.c:354-370 计算并展示；MD5 不是签名验证或来源认证。

### 3.2 failsafe_validate_image()
- board/qca/arm/common/failsafe.c:857-915：调用 check_fw_type()，按 upgrade_type 分派 validate_firmware/uboot/art/cdt/ptable/simg/initramfs。
- FIRMWARE：FIT 扫描 1 MiB 对齐位置的 SquashFS magic，估算 kernel/rootfs 切分并检查目标分区容量；Sysupgrade/ASUSWRT 解析 tar 内容并检查 HLOS/rootfs；GL.iNet/JDCloud 检查相应 FIT 节点；UBI 检查 rootfs 容量。
- UBOOT：只要求 FW_TYPE_ELF，再检查 0:APPSBL 分区容量。
- ART：因没有固定 magic，采用排除已知格式的方式，之后检查 0:ART 容量。
- CDT：检查 CDT magic 和 0:CDT 容量。
- PTABLE：GPT 仅由 FW_TYPE_GPT 识别后允许；MIBIB 检查目标分区容量。
- SIMG：检查设备存在和文件大小不超过整颗 SPI/NAND/eMMC 容量。
- INITRAMFS：只要求 FW_TYPE_FIT，未进一步验证 FIT configuration、load/entry 地址、内存布局或目标架构。
- validate_file_size()（246-296）依赖 SMEM 分区查找、MMC GPT 分区查询及 IPQ has_* API。
- 这一步大部分是 RAM 中解析和读取分区元数据，不执行镜像写入；但其校验深度有限，不应当作安全启动/签名验证。

## 4. failsafe_write_image() 的写入分派

文件：board/qca/arm/common/failsafe.c:917-961。
- 建立本次命令列表并再次 check_fw_type()。
- 按 upgrade_type 调用 failsafe_write_firmware/uboot/art/cdt/ptable/simg。
- 每个分支通过 ADD_CMD_TO_LIST() 生成 U-Boot 命令，最后 failsafe_run_command_list()（466-473）逐条执行。
- failsafe_run_command_capture()（450-464）通过 call_func_capture() 包装 failsafe_run_command()；failsafe_run_command()（445-448）直接调用 run_command(cmd, 0)。
- 命令执行遇到第一条失败即停止，没有事务回滚；此前成功的分区/扇区写入不会自动恢复。

## 5. 所有可能的持久化写入路径清单

下表为 failsafe.c 实际生成的写命令。除 INITRAMFS 外，以下均为持久化写入路径。

| upgrade_type / fw_type | 源码位置 | 实际命令/调用 | 持久化目标和风险 |
|---|---|---|---|
| FIRMWARE + FIT | failsafe.c:673-681 | `flash 0:HLOS`；`flash rootfs` | 写 HLOS/kernel 与 rootfs 分区 |
| FIRMWARE + GL.iNet V3/V4 | 682-687 | `xtract_n_flash ... rootfs` | 解包后写 NAND rootfs |
| FIRMWARE + JDCloud | 688-698、715-718 | 多次 `xtract_n_flash`；`flasherase rootfs_data` | 写 HLOS/rootfs/WIFIFW/BTFW 等分区，并可能擦除 rootfs_data |
| FIRMWARE + Sysupgrade/ASUSWRT | 699-705 | `untar`；`flash 0:HLOS`；`flash rootfs` | untar 只解析并设置 RAM 环境变量；后续 flash 命令写 kernel/rootfs |
| FIRMWARE + UBI | 706-709 | `flash rootfs` | 写 NAND rootfs/UBI 内容 |
| FIRMWARE 附加 bootconfig | 720-722 | `bootconfig set firmware 0` | 可能写 BOOTCONFIG/BOOTCONFIG1；详见第 6 节 |
| UBOOT | failsafe.c:726-741 | `flash 0:APPSBL`；可选 `flash 0:APPSBL_1` | 写 QCA Apps Bootloader 分区；高风险引导链更新 |
| ART | 743-750 | `flash 0:ART` | 写无线校准/ART 分区；可能破坏校准数据 |
| CDT | 752-767 | `flash 0:CDT`；可选 `flash 0:CDT_1` | 写板级配置/设备树相关分区 |
| PTABLE + GPT | 769-781 | `mmc erase 0x0 count`；`mmc write addr 0x0 count` | 擦写 eMMC 起始 LBA，覆盖 GPT/分区表 |
| PTABLE + MIBIB NOR/NAND | 782-795 | `flash 0:MIBIB` | 写 Qualcomm MIBIB 分区表 |
| SIMG + eMMC/GPT | 798-811 | 与 GPT 相同的 `mmc erase` + `mmc write` | 从 LBA0 写整盘镜像，覆盖 GPT 和后续数据 |
| SIMG + NAND | 812-817 | `nand device`；`nand erase`；`nand write` | 擦写整颗 NAND 的镜像范围 |
| SIMG + SPI NOR | 818-827 | `sf probe`；`sf update` | 擦写 SPI NOR 偏移 0 开始的镜像范围 |

### 5.1 写命令的进一步含义
- `flash`、`xtract_n_flash`、`flasherase` 是 QCA/IPQ 平台命令，不是可直接映射到 PA-220 的通用分区 API。
- `mmc erase/write` 是原始块设备写入，GPT/SIMG 路径可能覆盖 LBA0 及整盘数据。
- `nand erase/write` 是 NAND 物理擦写路径。
- `sf update` 会按 SPI NOR 写入规则擦除/写入；不能因为是 U-Boot 命令就当成安全更新。
- 命令列表最多 10 项；顺序执行，前序写成功、后续失败时存在部分升级状态。

## 6. bootconfig 隐式持久化写入

该路径容易被忽略，但必须纳入升级写风险：

1. failsafe.c:720-722 在 firmware 升级路径末尾调用 `validate_bootconfig()`；若返回 true，加入 `bootconfig set firmware 0`。
2. 命令实现位于 lib/bootconfig.c 的 `set_bootconfig()`（351-435）。
3. `set_bootconfig()` 调用 `read_bootconfig_and_write_back_if_needed()`（153-179）。如果主 BOOTCONFIG 读取/校验失败但备份 BOOTCONFIG1 可读，会先将备份数据写回主分区。
4. 修改 firmware 分区的 primaryboot 标志后调用 `write_bootconfig()`（184-214）。
5. `write_bootconfig()` 按 SMEM flash_type 分派 `write_data_to_spi()`、`write_data_to_nand()` 或 `write_data_to_mmc()`。
6. `set_bootconfig()` 最后调用 `sync_bootconfig(false)`（236-295、425-434），将 BOOTCONFIG 同步到 BOOTCONFIG1；若两者不同，会再次调用 write_bootconfig()。
7. 因此一次 firmware 更新可能除了 kernel/rootfs 之外，还写入主/备 BOOTCONFIG 分区；若主配置损坏，还可能先执行恢复写。

## 7. INITRAMFS 路径：不直接写存储，但会启动执行

- upgrade.c:207-210 对 INITRAMFS 特殊处理：不调用 failsafe_write_image()，而是将 auto_reboot 强制设为 true。
- failsafe.c:834-855 的 boot_from_mem() 调用 QCA config_select()，构造 bootm 命令并执行 run_command()。
- do_httpd()（failsafe.c:234-240）在 HTTP 服务退出后调用 boot_from_mem(upload_data)。
- 此路径本身不调用 SPI/NAND/MMC/EEPROM 写函数；它是从上传缓冲区启动的 RAM 启动路径。
- 但仍须验证上传缓冲区生命周期、内存覆盖、FIT load/entry 地址、PA-220 MIPS64 big-endian/n32 架构兼容性、启动参数和 DRAM 保留区。不能把“无持久化写入”误认为“可直接在 PA-220 启动”。

## 8. PA-220 对照和明确禁止的直接移植项

PA-220 固定源码：
- octeon_pa220.h 启用 OCTEON MMC、SPI_FLASH、EFI/GPT；SPI NOR 注释明确为 read access only intended。
- PA-220 eMMC 使用 GPT；没有 QCA SMEM/MIBIB、QCA partition flash、QCA firmware node extraction 机制。
- PA-220 Stage 3 U-Boot 由原厂前级加载，DRAM 已由 Stage 2 初始化；SPI 启动链/板载 CPLD flash-select 和 EEPROM/TLV 结构属于 PA-220 专用设计。
- PA-220 配置中的 `CONFIG_CMD_SF`、`CONFIG_CMD_MMC`、`CONFIG_CMD_SAVEENV`、`CONFIG_CMD_OCTEON_BOOTLOADER_UPDATE` 及文件系统写命令说明其命令环境具备持久化操作能力；这不意味着 Web 可开放这些命令。

逐类型判断：
- FIRMWARE：QCA FIT/Sysupgrade/UBI/厂商分区映射不适用；必须先定义 PA-220 固件格式、GPT 分区白名单和校验流程。
- UBOOT：QCA 0:APPSBL 不等于 PA-220 Stage 3 SPI 布局；不得移植 flash 命令或在线写 bootloader。
- ART：PA-220 MAC/板卡信息来自 TLV EEPROM/OCTEON board API，不存在 QCA ART 分区等价物。
- CDT：QCA CDT 分区不等于 PA-220 FDT/板级数据，不能直接移植。
- PTABLE：PA-220 使用 eMMC GPT，虽然 GPT 结构可识别，但 LBA0 写入是高风险持久化操作；P1 不实现 GPT 写入。
- SIMG：QCA 全芯片镜像写入方式不适用于 PA-220 的 SPI/eMMC 布局，禁止照搬。
- INITRAMFS：只有“上传到 RAM 后启动”的总体思路可能适配；QCA config_select/bootm 和固件识别逻辑必须重写，先验证 FIT 架构、地址和 PA-220 boot command。

## 9. 持久化写入风险结论

- 上传验证与真正写入是两个独立 HTTP 请求；/result 是明确写入入口。
- 所有 firmware、U-Boot、ART、CDT、PTABLE、SIMG 写入路径均通过 run_command 执行 QCA/IPQ 专用命令。
- firmware 还可能隐式写 BOOTCONFIG 和 BOOTCONFIG1。
- 命令失败不会回滚已完成的写入；存在部分更新、不可启动和校准数据丢失风险。
- INITRAMFS 是唯一不调用 failsafe_write_image() 的升级类型；它会在 HTTP 服务退出后直接 boot_from_mem()。
- 本项目 PA-220 适配不得移植 /upload + /result 的写入链；初期只保留经独立审查的 RAM 上传/镜像识别/只读信息展示。所有持久化写操作继续保持禁用，除非完成独立审查并取得用户明确授权。
- 本次未执行任何 SPI/eMMC/NAND/EEPROM/CPLD 写入，未调用 saveenv、sf erase/write/update、mmc erase/write 或 bootloader 更新命令。
