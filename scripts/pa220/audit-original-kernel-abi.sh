#!/usr/bin/env bash
set -euo pipefail
KERNEL="artifacts/pa220/original-kernel/pa220-original-vmlinux"
OUT="reports/pa220/P7-4/ORIGINAL-KERNEL-ABI-DISASSEMBLY.md"
EXPECTED_SHA="eff6e9d16cf11144642404ddd010bb9d1f62616df185460491a9cd1fd7639e6d"
OBJDUMP="${MIPS_OBJDUMP:-objdump}"
NM="${MIPS_NM:-nm}"
test -s "$KERNEL"
command -v "$OBJDUMP" >/dev/null
command -v "$NM" >/dev/null
mkdir -p "$(dirname "$OUT")"
ACTUAL_SHA="$(sha256sum "$KERNEL" | awk '{print $1}')"
test "$ACTUAL_SHA" = "$EXPECTED_SHA"
SIZE="$(stat -c %s "$KERNEL")"
test "$SIZE" -eq 10387920
{
  echo "# PA-220 original PAN-OS kernel: boot ABI disassembly"
  echo
  echo "Read-only disassembly of the exact user-exported kernel. No device interaction or persistent writes."
  echo
  echo "- File: $KERNEL"
  echo "- Size: $SIZE bytes"
  echo "- SHA-256: $ACTUAL_SHA"
  echo "- Build ID: b305d1bd17f2baaf2776ace2712707b47a73a3ae"
  echo "- Tool: $($OBJDUMP --version | head -n 1)"
  echo
  echo "## ELF header and program headers"
  echo
  echo '~~~text'
  readelf -hW "$KERNEL"
  readelf -lW "$KERNEL"
  echo '~~~'
  echo
  echo "## Relevant symbols"
  echo
  echo '~~~text'
  "$NM" -n -S --defined-only "$KERNEL" | grep -E ' (prom_init|device_tree_init|fw_init_cmdline|plat_mem_setup|octeon_bootinfo|octeon_boot_desc_ptr|octeon_setup_smp|plat_smp_setup|smp_setup|kernel_entry)$' || true
  echo '~~~'
  echo
  for sym in prom_init device_tree_init fw_init_cmdline plat_mem_setup octeon_setup_smp plat_smp_setup; do
    if "$NM" -n --defined-only "$KERNEL" | awk -v s="$sym" '$3 == s { found=1 } END { exit !found }'; then
      echo "## Disassembly: $sym"
      echo
      echo '~~~asm'
      "$OBJDUMP" -d --disassemble="$sym" "$KERNEL"
      echo '~~~'
      echo
    fi
  done
  echo "## Relevant embedded strings"
  echo
  echo '~~~text'
  strings -a -t x "$KERNEL" | grep -Ei 'Using (internal|passed|appended) Device Tree|bootinfo|core_mask|fdt_addr|cpuid=0|cpuid=|mem=4096|console=ttyS0|root=' || true
  echo '~~~'
  echo
  echo "## Interpretation boundary"
  echo
  echo "- This report exposes the exact compiled kernel reads of the boot descriptor, bootinfo version/coremask/FDT fields, and SMP setup paths."
  echo "- It does not infer runtime RAM contents from U-Boot's own diagnostic prints."
  echo "- A symbol may be inlined or optimized; absence from the symbol list is not proof that the corresponding logic is absent."
  echo "- The literal string scan is not a substitute for reconstructing the actual command line."
} > "$OUT"
echo "Wrote $OUT"
sha256sum "$OUT"
