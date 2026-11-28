#!/usr/bin/env python3
"""Generate PA-220 PAN-kernel-layout bootinfo/FDT handoff diagnostics."""
import argparse
import difflib
from pathlib import Path

BOOT_REL = "u-boot/arch/mips/cpu/octeon/octeon_boot.c"
LINUX_REL = "u-boot/arch/mips/cpu/octeon/commands/cmd_octeon_linux.c"

BOOT_ANCHOR = """\t\t\tmemcpy((void *)(uint32_t) boot_desc[core].cvmx_desc_vaddr,
\t\t\t       (void *)&cvmx_bootinfo_array[core],
\t\t\t       sizeof(cvmx_bootinfo_t));
\t\t\tpa220_prepare_pan_kernel_bootinfo(
\t\t\t\t(void *)(uint32_t)boot_desc[core].cvmx_desc_vaddr,
\t\t\t\t&cvmx_bootinfo_array[core], core);
"""
BOOT_DIAGNOSTIC = """\t\t\tif (core == 0) {
\t\t\t\tuint8_t *pa220_bi_copy = (uint8_t *)cvmx_phys_to_ptr(
\t\t\t\t\tboot_desc[core].cvmx_desc_vaddr);
\t\t\t\tuint64_t pa220_copy_fdt = 0, pa220_ext_low = 0;
\t\t\t\tvoid *pa220_fdt_copy;

\t\t\t\tmemcpy(&pa220_copy_fdt, pa220_bi_copy + 176,
\t\t\t\t       sizeof(pa220_copy_fdt));
\t\t\t\tmemcpy(&pa220_ext_low, pa220_bi_copy + 184,
\t\t\t\t       sizeof(pa220_ext_low));
\t\t\t\tpa220_fdt_copy = pa220_copy_fdt ?
\t\t\t\t\tcvmx_phys_to_ptr(pa220_copy_fdt) : NULL;

\t\t\t\tprintf("PA220-HANDOFF: PAN ABI layout extension=152 fdt=176 ext_core_mask=184 size=312\\n");
\t\t\t\tprintf("PA220-HANDOFF: source=%u.%u legacy_mask=0x%x source_fdt=0x%llx copy=%u.%u profile=%u cpuid=%u uart=%u board=%s copy_fdt=0x%llx ext0=0x%llx desc_fdt=0x%llx\\n",
\t\t\t\t       (unsigned)cvmx_bootinfo_array[core].major_version,
\t\t\t\t       (unsigned)cvmx_bootinfo_array[core].minor_version,
\t\t\t\t       (unsigned)cvmx_bootinfo_array[core].core_mask,
\t\t\t\t       (unsigned long long)cvmx_bootinfo_array[core].fdt_addr,
\t\t\t\t       (unsigned)*(uint32_t *)pa220_bi_copy,
\t\t\t\t       (unsigned)*(uint32_t *)(pa220_bi_copy + 4),
\t\t\t\t       (unsigned)pa220_bi_copy[152],
\t\t\t\t       (unsigned)pa220_bi_copy[153],
\t\t\t\t       (unsigned)pa220_bi_copy[154],
\t\t\t\t       (char *)(pa220_bi_copy + 155),
\t\t\t\t       (unsigned long long)pa220_copy_fdt,
\t\t\t\t       (unsigned long long)pa220_ext_low,
\t\t\t\t       (unsigned long long)boot_desc[core].cvmx_desc_vaddr);
\t\t\t\tprintf("PA220-HANDOFF: copied_fdt=%p check=%d total=%u\\n",
\t\t\t\t       pa220_fdt_copy,
\t\t\t\t       pa220_fdt_copy ? fdt_check_header(pa220_fdt_copy) : -1,
\t\t\t\t       pa220_fdt_copy ? fdt_totalsize(pa220_fdt_copy) : 0);
\t\t\t\tprintf("PA220-HANDOFF: desc=%p version=%u size=%u argc=%u flags=0x%x coremask=0x%x boot_desc_copy=0x%llx\\n",
\t\t\t\t       &boot_desc[core],
\t\t\t\t       (unsigned)boot_desc[core].desc_version,
\t\t\t\t       (unsigned)boot_desc[core].desc_size,
\t\t\t\t       (unsigned)boot_desc[core].argc,
\t\t\t\t       (unsigned)boot_desc[core].flags,
\t\t\t\t       (unsigned)boot_desc[core].core_mask,
\t\t\t\t       (unsigned long long)boot_info_block_array[core].boot_desc_addr);
\t\t\t}
"""
LINUX_ANCHOR = """\targ3 = boot_desc_addr;
"""
LINUX_DIAGNOSTIC = """\tif (core_num == 0) {
\t\tstruct octeon_boot_descriptor *pa220_desc_copy =
\t\t\t(struct octeon_boot_descriptor *)cvmx_phys_to_ptr(
\t\t\t\tboot_info_ptr->boot_desc_addr);
\t\tuint8_t *pa220_bi = (uint8_t *)cvmx_phys_to_ptr(
\t\t\tcur_boot_desc->cvmx_desc_vaddr);
\t\tuint64_t pa220_fdt = 0, pa220_ext_low = 0;

\t\tmemcpy(&pa220_fdt, pa220_bi + 176, sizeof(pa220_fdt));
\t\tmemcpy(&pa220_ext_low, pa220_bi + 184, sizeof(pa220_ext_low));
\t\tprintf("PA220-START: a3=0x%llx global=%p global_phys=0x%llx copy_phys=0x%llx\\n",
\t\t       (unsigned long long)boot_desc_addr, cur_boot_desc,
\t\t       (unsigned long long)uboot_tlb_ptr_to_phys(cur_boot_desc),
\t\t       (unsigned long long)boot_info_ptr->boot_desc_addr);
\t\tprintf("PA220-START: global_desc cvmx=0x%llx argc=%u flags=0x%x coremask=0x%x; copy_desc cvmx=0x%llx argc=%u flags=0x%x coremask=0x%x cmp=%d\\n",
\t\t       (unsigned long long)cur_boot_desc->cvmx_desc_vaddr,
\t\t       (unsigned)cur_boot_desc->argc, (unsigned)cur_boot_desc->flags,
\t\t       (unsigned)cur_boot_desc->core_mask,
\t\t       (unsigned long long)pa220_desc_copy->cvmx_desc_vaddr,
\t\t       (unsigned)pa220_desc_copy->argc, (unsigned)pa220_desc_copy->flags,
\t\t       (unsigned)pa220_desc_copy->core_mask,
\t\t       memcmp(cur_boot_desc, pa220_desc_copy,
\t\t              cur_boot_desc->desc_size));
\t\tprintf("PA220-START: bootinfo ABI=%u.%u legacy_mask=0x%x profile=%u cpuid=%u uart=%u board=%s fdt=0x%llx ext0=0x%llx phy_mem=0x%llx\\n",
\t\t       (unsigned)*(uint32_t *)pa220_bi,
\t\t       (unsigned)*(uint32_t *)(pa220_bi + 4),
\t\t       (unsigned)*(uint32_t *)(pa220_bi + 52),
\t\t       (unsigned)pa220_bi[152], (unsigned)pa220_bi[153],
\t\t       (unsigned)pa220_bi[154], (char *)(pa220_bi + 155),
\t\t       (unsigned long long)pa220_fdt,
\t\t       (unsigned long long)pa220_ext_low,
\t\t       (unsigned long long)*(uint64_t *)(pa220_bi + 60));
\t}
"""

def patch_one(source_root: Path, rel: str, anchor: str, diagnostic: str,
              marker: str) -> str:
    path = source_root / rel
    current = path.read_text()
    if marker in current:
        if current.count(diagnostic) != 1:
            raise RuntimeError(f"{rel}: diagnostic marker exists but exact block is missing")
        original = current.replace(diagnostic, "", 1)
        modified = current
    else:
        if current.count(anchor) != 1:
            raise RuntimeError(f"{rel}: expected unique anchor; source drift detected")
        original = current
        modified = current.replace(anchor, anchor + diagnostic, 1)
    return "".join(difflib.unified_diff(
        original.splitlines(True), modified.splitlines(True),
        fromfile="a/" + rel, tofile="b/" + rel, n=5))

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--source-root", required=True)
    ap.add_argument("--output", required=True)
    args = ap.parse_args()
    root = Path(args.source_root)
    patch = patch_one(root, BOOT_REL, BOOT_ANCHOR, BOOT_DIAGNOSTIC,
                      "PA220-HANDOFF: PAN ABI layout")
    patch += patch_one(root, LINUX_REL, LINUX_ANCHOR, LINUX_DIAGNOSTIC,
                       "PA220-START: bootinfo ABI=")
    if not patch:
        raise SystemExit("generated diagnostic patch is empty")
    output = Path(args.output)
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(patch)
    print("Generated", output, "bytes=", len(patch))
    print("DIAGNOSTIC_LAYOUT=extension@152,fdt@176,ext_core_mask@184,size=312")
    print("DIFF_ENGINE=python-difflib-unified-diff")

if __name__ == "__main__":
    main()
