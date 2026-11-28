# PA-220 原机 Linux 3.10.87 boot handoff 诊断

日期：2026-11-28  
状态：诊断构建已通过；下一步等待 RAM-only 实机回归  
固定源码：`sdzbzyh/pa220-openwrt@801717ba6a8a59ed2cfd53bf3e1b19841fecd2e8`

## 1. 实机现象

用户从原机 eMMC p3 加载原机内核：

```text
ext2load mmc 0:3 0x22000000 /boot/vmlinux-3.10.87-oct3-mp
10387920 bytes read
bootoctlinux 0x22000000 numcores=4 endbootargs console=ttyS0,115200 nousb root=/dev/mmcblk0p3 rootfstype=ext3 rootflags=ro,noload ro rootwait init=/bin/sh
```

内核成功进入 Linux 3.10.87-oct3-mp，U-Boot 启动日志报告 coremask 0xf，但 Linux 最终只识别 1 个 CPU。最关键的两条输出：

```text
Using internal Device Tree.
Waiting for root device /dev/mmcblk0p3...
```

U-Boot 的 `ext2load mmc 0:3` 已成功读取内核，说明 U-Boot 阶段能够访问该 eMMC 分区；Linux 未出现 MMC host/block 设备初始化，不能据此认定 eMMC 分区或文件损坏。

本次命令行显式包含 `nousb`，因此 `usbcore: USB support disabled` 与该启动参数一致；此行本身不能证明内核未编译 USB 支持。USB 实际探测应在 FDT handoff 确认后，另行去掉 `nousb` 测试。

## 2. OEM FDT 与当前重建 FDT 的只读比较

证据来自 Actions #46131655103 的 SPI Stage-3 / 重建 U-Boot FDT 对比：

| 项目 | 原机 SPI Stage 3 FDT | 当前重建 U-Boot FDT |
|---|---:|---:|
| SPI/U-Boot image 内偏移 | 0x1c9460 | 0x113910 |
| FDT 大小 | 10,850 bytes | 10,277 bytes |
| 节点数 | 59 | 52 |
| model | pan,pa-220 | pan,pa-220 |
| compatible | cavium,tb7100 | pan,pa-220, cavium,tb7100 |

**重要：当前重建 U-Boot FDT 本身已经包含 eMMC 和 CN7130 USB 节点**，且两份 FDT 中这些节点的 compatible 与寄存器范围一致：

- eMMC：`/soc@0/mmc@1180000002000`，`cavium,octeon-6130-mmc`
- USB UCTL：`/soc@0/uctl@1180068000000` 与 `/soc@0/uctl@1180069000000`
- xHCI：`0x1680000000000` 与 `0x1690000000000`

审计 JSON 中 `status: ABSENT` 表示 DT 节点没有显式的 `status` 属性，不表示节点被禁用；设备树中缺省状态按 enabled 处理。

两份 FDT 的 MMC/USB power GPIO phandle 数值不同（OEM 为 2，重建版为 11），但重建 DTS 中这些属性均通过同一个 `&gpio` 标签引用 GPIO 控制器，GPIO 编号分别仍为 8、2、3。其编译后 phandle 目标仍需在后续精确比对中保留核验。

重建 FDT 相对 OEM FDT 缺少部分 LED、风扇、EEPROM 与温度监测节点；这些差异不是当前 eMMC host 未出现的首要解释。核心问题是 Linux 输出 `Using internal Device Tree.`，说明本次启动没有采用传入的 PA-220 专用 FDT。

完整机器报告：`reports/pa220/P7-4/boot-handoff-audit/boot-handoff-audit.json`；Actions 构建日志与完整反汇编在 artifact `15257646907` 中。

## 3. Boot descriptor 地址核验

旧诊断输出：

```text
PA220-HANDOFF: array=1.4 fdt=0xfff2da0 copy=1.4 fdt=0xfff2da0
PA220-HANDOFF: src=00080000 new=0fff2da0 new_phys=0xfff2da0 size=10240 copied=0fff2da0 check=0 total=9253
PA220-HANDOFF: desc=c025ef18 version=7 size=400 argc=8 flags=0x5 coremask=0xf boot_desc_copy=0x7ffffe60
```


说明：旧日志中的 `desc_fdt=0x7ffffd40` 是诊断打印标签不准确；该值来自 `boot_desc.cvmx_desc_vaddr`，指向 `cvmx_bootinfo_t` 结构，不是 FDT 本体地址。实际 FDT 地址是同一行的 `fdt=0x0fff2da0`。新 `PA220-START` 输出会把 bootinfo 指针与 `fdt_addr` 分开打印。

FDT header 检查成功，但 Linux 仍选择内部 FDT。此前曾提出 `0xc025ef18` 可能落入内核 ELF 加载范围的假设；该地址是 U-Boot 虚拟地址，不能直接将其低 32 位当作物理地址，因此**该重叠假设尚未证实，不得据此直接修改 descriptor 指针**。

## 4. 新诊断 U-Boot

诊断生成器已扩展为同时修改：

- `octeon_boot.c`：记录 bootinfo/FDT 副本及头部信息；
- `cmd_octeon_linux.c:start_linux()`：记录 Linux 将收到的 a3、全局 descriptor 的 TLB 物理地址、独立 descriptor 副本地址、两份 descriptor 比较结果，以及 Linux 将读取的 bootinfo 版本、core_mask、fdt_addr 和 phy_mem_desc_addr。

新日志前缀：

```text
PA220-START: a3=...
PA220-START: global_desc ... copy_desc ... cmp=...
PA220-START: bootinfo=... version=... core_mask=... fdt=...
```

这样可以用实际 TLB 转换结果判断 descriptor 的真实物理位置，避免从虚拟地址猜测。

- 诊断生成器提交：`d74ca2614483fddac1a64bc50aefbb2f78159e6e`
- Actions：[#46131655103](https://github.com/sdzbzyh/uboot-qsdk12.5-build/actions/runs/46131655103)，SUCCESS
- 审计归档提交：`e547d90b16399eb779fe685003bf34ad2800aa25`
- RAM 测试 BIN：1,403,592 bytes
- SHA-256：`0195a96a511caa3ac2a170bc0b2c11175bbf9ea65bf5d78540be1e1c32cc9d0d`
- Artifact：[15257646907](https://github.com/sdzbzyh/uboot-qsdk12.5-build/actions/runs/46131655103/artifacts/15257646907)

## 5. 下一步：单轮 RAM-only 实机诊断

1. 通过既有 TFTP/RAM 方法加载本轮诊断 U-Boot；不写 SPI，不调用 `saveenv`。
2. 从 eMMC p3 加载同一原机内核 `/boot/vmlinux-3.10.87-oct3-mp`。
3. 保持本轮原命令行（包括 `nousb`），避免将 USB 探测变量混入 FDT handoff 诊断。
4. 完整采集 `PA220-HANDOFF`、`PA220-START` 与 Linux 启动输出，重点核对：
   - `a3` 是否指向有效 descriptor；
   - global descriptor 与 copy descriptor 是否一致；
   - Linux 可见 bootinfo 是否为 1.4、`core_mask=0xf`、`fdt_addr` 非零；
   - Linux 是否不再打印 `Using internal Device Tree.`；
   - 是否出现 MMC host/block 初始化并识别 `/dev/mmcblk0p3`。
5. 若 FDT 已采用但 MMC 仍未出现，再按驱动 probe、GPIO、电源、clock 与中断逐项排查；若 FDT 仍回退，依据实际 a3/descriptor/bootinfo 地址继续定位 ABI 或地址转换问题。

本轮只读取 eMMC 文件并在 RAM 中启动；不写 eMMC、SPI、环境变量或其他持久化介质。原机 Linux 启动仍为未验收状态。


## 6. 2026-11-28 诊断 U-Boot 实机回归结果（已执行）

用户已完成上一节要求的 RAM-only 测试；本节取代“等待 RAM-only 实机回归”的状态，不再重复该轮命令。

关键输出：

```text
PA220-HANDOFF: build ABI=1.4 offsets major=0 minor=4 fdt=152 size=288
PA220-HANDOFF: array=1.4 fdt=0xfff2da0 copy=1.4 fdt=0xfff2da0
PA220-HANDOFF: src=00080000 new=0fff2da0 new_phys=0xfff2da0 size=10240 copied=0fff2da0 check=0 total=9253
PA220-HANDOFF: desc=c025f118 version=7 size=400 argc=8 flags=0x5 coremask=0xf boot_desc_copy=0x7ffffe60
PA220-START: a3=0x800000020f25f118 global=c025f118 global_phys=0x20f25f118 copy_phys=0x7ffffe60
PA220-START: global_desc cvmx=0x7ffffd40 argc=8 flags=0x5 coremask=0xf; copy_desc cvmx=0x7ffffd40 argc=8 flags=0x5 coremask=0xf cmp=0
PA220-START: bootinfo=7ffffd40 version=1.4 core_mask=0xf fdt=0xfff2da0 phy_mem=0x6c108
```

Linux nevertheless prints `Using internal Device Tree.` and later waits for `/dev/mmcblk0p3`. The kernel reports only one CPU online. Thus the U-Boot-side descriptor snapshot is internally consistent, but the kernel has not demonstrated that it consumed the same bootinfo/FDT values.

A source-level cross-check of the OCTEON Linux boot path shows:
- `prom_init()` obtains the boot descriptor from `fw_arg3`;
- it obtains `octeon_bootinfo` by translating `octeon_boot_desc_ptr->cvmx_desc_vaddr`;
- `device_tree_init()` uses the passed FDT only when the kernel-visible `octeon_bootinfo->minor_version >= 3` and `fdt_addr != 0`; otherwise it selects the built-in OCTEON tree and prints `Using internal Device Tree.`.

Reference implementation for this control flow: Linux `arch/mips/cavium-octeon/setup.c`, lines 632-639 and 1090-1116:
https://cos.googlesource.com/third_party/kernel/+/58bd6e060260d938650687cdd8f02dfafcb51f91/arch/mips/cavium-octeon/setup.c
This is a reference implementation, not yet verified as byte-for-byte identical to the pinned PAN SDK 3.1.2 kernel source.

### Updated diagnosis

- The previous proposed descriptor *copy mismatch* is ruled out at the U-Boot-observed level: `cmp=0`.
- The FDT bytes/header and U-Boot-observed bootinfo fields are valid/nonzero.
- The remaining discrepancy is at the boundary between the values printed by U-Boot and the values actually consumed by Linux: Linux's `fw_arg3 -> cvmx_desc_vaddr -> octeon_bootinfo` resolution and the kernel's bootinfo structure layout/address translation must be traced against the exact PAN kernel build.
- Do not change the FDT contents, GPIO phandles, descriptor pointer, coremask, or USB settings by guesswork. In particular, `nousb` remains intentional for this test and explains the USB-disabled line.
- The eMMC root wait is currently a downstream symptom of the absent Linux MMC host/block device; do not repeat partition, filesystem-label, or kernel-file discovery.

### Next work unit — source-level ABI reconciliation (no new device round yet)

1. Inspect the exact pinned U-Boot `bootoctlinux` / `start_linux()` handoff code and the PAN Linux 3.10.87 `prom_init()` / `device_tree_init()` / `cvmx-bootinfo.h` definitions.
2. Compare the exact byte offsets and address domains for `fw_arg3`, `cvmx_desc_vaddr`, bootinfo major/minor, `core_mask`, and `fdt_addr`; include big-endian MIPS64 and the kernel's `cvmx_phys_to_ptr()` semantics.
3. Establish why the live kernel behaves as if `minor_version < 3` or `fdt_addr == 0` despite the U-Boot-side print. Also reconcile why Linux brings up one CPU despite the descriptor coremask being 0xf.
4. Only after the source trace identifies one concrete candidate change, build one instrumented or corrected RAM-only test image. The next device round must add a kernel-visible early diagnostic or an equivalent observable proof; do not repeat the current U-Boot-only printout.
5. Once Linux confirms the passed FDT, continue to MMC host probe and root-device startup. Only after that test remove `nousb` in a separate USB test.

No persistent storage was written; no `saveenv`, SPI/eMMC write, erase, partition modification, or filesystem repair was performed.
