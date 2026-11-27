# P6-B：32 MiB+1 超限拒绝与失败清理实机验收

- 日期：2026-11-27
- 设备：PA-220，U-Boot 2013.07，build time `Nov 25 2026 - 11:04:13`
- 网络：`octrgmii0`，PA-220 `192.168.2.2`，TFTP 主机 `192.168.2.10`
- 测试文件：`pa220-p6-get-33554433.bin`，长度 33,554,433 bytes（32 MiB + 1）
- 预期 SHA-256：`b92c884a7f921c6065d1e8a7bf696aa26d6ef0ef61d17effa71b7a2656e828d2`
- 实测 Windows SHA-256 与预期一致；实测长度 33,554,433 bytes。

## 设备结果

串口执行：

```text
pa-220# tftpboot ${loadaddr} pa220-p6-get-33554433.bin
Using octrgmii0 device
TFTP from server 192.168.2.10; our IP address is 192.168.2.2
Filename 'pa220-p6-get-33554433.bin'.
Load address: 0x20000000
Loading: *
TFTP GET failed: invalid or oversized server tsize
pa-220# printenv filesize fileaddr
## Error: "filesize" not defined
## Error: "fileaddr" not defined
pa-220# pa220 memory
loadaddr  : 0x20000000
load base : (not set)
load size : (not set)
__tmp_load: base=0x20000000 size=0x10000000
```

结论：
- 超过 32 MiB 的文件被拒绝。
- 失败后 `filesize` 与 `fileaddr` 均不存在。
- live descriptor 保持不变：`loadaddr=0x20000000`，`load base/load size` 未设置，`__tmp_load` base `0x20000000`、size `0x10000000`。
- 本次未执行 loader/启动命令、`saveenv` 或持久化介质写入。

## 抓包证据

抓包文件：`C:\pa220-p6-b\pa220-p6-b-overlimit.pcapng`

- 文件长度：736 bytes
- SHA-256：`07ba4b966ad60f963ec22579c369caba1f954d4c63cacf0a93bfa8b20468f723`
- Wireshark 解码仅包含两个 TFTP 报文：
  1. Frame 1：PA-220 `192.168.2.2:3779 -> 192.168.2.10:69`，RRQ，文件名正确，选项 `timeout=5`、`tsize=0`、`blksize=1468`。
  2. Frame 2：服务器 `192.168.2.10:54682 -> 192.168.2.2:3779`，OACK，明确返回 `timeout=5`、`tsize=33554433`、`blksize=1468`。
- 捕获中没有 TFTP DATA 报文。

因此本次拒绝阶段已被抓包确定为 **收到 OACK 并校验服务器 tsize 时**，早于 DATA 接收/写入；不是 DATA block reservation guard 的越界实测。本项只证明超限 tsize 的协商阶段拒绝。

## 验收结论

- 32 MiB+1 主机 fixture 长度与 SHA-256：PASS。
- OACK 返回 33,554,433 后拒绝，且没有 DATA：PASS。
- 失败后 `filesize`/`fileaddr` 清理：PASS。
- live temporary descriptor 保持原值：PASS。

**P6-B 仍未整体验收。** 受控中断/超时失败清理仍待实机测试；应使用可控延迟或停止 TFTP 服务的方式，不依赖随机拔网线。
