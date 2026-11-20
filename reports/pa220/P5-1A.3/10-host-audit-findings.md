# P5-1A.3 Host Audit Findings

Audit input: `P5 1A 3 HOST AUDIT 2026.txt` (blob `f0d24f224fc703fea53c700c67e19cca49c50878`).

## Source identity

At audit time, Debian fetched branch HEAD `1f9f527ef6f3436ddd04ab48d45f7dd530fe545d`. The exported TCP source hashes were:

- `u-boot/net/tcp.c`: SHA1 `2be12d49f0e5d80e6980b649d6148b90a474d287`
- `u-boot/include/net/tcp.h`: SHA1 `db4752ce1c9589c9579069e40476279c0eabda75`

These match the previously recorded P5-1A.2 validation inputs. The audit-log upload subsequently advanced the branch to `24f8b080c94a23b5da547bbc987dac05ac8dc22e`; that commit is a direct child of the audited HEAD. TCP source history still points to `5c33f6c4496cdc08a1cab66facf2c03b3bcc6754`.

## Target object evidence

Toolchain: Cavium OCTEON SDK Build 52, GCC 4.7.0, target `mips64-octeon-linux-gnu`.

The existing object `/tmp/pa220-p5-validation/obj/tcp.o` is ELF32, big-endian, MIPS N32, OCTEON, MIPS64R2, soft-float, relocatable. Size: text 5336, data 0, bss 0. Relocations show internal TCP calls and ordinary libc-like memory helpers `memcpy`, `memset`, and `memmove`. This is consistent with an independently relocatable object; the presence of undefined memory helper symbols is not itself a failure.

The object was not rebuilt during this audit. Its source identity is supported by matching exported source SHA1 values and the prior compile record; a fresh compile with recorded exact arguments remains a follow-up evidence item.

## Macro redefinition findings

Observed source locations:

| Macro | Earlier definition | Later definition |
|---|---|---|
| `CONFIG_OCTEON_CN70XX` | `include/config.h:4` = `1` | `include/configs/octeon_pa220.h:15` = empty |
| `CONFIG_CMD_EXPORTENV` | `include/config_cmd_defaults.h:14` = `1` | `include/configs/octeon_pa220.h:164` = empty |
| `CONFIG_CMD_IMPORTENV` | `include/config_cmd_defaults.h:16` = `1` | `include/configs/octeon_pa220.h:165` = empty |
| `CONFIG_SYS_MEMTEST_START` | `include/configs/octeon_common.h:283` = `(CONFIG_SYS_SDRAM_BASE + 0x100000)` | `include/configs/octeon_pa220.h:196` = `0x22000000` |
| `CONFIG_SYS_MEMTEST_END` | `include/configs/octeon_common.h:284` = `(CONFIG_SYS_SDRAM_BASE + 0xffffff)` | `include/configs/octeon_pa220.h:197` = `0x2a000000` |

The first three pairs have differing replacement token sequences (numeric `1` versus empty replacement), which explains GCC redefinition diagnostics even where both forms may act as enabled feature macros in conditional compilation. The two memory-test pairs have genuinely different expressions; their evaluated values and intended PA220 memory-layout semantics have not yet been independently established.

The log also shows `CONFIG_OCTEON_CN70XX` diagnosed a second time through another include path. Exact command-line flags, include search order, forced includes, macro dump, and full include trace were not captured. Therefore the warning-origin audit is partial, not closed.

No configuration header was modified. No warning was suppressed.

## Status and next work

- Confirmed: target compiler identity, source SHA1 identity, successful prior preprocessing/compile return codes, ELF ABI/architecture attributes, object section sizes, and observed relocation dependencies.
- Partial: symbol/dependency review and macro warning provenance.
- Not yet done: exact compiler argument capture, macro expansion/value comparison, complete include trace, independent Host TCP protocol harness and execution.
- No TCP source change or speculative bug fix was made.
