#!/usr/bin/env python3
"""Generate the PAN-OS 3.10.87 bootinfo ABI compatibility patch with valid hunks."""
import argparse
import difflib
import subprocess
from pathlib import Path

REL = "u-boot/arch/mips/cpu/octeon/octeon_boot.c"
HELPER = r"""
#ifdef __U_BOOT__
/*
 * PAN-OS 3.10.87 uses a PA-specific 24-byte bootinfo extension between
 * config_flags and fdt_addr. Generic SDK 5.1.0 offsets are fdt=152/ext=160;
 * the PAN kernel reads profile/cpuid/uart/name at 152..175, fdt=176, ext=184.
 */
#define PA220_PAN_BOOTINFO_EXTENSION_SIZE 24
#define PA220_PAN_BOOTINFO_FDT_OFFSET 176
#define PA220_PAN_BOOTINFO_EXT_CORE_MASK_OFFSET 184
#define PA220_PAN_BOOTINFO_SIZE \
	(sizeof(cvmx_bootinfo_t) + PA220_PAN_BOOTINFO_EXTENSION_SIZE)

struct pa220_pan_bootinfo_extension {
	uint8_t board_profile;
	uint8_t cpuid;
	uint8_t console_uart_num;
	char board_name[20];
	uint8_t reserved;
};

typedef char pa220_bi_fdt_offset_check[
	(offsetof(cvmx_bootinfo_t, fdt_addr) == 152) ? 1 : -1];
typedef char pa220_bi_ext_offset_check[
	(offsetof(cvmx_bootinfo_t, ext_core_mask) == 160) ? 1 : -1];
typedef char pa220_bi_size_check[
	(sizeof(cvmx_bootinfo_t) == 288) ? 1 : -1];
typedef char pa220_bi_coremask_size_check[
	(sizeof(cvmx_coremask_t) == 128) ? 1 : -1];
typedef char pa220_bi_extension_size_check[
	(sizeof(struct pa220_pan_bootinfo_extension) == 24) ? 1 : -1];

/*
 * Serialize only the kernel-visible copy in the PAN SDK 3.1.2 layout.
 * Profile 0 preserves the currently observed generic PA-220 path; cpuid is
 * the physical OCTEON core number; UART0 matches the PA-220 console.
 */
static void pa220_prepare_pan_kernel_bootinfo(void *dest,
					      const cvmx_bootinfo_t *src,
					      int core)
{
	uint8_t *bytes = (uint8_t *)dest;
	struct pa220_pan_bootinfo_extension extension;
	cvmx_coremask_t clean_core_mask;

	memset(bytes, 0, PA220_PAN_BOOTINFO_SIZE);
	memcpy(bytes, src, offsetof(cvmx_bootinfo_t, fdt_addr));

	memset(&extension, 0, sizeof(extension));
	extension.board_profile = 0;
	extension.cpuid = (uint8_t)core;
	extension.console_uart_num = 0;
	memcpy(extension.board_name, "PA-220", sizeof("PA-220"));
	memcpy(bytes + offsetof(cvmx_bootinfo_t, fdt_addr),
	       &extension, sizeof(extension));
	memcpy(bytes + PA220_PAN_BOOTINFO_FDT_OFFSET,
	       &src->fdt_addr, sizeof(src->fdt_addr));

	cvmx_coremask_clear_all(&clean_core_mask);
	cvmx_coremask_set64(&clean_core_mask, (uint64_t)src->core_mask);
	memcpy(bytes + PA220_PAN_BOOTINFO_EXT_CORE_MASK_OFFSET,
	       &clean_core_mask, sizeof(clean_core_mask));

	if (core == 0) {
		printf("PA220-PAN-ABI: size=%u profile=%u cpuid=%u uart=%u "
		       "fdt=0x%llx legacy_mask=0x%x ext_mask=0x%x "
		       "offsets=fdt:%u/ext:%u\n",
		       (unsigned)PA220_PAN_BOOTINFO_SIZE,
		       (unsigned)extension.board_profile,
		       (unsigned)extension.cpuid,
		       (unsigned)extension.console_uart_num,
		       (unsigned long long)src->fdt_addr,
		       (unsigned)src->core_mask,
		       (unsigned)cvmx_coremask_get32(&clean_core_mask),
		       (unsigned)PA220_PAN_BOOTINFO_FDT_OFFSET,
		       (unsigned)PA220_PAN_BOOTINFO_EXT_CORE_MASK_OFFSET);
	}
}
#endif /* __U_BOOT__ */
"""

def pristine_source(root: Path) -> str:
    return subprocess.check_output(
        ["git", "-C", str(root), "show", "HEAD:" + REL], text=True
    )

def replace_once(source: str, old: str, new: str, label: str) -> str:
    count = source.count(old)
    if count != 1:
        raise RuntimeError(f"{label}: expected one anchor, found {count}; source drift")
    return source.replace(old, new, 1)

def build_modified(source: str) -> str:
    if "pa220_prepare_pan_kernel_bootinfo" in source:
        raise RuntimeError("compatibility helper already exists in pristine source")
    source = replace_once(
        source, "# include <common.h>\n",
        "# include <common.h>\n# include <stddef.h>\n", "stddef include"
    )
    source = replace_once(
        source, "#define MAX_CORE_TADS\t\t8\n",
        "#define MAX_CORE_TADS\t\t8\n" + HELPER + "\n",
        "compatibility helper insertion"
    )
    source = replace_once(
        source,
        "void *new_fdt;\n#endif\n\tint hplug = 0;",
        "void *new_fdt;\n\tuint32_t bootinfo_size = (image_flags & OCTEON_BOOT_DESC_IMAGE_LINUX) ?\n\t\tPA220_PAN_BOOTINFO_SIZE : sizeof(cvmx_bootinfo_t);\n#endif\n\tint hplug = 0;",
        "Linux bootinfo size selection"
    )
    source = replace_once(
        source,
        "addr = cvmx_bootmem_phy_alloc(sizeof(cvmx_bootinfo_t),\n",
        "addr = cvmx_bootmem_phy_alloc(bootinfo_size,\n",
        "Linux bootinfo allocation"
    )
    source = replace_once(
        source,
        "boot_desc[core].cvmx_desc_vaddr =\n\t\t\t    (boot_info_block_array[core].boot_desc_addr -\n\t\t\t     sizeof(cvmx_bootinfo_t)) & (~0x7ull);",
        "boot_desc[core].cvmx_desc_vaddr =\n\t\t\t    (boot_info_block_array[core].boot_desc_addr -\n\t\t\t     bootinfo_size) & (~0x7ull);",
        "Linux bootinfo heap reservation"
    )
    copy = (
        "\t\t\tmemcpy((void *)(uint32_t) boot_desc[core].cvmx_desc_vaddr,\n"
        "\t\t\t       (void *)&cvmx_bootinfo_array[core],\n"
        "\t\t\t       sizeof(cvmx_bootinfo_t));\n"
    )
    compat_copy = copy + (
        "\t\t\tpa220_prepare_pan_kernel_bootinfo(\n"
        "\t\t\t\t(void *)(uint32_t)boot_desc[core].cvmx_desc_vaddr,\n"
        "\t\t\t\t&cvmx_bootinfo_array[core], core);\n"
    )
    source = replace_once(source, copy, compat_copy, "Linux bootinfo copy")
    return source

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--source-root", required=True)
    ap.add_argument("--output", required=True)
    args = ap.parse_args()
    original = pristine_source(Path(args.source_root))
    modified = build_modified(original)
    patch = "".join(difflib.unified_diff(
        original.splitlines(True), modified.splitlines(True),
        fromfile="a/" + REL, tofile="b/" + REL, n=5
    ))
    if not patch:
        raise SystemExit("generated compatibility patch is empty")
    output = Path(args.output)
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(patch)
    print("Generated", output, "bytes=", len(patch))
    print("PAN_KERNEL_BOOTINFO_LAYOUT=extension@152,fdt@176,ext_core_mask@184,size=312")
    print("COREMASK_SOURCE=validated_legacy_core_mask")
    print("DIFF_ENGINE=python-difflib-unified-diff")

if __name__ == "__main__":
    main()
