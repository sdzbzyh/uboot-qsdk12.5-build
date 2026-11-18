# P1-A2：Web 业务模块审计（第二组：misc.c、webterm.c）

- 状态：**本组源码审计完成；未移植、未编译、未实机验证**
- 日期：2026-11-18
- uBootKit：chenxin527/uboot-qsdk12.5-build@9ce315735ac88fd0a7c7697bc527a8616ed0a972
- PA-220：sdzbzyh/pa220-openwrt@801717ba6a8a59ed2cfd53bf3e1b19841fecd2e8
- 文档分支：feature/pa220-ubootkit-web
- 前置：P1-A2-READONLY-WEB-MODULES.md（sysinfo/network/env/syslog）
- 范围：misc.c、webterm.c。backup/mibib/mac/upgrade 尚未完成，upgrade 仍最后审计。

## 1. 编译入口

- u-boot-2016/Makefile:704：`libs-$(CONFIG_HTTPD) += failsafe/`。
- failsafe/Makefile：非 SPL 时编入 modules/、fs.o、failsafe.o。
- failsafe/modules/Makefile：misc.o、webterm.o 均为 obj-y，没有独立 CONFIG 开关；因此在 CONFIG_HTTPD 使能且非 SPL 时进入 failsafe 模块构建。
- 本批没有目标 .config/defconfig、编译日志或镜像证据；CONFIG_HTTPD 实际启用、编译成功均 NOT VERIFIED。

## 2. misc.c

### 2.1 页面和静态资源分发

文件：u-boot-2016/failsafe/modules/misc.c。

- `output_plain_file()`（81-147）调用 `fs_find_file()` 查找嵌入式页面资源；未压缩资源直接返回，gzip 资源在客户端支持时发送压缩数据，否则在 CONFIG_GZIP 下分配缓冲区调用 `gunzip()` 解压。
- `gunzip_and_send()`（38-79）将动态解压缓冲区放入 response->gunzip_buffer，供 HTTPD 生命周期释放。
- `index_handler()`（159-165）返回 index.html；`html_handler()`（233-242）根据 URI 文件名返回页面；`upload_html_handler()`（249-287）生成动态上传页面。
- `index_register_uri_handlers()`（289-295）、`html_register_uri_handlers()`（297-317）、`misc_register_uri_handlers()`（319-328）分别注册首页、页面、/main.js、/style.css、/version、/reboot 和 not-found handler。
- 页面静态资源与 HTTPD/FS API 耦合；PA-220 可复用资源组织和 gzip 协商思路，但需要适配旧 U-Boot 的 HTTP response 生命周期、gzip 配置和静态资源生成流程。

### 2.2 /version 与 /reboot

- `version_handler()`（167-182）直接返回 `version_string`，无持久化副作用；未显式校验 HTTP method。
- `reboot_handler()`（184-215）在 HTTP_CB_NEW 分配 session 并返回 rebooting；在 HTTP_CB_CLOSED 释放 session，调用 `tcp_close_all_conn()` 后执行 `do_reset()`。
- /reboot 是明确的复位动作，不是只读查询。PA-220 可以调用自身复位机制，但不得在只读 Web 第一版注册该 handler；如将来提供重启按钮，需确认连接关闭时序、用户确认和误触防护。
- `misc.c` include ipq_api.h，但本文件实际函数调用路径未发现直接调用 IPQ 专用 API。主要依赖是 HTTPD/TCP、嵌入式文件系统、CONFIG_GZIP、gunzip、U-Boot reset 命令接口。

### 2.3 安全/健壮性
- `upload_html_handler()` 使用静态 512 字节响应缓冲和 32 字节 page_name；当前只对固定注册 URI 调用，cpy_len 来自 URI 长度。重写时应保留白名单路由，不能让任意用户 URI 进入该生成函数。
- gzip 解压分配大小取自 fs_desc->uncompressed_size；资源是编译期内置文件，但仍应检查长度、解压返回值和内存分配失败。
- /reboot 不检查 method；应在 PA-220 重写时限制为 POST 并增加 CSRF/确认等 Web 侧保护（若服务无认证，仍不应提供复位动作）。

### 2.4 PA-220 适配结论
- 页面分发、版本字符串输出、gzip 静态资源服务：通用业务逻辑可重写。
- HTTPD/TCP 回调、response->gunzip_buffer 生命周期：需按 PA-220 新建 HTTP 服务 API 适配。
- reset：PA-220 有 U-Boot reset 能力，但 Web 初版不纳入。
- 本模块不涉及 SPI/eMMC/CPLD/EEPROM 写入；/reboot 会触发系统复位。源码存在；PA-220 编译/实机 NOT VERIFIED。

## 3. webterm.c

### 3.1 入口、注册和构建依赖

文件：u-boot-2016/failsafe/modules/webterm.c。

- `webterm_register_uri_handlers()`（333-338）注册：
  - POST `/webterm/exec` -> `webterm_exec_handler()`
  - POST `/webterm/upload` -> `webterm_upload_handler()`
  - GET `/webterm/cmdlist` -> `webterm_cmdlist_handler()`
- 前端 main.js：约 4786 行 GET cmdlist；约 5367 行 POST exec；约 5476 行 POST upload。
- 模块依赖 HTTPD、ipq_api.h、MMC/SDHCI host、partition、MD5、failsafe/fw_dec.h、capture.h。
- 通过 CONFIG_SDHCI_SUPPORT 在 qca_mmc 与 sdhci_host 之间选择全局 mmc_host。
- `failsafe/modules/Makefile` 无条件编入 webterm.o（受父级 CONFIG_HTTPD 和非 SPL 条件约束）。

### 3.2 /webterm/exec：远程执行 U-Boot 命令

- NEW 状态要求 POST，读取表单字段 cmd；缺失则 400。
- 每个会话分配 `webterm_exec_session` 和 16,666 字节输出缓冲。
- RESPONDING 状态调用 `call_func_capture(webterm_run_command, ...)`；`webterm_run_command()`（84-87）直接执行 `run_command(cmd, 0)`。
- 命令字符串被复制到全局静态 `command.data[257]`，最大有效长度受到 strlcpy 截断；没有对输入长度超限作拒绝处理。
- repeat 逻辑使用全局 `command.repeatable` 和 hush 扩展函数 `webterm_init_repeat_flag()`、`webterm_repeat_last_command()`、`is_last_command_repeatable()`。
- CLOSED 状态释放输出缓冲和 session。
- `webterm_exec_resp_custom`、`command.data`、`command.repeatable` 是模块级全局状态；在多连接/多会话下存在状态互相影响的可能性，需要结合 HTTPD 调度模型验证，标记 NOT VERIFIED。

**安全结论：** 这是远程任意 U-Boot 命令执行入口，不是受限诊断 API。PA-220 已配置环境保存、SPI Flash 命令、MMC/文件系统写入及 OCTEON bootloader 更新等命令；若 Web terminal 暴露 run_command，调用者可能触发 saveenv、sf、mmc、文件系统写入、复位等操作。即使 HTTP 服务只在 MGT 口监听，也不能将其当作安全边界。PA-220 Web 初版必须完全不注册该 handler。

### 3.3 /webterm/upload：上传文件信息处理

- NEW 状态读取 multipart 字段 file；缺失则 400。
- 调用 `set_file_info_env((ulong)file->data, (ulong)file->size)` 设置上传文件信息环境。
- 分配 999 字节输出缓冲，通过 `call_func_capture(print_file_info, ...)` 捕获信息。
- `print_file_info()` 调用 `check_fw_type()`、`fw_type_to_string()`、`md5()`，打印文件名、类型、地址、长度和 MD5。
- CLOSED 释放 session 数据。
- 依赖 IPQ/QSDK 固件识别头、捕获输出 API 和环境变量辅助函数；本次未追完 set_file_info_env 的具体实现及其是否会调用 saveenv，标记 UNKNOWN，不推定为纯 RAM 状态修改。
- 即使该 endpoint 看起来只是文件信息展示，也会改写环境状态并暴露上传缓冲地址；PA-220 不应直接移植，需使用独立 RAM 上传对象和 PA-220 自己的只读镜像识别逻辑。

### 3.4 /webterm/cmdlist：命令枚举

- GET 请求遍历 `ll_entry_start(cmd_tbl_t, cmd)` / `ll_entry_count(cmd_tbl_t, cmd)`，输出命令名称和 usage JSON。
- 对 usage 调用 json_escape；分配固定 6,666 字节缓冲。
- len 由多次 snprintf 返回值累加，但没有检查剩余缓冲区；命令数量或 usage 文本较大时可能越界。
- 命令列表本身可暴露目标固件启用的命令面，不能视为只读诊断功能的必要部分。

### 3.5 PA-220 对照与适配结论

- PA-220 U-Boot 有命令框架和 run_command 类通用能力，但本项目目标不应将其包装成 HTTP 远程 shell。
- uBootKit 专用 `call_func_capture()`、`capture.h`、hush repeat 标志扩展和 QCA MMC host 依赖尚未在 PA-220 固定树确认等价实现；不能直接移植。
- PA-220 已启用 CONFIG_CMD_SAVEENV、CONFIG_CMD_SF、CONFIG_CMD_MMC、EXT4/FAT 写入和 OCTEON bootloader update 相关命令（具体见 octeon_cmd_conf.h / octeon_pa220.h）；这会扩大远程命令执行的持久化破坏面。
- PA-220 eMMC/SPI 环境和 SPI NOR 写入受项目 RAM-only/持久化保护规则约束；webterm 不允许成为绕过这些约束的入口。
- 结论：/webterm/exec 与 /webterm/cmdlist 不适合 PA-220 Web 初版；/webterm/upload 也不直接移植。若未来需要诊断功能，应设计明确白名单的只读 RPC，不执行任意命令。

## 4. 本组结论矩阵

| 模块 | 源码/构建状态 | PA-220 对照 | 适配结论 |
|---|---|---|---|
| misc 静态文件分发 | 源码存在；CONFIG_HTTPD 路径编入 | 无 HTTPD；有 gzip/通用 U-Boot 基础需逐项确认 | 页面分发可重写，HTTP API 生命周期需适配 |
| /version | 源码存在 | version_string/版本信息可替代 | 可重写为只读 |
| /reboot | 源码存在 | 有 U-Boot reset 能力 | 有副作用，Web 初版不提供 |
| webterm exec | 源码存在 | run_command 与危险命令可用 | 不可直接移植；禁止任意远程命令执行 |
| webterm cmdlist | 源码存在 | 命令表存在 | 不必要且暴露攻击面，初版不提供 |
| webterm upload | 源码存在 | IPQ firmware helper/QCA host 不等价 | 重写独立 RAM 文件识别，不移植原 handler |

本组未执行编译、设备访问、SPI/eMMC 写入或实机测试。
