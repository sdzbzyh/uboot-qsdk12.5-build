#!/usr/bin/env python3
import argparse, hashlib, os, pathlib, subprocess, sys, tempfile
PIN = "801717ba6a8a59ed2cfd53bf3e1b19841fecd2e8"
BOARD = pathlib.Path("u-boot/board/octeon/pa220/pa220_board.c")
MAKEFILE = pathlib.Path("u-boot/board/octeon/pa220/Makefile")
def git(root, *args, env=None):
    return subprocess.run(["git", "-C", str(root), *args], check=True,
        stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True, env=env).stdout
def once(src, old, new, label):
    n=src.count(old)
    if n != 1: raise RuntimeError(label + " anchor count=" + str(n))
    return src.replace(old,new,1)
def main():
    ap=argparse.ArgumentParser()
    ap.add_argument("--source-root",required=True)
    ap.add_argument("--output",required=True)
    a=ap.parse_args()
    root=pathlib.Path(a.source_root).resolve(); out=pathlib.Path(a.output)
    if git(root,"rev-parse","HEAD").strip()!=PIN: raise RuntimeError("wrong fixed PA-220 source pin")
    board=root/BOARD; mk=root/MAKEFILE; bs=board.read_text(); ms=mk.read_text()
    if "pa220_install_dry_run_cmd" in bs and "pa220_install_plan.o" in ms:
        if not out.is_file() or "pa220_install_dry_run_cmd" not in out.read_text():
            raise RuntimeError("P7-3 patch missing after source modification")
        print("P7_3_ALREADY_GENERATED=verified")
        print("PATCH_SHA256="+hashlib.sha256(out.read_bytes()).hexdigest())
        return 0
    bs=once(bs,"#include <command.h>","#include <command.h>\n#include <mmc.h>\n#include <part.h>\n#include <environment.h>\n#include <asm/arch/octeon_load_range.h>\n#include <pa220_partition.h>\n#include <pa220_install_plan.h>","partition includes")
    helper=r'''
struct pa220_mmc_reader {
	block_dev_desc_t *desc;
};

static int pa220_mmc_read_sector(void *ctx, uint64_t lba, uint8_t sector[512])
{
	struct pa220_mmc_reader *reader = ctx;
	block_dev_desc_t *desc = reader->desc;

	if (!desc || desc->blksz != 512 || !desc->block_read ||
	    lba >= (uint64_t)desc->lba)
		return -1;
	return desc->block_read(desc->dev, (lbaint_t)lba, 1, sector) == 1 ? 0 : -1;
}

static const char *pa220_pt_status_name(enum pa220_pt_status status)
{
	switch (status) {
	case PA220_PT_OK: return "OK";
	case PA220_PT_UNKNOWN: return "UNKNOWN";
	case PA220_PT_AMBIGUOUS: return "AMBIGUOUS";
	case PA220_PT_CORRUPT: return "CORRUPT";
	case PA220_PT_IO: return "IO_ERROR";
	default: return "INVALID";
	}
}

static int pa220_partition_scan_cmd(int dev)
{
	struct mmc *mmc;
	block_dev_desc_t *desc;
	struct pa220_mmc_reader reader;
	struct pa220_partition_table table;
	unsigned int i;

	mmc = find_mmc_device(dev);
	if (!mmc) {
		printf("pa220 partition: no MMC device %d\n", dev);
		return CMD_RET_FAILURE;
	}
	if (mmc_init(mmc)) {
		printf("pa220 partition: MMC init failed for device %d\n", dev);
		return CMD_RET_FAILURE;
	}
	desc = mmc_get_dev(dev);
	if (!desc || desc->type == DEV_TYPE_UNKNOWN || desc->blksz != 512) {
		puts("pa220 partition: unavailable 512-byte block device\n");
		return CMD_RET_FAILURE;
	}
	reader.desc = desc;
	if (pa220_partition_scan(pa220_mmc_read_sector, &reader,
				 (uint64_t)desc->lba, &table)) {
		printf("partition table: %s\n", pa220_pt_status_name(table.status));
		return CMD_RET_FAILURE;
	}
	printf("partition table: %s, capacity=%llu sectors, count=%u\n",
	       table.format == PA220_PT_DOS_MBR ? "DOS/MBR" : "GPT",
	       (unsigned long long)table.capacity, table.count);
	for (i = 0; i < table.count; i++) {
		const struct pa220_partition *p = &table.parts[i];
		printf("  #%u %-7s start=%llu sectors=%llu",
		       p->index,
		       p->kind == PA220_PART_PRIMARY ? "PRIMARY" :
		       p->kind == PA220_PART_LOGICAL ? "LOGICAL" : "GPT",
		       (unsigned long long)p->start_lba,
		       (unsigned long long)p->sectors);
		if (p->kind != PA220_PART_GPT)
			printf(" type=0x%02x\n", p->type);
		else
			printf(" attributes=0x%llx\n",
			       (unsigned long long)p->attributes);
	}
	puts("partition inventory: READ_ONLY; target mapping: UNMAPPED\n");
	return CMD_RET_SUCCESS;
}

static int pa220_install_dry_run_cmd(int dev)
{
	ulong addr = getenv_hex("fileaddr", 0);
	ulong size = getenv_hex("filesize", 0);
	struct pa220_image_info image;
	struct pa220_mmc_reader reader;
	struct pa220_partition_table table;
	struct pa220_install_plan plan;
	struct mmc *mmc;
	block_dev_desc_t *desc;
	enum pa220_plan_status status;

	if (!addr || !size || size > (~0UL - addr) ||
	    octeon_load_range_check((uint64_t)addr, (uint64_t)size)) {
		puts("install dry-run: rejected; no valid completed RAM image\n");
		return CMD_RET_FAILURE;
	}
	flush_dcache_range(addr, addr + size);
	if (pa220_image_parse((const uint8_t *)(uintptr_t)addr,
			      (uint64_t)size, &image)) {
		puts("install dry-run: rejected; malformed or unsupported image\n");
		return CMD_RET_FAILURE;
	}
	mmc = find_mmc_device(dev);
	if (!mmc || mmc_init(mmc)) {
		printf("install dry-run: MMC device %d unavailable\n", dev);
		return CMD_RET_FAILURE;
	}
	desc = mmc_get_dev(dev);
	if (!desc || desc->type == DEV_TYPE_UNKNOWN || desc->blksz != 512) {
		puts("install dry-run: unavailable 512-byte block device\n");
		return CMD_RET_FAILURE;
	}
	reader.desc = desc;
	if (pa220_partition_scan(pa220_mmc_read_sector, &reader,
				 (uint64_t)desc->lba, &table)) {
		printf("install dry-run: partition inventory %s\n",
		       pa220_pt_status_name(table.status));
		return CMD_RET_FAILURE;
	}
	printf("image: %s size=%llu bytes\n",
	       pa220_image_format_name(image.format),
	       (unsigned long long)image.total_size);
	printf("partition inventory: %s count=%u; READ_ONLY\n",
	       table.format == PA220_PT_DOS_MBR ? "DOS/MBR" : "GPT",
	       table.count);
	status = pa220_install_plan_build(NULL, &table, image.total_size,
					  NULL, 0, NULL, 0, &plan);
	printf("install dry-run: REJECTED reason=%s; target mapping=UNMAPPED; actions=NONE\n",
	       pa220_plan_status_name(status));
	return CMD_RET_FAILURE;
}
'''
    fn_start=bs.index("static int do_pa220(")
    bs=bs[:fn_start]+helper+"\n"+bs[fn_start:]
    cmd_start=bs.index("U_BOOT_CMD(pa220,",fn_start)
    body=bs[fn_start:cmd_start]
    dispatch='''\tif ((argc == 3 || argc == 4) && !strcmp(argv[1], "partition") &&
\t    !strcmp(argv[2], "scan")) {
\t\tint dev = argc == 4 ? (int)simple_strtoul(argv[3], NULL, 10) : 0;
\t\tif (dev < 0 || dev > 15)
\t\t\treturn CMD_RET_USAGE;
\t\treturn pa220_partition_scan_cmd(dev);
\t}

\tif ((argc == 3 || argc == 4) && !strcmp(argv[1], "install") &&
	    !strcmp(argv[2], "dry-run")) {
		int dev = argc == 4 ? (int)simple_strtoul(argv[3], NULL, 10) : 0;
		if (dev < 0 || dev > 15)
			return CMD_RET_USAGE;
		return pa220_install_dry_run_cmd(dev);
	}

	return CMD_RET_USAGE;'''
    body=once(body,"\treturn CMD_RET_USAGE;",dispatch,"partition command dispatch")
    bs=bs[:fn_start]+body+bs[cmd_start:]
    bs=once(bs,"U_BOOT_CMD(pa220, 3, 1, do_pa220,","U_BOOT_CMD(pa220, 4, 1, do_pa220,","pa220 maxargs")
    ms=once(ms,"COBJS\t= $(BOARD)_board.o pa220_image.o",
            "COBJS\t= $(BOARD)_board.o pa220_image.o pa220_partition.o pa220_install_plan.o",
            "partition object integration")
    with tempfile.TemporaryDirectory() as td:
        env=os.environ.copy(); env["GIT_INDEX_FILE"]=str(pathlib.Path(td)/"index")
        git(root,"read-tree","HEAD",env=env)
        git(root,"add","--",str(BOARD),str(MAKEFILE),env=env)
        board.write_text(bs); mk.write_text(ms)
        patch=git(root,"diff","--no-ext-diff","--no-renames","--binary","--",str(BOARD),str(MAKEFILE),env=env)
        if not patch or "pa220_partition_scan_cmd" not in patch or "pa220_partition.o" not in patch:
            raise RuntimeError("generated P7-3 patch is incomplete")
        out.parent.mkdir(parents=True,exist_ok=True); out.write_text(patch)
    print("PA220_SOURCE_PIN="+PIN)
    print("P7_READ_ONLY_PARTITION_SCAN=integrated")
    print("P7_PATCH="+str(out))
    print("P7_PATCH_SHA256="+hashlib.sha256(out.read_bytes()).hexdigest())
    print("DIFF_GENERATED_BY=git diff --binary")
    return 0
if __name__=="__main__":
    try: sys.exit(main())
    except Exception as e: print("P7-3 patch generation failed: "+str(e),file=sys.stderr); sys.exit(1)
