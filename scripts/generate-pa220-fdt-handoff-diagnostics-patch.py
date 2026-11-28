#!/usr/bin/env python3
"""Generate PA-220 bootinfo/FDT handoff diagnostics with valid unified-diff hunks."""
import argparse
import difflib
from pathlib import Path

BOOT_REL = "u-boot/arch/mips/cpu/octeon/octeon_boot.c"
LINUX_REL = "u-boot/arch/mips/cpu/octeon/commands/cmd_octeon_linux.c"

BOOT_ANCHOR = """\t\t\tmemcpy((void *)(uint32_t) boot_desc[core].cvmx_desc_vaddr,
\t\t\t       (void *)&cvmx_bootinfo_array[core],
\t\t\t       sizeof(cvmx_bootinfo_t));
"""
BOOT_DIAGNOSTIC = """\t\t\tif (core == 0) {
\t\t\t\tcvmx_bootinfo_t *pa220_bi_copy =
\t\t\t\t\t(cvmx_bootinfo_t *)cvmx_phys_to_ptr(
\t\t\t\t\t\tboot_desc[core].cvmx_desc_vaddr);
\t\t\t\tvoid *pa220_fdt_copy = pa220_bi_copy->fdt_addr ?
\t\t\t\t\tcvmx_phys_to_ptr(pa220_bi_copy->fdt_addr) : NULL;

\t\t\t\tprintf("PA220-HANDOFF: build ABI=%u.%u offsets major=%u minor=%u fdt=%u size=%u\\n",
\t\t\t\t       (unsigned)CVMX_BOOTINFO_MAJ_VER,
\t\t\t\t       (unsigned)CVMX_BOOTINFO_MIN_VER,
\t\t\t\t       (unsigned)offsetof(cvmx_bootinfo_t, major_version),
\t\t\t\t       (unsigned)offsetof(cvmx_bootinfo_t, minor_version),
\t\t\t\t       (unsigned)offsetof(cvmx_bootinfo_t, fdt_addr),
\t\t\t\t       (unsigned)sizeof(cvmx_bootinfo_t));
\t\t\t\tprintf("PA220-HANDOFF: array=%u.%u fdt=0x%llx copy=%u.%u fdt=0x%llx desc_fdt=0x%llx\\n",
\t\t\t\t       (unsigned)cvmx_bootinfo_array[core].major_version,
\t\t\t\t       (unsigned)cvmx_bootinfo_array[core].minor_version,
\t\t\t\t       (unsigned long long)cvmx_bootinfo_array[core].fdt_addr,
\t\t\t\t       (unsigned)pa220_bi_copy->major_version,
\t\t\t\t       (unsigned)pa220_bi_copy->minor_version,
\t\t\t\t       (unsigned long long)pa220_bi_copy->fdt_addr,
\t\t\t\t       (unsigned long long)boot_desc[core].cvmx_desc_vaddr);
\t\t\t\tprintf("PA220-HANDOFF: src=%p new=%p new_phys=0x%llx size=%u copied=%p check=%d total=%u\\n",
\t\t\t\t       gd->fdt_blob, new_fdt,
\t\t\t\t       (unsigned long long)new_fdt_addr, (unsigned)fdt_size,
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
\t\tcvmx_bootinfo_t *pa220_bi =
\t\t\t(cvmx_bootinfo_t *)cvmx_phys_to_ptr(
\t\t\t\tcur_boot_desc->cvmx_desc_vaddr);

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
\t\tprintf("PA220-START: bootinfo=%p version=%u.%u core_mask=0x%x fdt=0x%llx phy_mem=0x%llx\\n",
\t\t       pa220_bi, (unsigned)pa220_bi->major_version,
\t\t       (unsigned)pa220_bi->minor_version,
\t\t       (unsigned)pa220_bi->core_mask,
\t\t       (unsigned long long)pa220_bi->fdt_addr,
\t\t       (unsigned long long)pa220_bi->phy_mem_desc_addr);
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
                      "PA220-HANDOFF: build ABI=")
    patch += patch_one(root, LINUX_REL, LINUX_ANCHOR, LINUX_DIAGNOSTIC,
                       "PA220-START: a3=")
    if not patch:
        raise SystemExit("generated diagnostic patch is empty")
    output = Path(args.output)
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(patch)
    print("Generated", output, "bytes=", len(patch))
    print("DIAGNOSTIC_SITES=bootinfo-copy,start-linux-a3")
    print("DIFF_ENGINE=python-difflib-unified-diff")

if __name__ == "__main__":
    main()
