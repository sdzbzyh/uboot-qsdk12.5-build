#!/usr/bin/env python3
"""Generate PA-220 RAM-validator and TFTP pre-write diffs from pinned source."""
import argparse
import hashlib
import pathlib
import subprocess
import sys

PIN = "801717ba6a8a59ed2cfd53bf3e1b19841fecd2e8"
MAKEFILE = pathlib.Path("u-boot/arch/mips/cpu/octeon/Makefile")
TFTP = pathlib.Path("u-boot/net/tftp.c")
MAKE_ANCHOR = "\t\tocteon_exec.o timer.o\n"

STORE_BLOCK = r'''static inline int
store_block(int block, uchar *src, unsigned len)
{
	ulong offset, newsize, dest;
#ifdef CONFIG_SYS_DIRECT_FLASH_TFTP
	int i, rc = 0;
#endif
#ifdef CONFIG_OCTEON_CN70XX
	uint64_t block_index, block_offset, offset64, dest64, newsize64;
#endif

#ifdef CONFIG_OCTEON_CN70XX
	if (!TftpBlkSize || len > TftpBlkSize)
		goto range_error;
	if (block == -1) {
		if ((uint64_t)TftpBlockWrapOffset < (uint64_t)TftpBlkSize)
			goto range_error;
		offset64 = (uint64_t)TftpBlockWrapOffset -
			   (uint64_t)TftpBlkSize;
	} else {
		if (block < 0)
			goto range_error;
		block_index = (uint64_t)(unsigned int)block;
		if (block_index > (~0ULL / (uint64_t)TftpBlkSize))
			goto range_error;
		block_offset = block_index * (uint64_t)TftpBlkSize;
		if (block_offset > ~0ULL - (uint64_t)TftpBlockWrapOffset)
			goto range_error;
		offset64 = block_offset + (uint64_t)TftpBlockWrapOffset;
	}
	if (offset64 > ~0ULL - (uint64_t)len)
		goto range_error;
	newsize64 = offset64 + (uint64_t)len;
	if ((uint64_t)load_addr > ~0ULL - offset64)
		goto range_error;
	dest64 = (uint64_t)load_addr + offset64;
	if (dest64 > (uint64_t)(ulong)~0UL ||
	    newsize64 > (uint64_t)(ulong)~0UL)
		goto range_error;
	offset = (ulong)offset64;
	newsize = (ulong)newsize64;
	dest = load_addr + offset;
	if ((uint64_t)dest != dest64)
		goto range_error;
	if (len && octeon_load_range_check(dest64, (uint64_t)len))
		goto range_error;
#else
	offset = block * TftpBlkSize + TftpBlockWrapOffset;
	newsize = offset + len;
	dest = load_addr + offset;
#endif
#ifdef CONFIG_SYS_DIRECT_FLASH_TFTP
	for (i = 0; i < CONFIG_SYS_MAX_FLASH_BANKS; i++) {
		/* start address in flash? */
		if (flash_info[i].flash_id == FLASH_UNKNOWN)
			continue;
		if (dest >= flash_info[i].start[0]) {
			rc = 1;
			break;
		}
	}

	if (rc) { /* Flash is destination for this packet */
		rc = flash_write((char *)src, dest, len);
		if (rc) {
			flash_perror(rc);
			net_set_state(NETLOOP_FAIL);
			return -1;
		}
	} else
#endif /* CONFIG_SYS_DIRECT_FLASH_TFTP */
	{
#ifdef CONFIG_OCTEON_CN70XX
		if (len)
			(void)memcpy((void *)dest, src, len);
#else
		(void)memcpy((void *)dest, src, len);
#endif
	}
#ifdef CONFIG_MCAST_TFTP
	if (Multicast)
		ext2_set_bit(block, Bitmap);
#endif

	if (NetBootFileXferSize < newsize)
		NetBootFileXferSize = newsize;
	return 0;

#ifdef CONFIG_OCTEON_CN70XX
range_error:
	printf("TFTP: rejecting DATA block (offset overflow or outside __tmp_load)\n");
	net_set_state(NETLOOP_FAIL);
	return -1;
#endif
}'''

UPDATE_BLOCK = r'''static int update_block_number(void)
{
#ifdef CONFIG_OCTEON_CN70XX
	uint64_t wrap_delta, max_offset;
#endif

	/*
	 * RFC1350 specifies that the first data packet will
	 * have sequence number 1. If we receive a sequence
	 * number of 0 this means that there was a wrap
	 * around of the (16 bit) counter.
	 */
	if (TftpBlock == 0 && TftpLastBlock != 0) {
#ifdef CONFIG_OCTEON_CN70XX
		wrap_delta = (uint64_t)TftpBlkSize *
			     (uint64_t)TFTP_SEQUENCE_SIZE;
		max_offset = (uint64_t)(ulong)~0UL;
		if (TftpBlockWrap == (ulong)~0UL ||
		    (uint64_t)TftpBlockWrapOffset > max_offset - wrap_delta) {
			printf("TFTP: block-wrap offset overflow; aborting transfer\n");
			net_set_state(NETLOOP_FAIL);
			return -1;
		}
#endif
		TftpBlockWrap++;
		TftpBlockWrapOffset += TftpBlkSize * TFTP_SEQUENCE_SIZE;
		TftpTimeoutCount = 0; /* we've done well, reset the timeout */
	} else {
		show_block_marker();
	}
	return 0;
}'''


def run(root, *args):
    return subprocess.run(["git", "-C", str(root), *args], check=True,
                          stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                          text=True).stdout.strip()


def replace_function(text, start_marker, end_marker, replacement):
    start = text.find(start_marker)
    if start < 0 or text.find(start_marker, start + 1) >= 0:
        raise RuntimeError("function start marker is missing or ambiguous: " + start_marker)
    end = text.find(end_marker, start)
    if end < 0:
        raise RuntimeError("function end marker is missing: " + end_marker)
    return text[:start] + replacement + "\n\n" + text[end:]


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--source-root", required=True)
    parser.add_argument("--output", required=True)
    args = parser.parse_args()
    root = pathlib.Path(args.source_root).resolve()
    if run(root, "rev-parse", "HEAD") != PIN:
        raise RuntimeError("source checkout is not the fixed PA-220 commit")

    make_path = root / MAKEFILE
    make_text = make_path.read_text()
    if "COBJS-y += octeon_load_range.o" not in make_text:
        if make_text.count(MAKE_ANCHOR) != 1:
            raise RuntimeError("OCTEON Makefile insertion anchor is not unique")
        make_path.write_text(make_text.replace(
            MAKE_ANCHOR, MAKE_ANCHOR + "COBJS-y += octeon_load_range.o\n", 1))

    tftp_path = root / TFTP
    tftp_text = tftp_path.read_text()
    include = "#ifdef CONFIG_OCTEON_CN70XX\n#include <asm/arch/octeon_load_range.h>\n#endif\n"
    if "octeon_load_range.h" not in tftp_text:
        if tftp_text.count("#include <common.h>\n") != 1:
            raise RuntimeError("TFTP common.h include anchor is not unique")
        tftp_text = tftp_text.replace("#include <common.h>\n",
                                      "#include <common.h>\n" + include, 1)

    if "octeon_load_range_check(dest64" not in tftp_text:
        tftp_text = replace_function(
            tftp_text, "static inline void\nstore_block(",
            "\n/* Clear our state ready for a new transfer */", STORE_BLOCK)
    if "static int update_block_number(void)" not in tftp_text:
        tftp_text = replace_function(
            tftp_text, "static void update_block_number(void)",
            "\n/* The TFTP get or put is complete */", UPDATE_BLOCK)

    lines = tftp_text.splitlines(keepends=True)
    wrap_new = 0
    wrap_existing = 0
    store_new = 0
    store_existing = 0
    for index, line in enumerate(lines):
        stripped = line.strip()
        if stripped == "update_block_number();":
            indent = line[:len(line) - len(line.lstrip())]
            lines[index] = (indent + "if (update_block_number())\n" +
                            indent + "\tbreak;\n")
            wrap_new += 1
        elif stripped == "if (update_block_number())":
            wrap_existing += 1
        elif stripped == "store_block(TftpBlock - 1, pkt + 2, len);":
            indent = line[:len(line) - len(line.lstrip())]
            lines[index] = (indent + "if (store_block((int)TftpBlock - 1, "
                            "pkt + 2, len))\n" + indent + "\treturn;\n")
            store_new += 1
        elif stripped == "if (store_block((int)TftpBlock - 1, pkt + 2, len))":
            store_existing += 1
    if wrap_new + wrap_existing != 2 or store_new + store_existing != 1:
        raise RuntimeError(
            "unexpected TFTP call-site counts: wrap=%d+%d store=%d+%d" %
            (wrap_new, wrap_existing, store_new, store_existing))
    tftp_text = "".join(lines)
    if tftp_text.count("octeon_load_range_check(dest64") != 1:
        raise RuntimeError("TFTP DATA pre-write validator call missing or duplicated")
    tftp_path.write_text(tftp_text)

    output = pathlib.Path(args.output)
    output.parent.mkdir(parents=True, exist_ok=True)
    make_patch = run(root, "diff", "--no-ext-diff", "--no-renames", "--binary",
                     "--", str(MAKEFILE))
    tftp_patch = run(root, "diff", "--no-ext-diff", "--no-renames", "--binary",
                     "--", str(TFTP))
    if "octeon_load_range.o" not in make_patch:
        raise RuntimeError("Makefile diff does not include validator object")
    if "octeon_load_range_check" not in tftp_patch:
        raise RuntimeError("TFTP diff does not include pre-write validator")
    output.write_text(make_patch + "\n")
    tftp_output = output.with_name("0002-pa220-tftp-prewrite-guard.patch")
    tftp_output.write_text(tftp_patch + "\n")
    print("PIN=" + PIN)
    print("MAKEFILE_PATCH=" + str(output))
    print("MAKEFILE_PATCH_SHA256=" + hashlib.sha256(output.read_bytes()).hexdigest())
    print("TFTP_PATCH=" + str(tftp_output))
    print("TFTP_PATCH_SHA256=" + hashlib.sha256(tftp_output.read_bytes()).hexdigest())
    print("TFTP_GUARD=octeon_load_range_check before memcpy")
    print("DIFF_GENERATED_BY=git diff --binary")


if __name__ == "__main__":
    try:
        sys.exit(main())
    except Exception as exc:
        print("load-range integration patch generation failed: " + str(exc),
              file=sys.stderr)
        sys.exit(1)
