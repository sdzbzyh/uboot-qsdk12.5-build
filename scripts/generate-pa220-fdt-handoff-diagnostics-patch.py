#!/usr/bin/env python3
"""Generate the PA-220 bootinfo/FDT runtime-diagnostic patch with valid hunks."""
import argparse
import difflib
from pathlib import Path

REL = "u-boot/arch/mips/cpu/octeon/octeon_boot.c"
ANCHOR = """\t\t\tmemcpy((void *)(uint32_t) boot_desc[core].cvmx_desc_vaddr,
\t\t\t       (void *)&cvmx_bootinfo_array[core],
\t\t\t       sizeof(cvmx_bootinfo_t));
"""
DIAGNOSTIC = """\t\t\tif (core == 0) {
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
MARKER = "PA220-HANDOFF: build ABI="


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--source-root", required=True)
    ap.add_argument("--output", required=True)
    args = ap.parse_args()

    source_path = Path(args.source_root) / REL
    current = source_path.read_text()
    if MARKER in current:
        if current.count(DIAGNOSTIC) != 1:
            raise SystemExit("diagnostic marker exists but exact generated block was not found")
        original = current.replace(DIAGNOSTIC, "", 1)
        modified = current
    else:
        if current.count(ANCHOR) != 1:
            raise SystemExit("expected unique bootinfo memcpy anchor; source drift detected")
        original = current
        modified = current.replace(ANCHOR, ANCHOR + DIAGNOSTIC, 1)

    patch = "".join(difflib.unified_diff(
        original.splitlines(True),
        modified.splitlines(True),
        fromfile="a/" + REL,
        tofile="b/" + REL,
        n=5,
    ))
    if not patch:
        raise SystemExit("generated diagnostic patch is empty")
    output = Path(args.output)
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(patch)
    print("Generated", output, "bytes=", len(patch))


if __name__ == "__main__":
    main()
