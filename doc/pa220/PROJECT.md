# PA-220 U-BootKit 项目章程

## 1. 项目目标

将 uBootKit 中所有经过评估、能够在 PA-220 上合理实现的功能，系统性移植到 PA-220 专用 U-Boot。目标不仅是增加若干命令，还包括建立可通过串口和 MGT 网口使用的维护环境，并尽可能提供信息展示、网络维护、镜像识别、存储维护和启动管理能力。

“完整适配”指完整评估功能并实现所有适用项，不代表照搬 Qualcomm IPQ 平台代码，也不代表把不兼容的功能强行移植。

## 2. 平台事实与约束

- 设备：Palo Alto Networks PA-220，Cavium OCTEON III CN7130，board type 20021（KINGFISHER）。
- CPU/ABI：MIPS64，大端，n32 ABI（以 PA-220 源码配置为准）。
- U-Boot：Cavium OCTEON SDK 5.1.0，U-Boot 2013.07。
- 启动角色：自定义 U-Boot 是 SPI 启动链的 Stage 3，由 Palo Alto 原厂前级 Stage 2 加载；原厂前级已经初始化 DRAM。它不是可独立替换原厂前级的 Boot ROM。
- 链接地址：0xC0000000；安装资料中的 RAM 测试加载地址：0x81000000。二者用途不同。
- 主要维护接口：串口与 MGT 网口。Web 管理优先限制在 MGT 维护网络。
- 存储：板载 eMMC、正常/备用 SPI NOR、板载 EEPROM；USB 控制器和 CPLD 由板级资料定义，实机能力仍须验证。

源码配置或 DTS 中存在某个驱动，不等于硬件已经实测可用。所有功能状态必须区分配置、编译、RAM 启动和实机验证。

## 3. 仓库与版本基线

### 集成仓库

- 仓库：sdzbzyh/uboot-qsdk12.5-build
- Qualcomm 原始主线：main，基线 985598df27d432df36fb840e9271cca349c0e497
- PA-220 独立开发分支：feature/pa220-octeon
- PA-220 维护分支：feature/pa220-maintenance
- 当前 P0 文档落点：feature/pa220-ubootkit-web。审计时该分支相对 feature/pa220-octeon 前进 7 个提交，包含维护命令补丁和维护路线文档。此分支选择仅用于承接本轮已有工作，不改变 main，也不自动合并其他分支。

### PA-220 源码

- 原始上游：IronDemiGod/pa220-openwrt
- 当前集成脚本使用的镜像/派生仓库：sdzbzyh/pa220-openwrt
- 固定源码 commit：801717ba6a8a59ed2cfd53bf3e1b19841fecd2e8
- U-Boot 子目录：u-boot/

正式开发必须明确区分原始上游、派生仓库和集成仓库。除非专门批准升级基线，不得静默跟随上游 HEAD。

### 构建入口

- 独立入口：./build-pa220.sh
- 默认产物：bin/pa220/u-boot-octeon_pa220.bin
- 工具链：OCTEON SDK MIPS64 交叉工具链，包含 mips64-octeon-linux-gnu-gcc
- PA-220 补丁：patches/pa220/*.patch
- Qualcomm 原有 build.sh、u-boot-2016/ 与设备构建逻辑必须保持独立。

## 4. 范围

纳入评估：
1. 串口命令行、状态信息与诊断命令。
2. Web HTTP 只读状态页及交互式维护界面。
3. MGT 网络配置、DHCP Client、Ping、DNS、TFTP、HTTP 下载等网络能力。
4. 文件上传、RAM 暂存、长度与校验和检查。
5. eMMC、SPI NOR、USB 存储、分区表和文件系统访问。
6. 固件格式识别、安装预览、安装事务、读回验证和恢复。
7. 启动项管理、正常/备用内核、RAM 启动和失败回退。
8. 板载 EEPROM、CPLD、LED、温度及其他可读取硬件状态。

排除或单独标记：
- Qualcomm 9008、bootipq、MBN、IPQ PPE/GMAC、IPQ 专用设备树及其寄存器接口。
- 无 PA-220 对应硬件的按键/LED/无线功能。
- 未经设备证据支持的自动 SPI 写入或 CPLD flash-select 切换。

## 5. 安全原则

1. 默认 RAM-only：构建、TFTP 下载和 RAM 启动测试不得写入 SPI/eMMC。
2. 不执行 saveenv：PA-220 当前环境位于 SPI NOR，主区 0xe00000、冗余区 0xe10000。
3. 不执行 sf erase/write/update，不写 CPLD 寄存器；除非后续独立高风险节点完成设计审查、备份验证和用户明确授权。
4. 任何存储写入功能必须有目标设备/分区/偏移/长度显示、边界校验、dry-run、读回校验和恢复方案。
5. Web 初期只读；上传阶段只允许写入已证明安全的 RAM 缓冲区。不得由 HTTP 请求直接触发持久化写入。
6. 不把“能编译”描述为“硬件可用”，不把源码文档中的 SPI 地址当作所有硬件修订版的实测事实。

## 6. 节点交付规则

每个节点必须有：
- 节点开始报告：范围、基线、输入资料、风险和完成标准。
- 仓库内正式文档或代码变更。
- 验证记录：命令、结果、未验证项。
- 节点结束报告：提交、变更清单、测试结果、遗留问题和下一节点入口。

每个会话原则上只推进一个节点。遇到资料不足时建立独立资料请求/研究任务，不用猜测填补空白。