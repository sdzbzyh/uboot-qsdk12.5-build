#!/usr/bin/env python3
import argparse, hashlib, os, pathlib, subprocess, sys, tempfile

PIN = "801717ba6a8a59ed2cfd53bf3e1b19841fecd2e8"
TFTP = pathlib.Path("u-boot/net/tftp.c")

GET_VERIFY_HELPERS = r'''
#define TFTP_GET_FORMAT_UNSUPPORTED 0
#define TFTP_GET_FORMAT_UIMAGE 1
#define TFTP_GET_FORMAT_FDT 2
#define TFTP_GET_FORMAT_ELF32 3
#define TFTP_GET_FORMAT_ELF64 4

static uint16_t tftp_get_rd16(const uchar *p, int little)
{
	if (little)
		return (uint16_t)p[0] | ((uint16_t)p[1] << 8);
	return ((uint16_t)p[0] << 8) | (uint16_t)p[1];
}

static uint32_t tftp_get_rd32(const uchar *p, int little)
{
	if (little)
		return (uint32_t)p[0] | ((uint32_t)p[1] << 8) |
		       ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
	return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) |
	       ((uint32_t)p[2] << 8) | (uint32_t)p[3];
}

static uint64_t tftp_get_rd64(const uchar *p, int little)
{
	uint64_t hi, lo;
	if (little) {
		lo = tftp_get_rd32(p, 1);
		hi = tftp_get_rd32(p + 4, 1);
	} else {
		hi = tftp_get_rd32(p, 0);
		lo = tftp_get_rd32(p + 4, 0);
	}
	return (hi << 32) | lo;
}

static const char *tftp_get_format_name(int format)
{
	switch (format) {
	case TFTP_GET_FORMAT_UIMAGE: return "legacy-uImage";
	case TFTP_GET_FORMAT_FDT: return "FIT/FDT";
	case TFTP_GET_FORMAT_ELF32: return "ELF32";
	case TFTP_GET_FORMAT_ELF64: return "ELF64";
	default: return "unsupported";
	}
}

/*
 * This is deliberately a bounded header recognizer, not an image loader.
 * Every offset/length test uses subtraction before addition/multiplication.
 */
static int tftp_get_classify(const uchar *p, uint64_t len, int *format)
{
	uint64_t off, count, entsize, table_bytes, shoff;
	uint32_t total, version, last_version, struct_off, strings_off;
	uint32_t reserve_off, struct_size, strings_size, payload_size;
	uint16_t ehsize, phentsize, phnum, shentsize, shnum;
	int little, elf_class;

	if (!p || !format || !len)
		return -1;
	*format = TFTP_GET_FORMAT_UNSUPPORTED;

	/* Legacy U-Boot image: 64-byte header followed by exactly ih_size bytes. */
	if (len >= 4 && tftp_get_rd32(p, 0) == 0x27051956U) {
		if (len < 64)
			return -1;
		payload_size = tftp_get_rd32(p + 12, 0);
		if ((uint64_t)payload_size != len - 64)
			return -1;
		*format = TFTP_GET_FORMAT_UIMAGE;
		return 0;
	}

	/* FIT and standalone FDT share the FDT magic and header. */
	if (len >= 4 && tftp_get_rd32(p, 0) == 0xd00dfeedU) {
		if (len < 40)
			return -1;
		total = tftp_get_rd32(p + 4, 0);
		struct_off = tftp_get_rd32(p + 8, 0);
		strings_off = tftp_get_rd32(p + 12, 0);
		reserve_off = tftp_get_rd32(p + 16, 0);
		version = tftp_get_rd32(p + 20, 0);
		last_version = tftp_get_rd32(p + 24, 0);
		strings_size = tftp_get_rd32(p + 32, 0);
		struct_size = tftp_get_rd32(p + 36, 0);
		if (total < 40 || (uint64_t)total > len ||
		    version < 16 || version > 17 ||
		    last_version < 16 || last_version > version ||
		    reserve_off < 40 || reserve_off > total || total - reserve_off < 16 ||
		    struct_off < 40 || struct_off > total ||
		    strings_off < 40 || strings_off > total ||
		    struct_size > total - struct_off ||
		    strings_size > total - strings_off)
			return -1;
		*format = TFTP_GET_FORMAT_FDT;
		return 0;
	}

	/* ELF32/ELF64: validate identification, fixed header and program table. */
	if (len >= 4 && p[0] == 0x7f && p[1] == 'E' &&
	    p[2] == 'L' && p[3] == 'F') {
		if (len < 16 || p[6] != 1)
			return -1;
		elf_class = p[4];
		if ((elf_class != 1 && elf_class != 2) ||
		    (p[5] != 1 && p[5] != 2))
			return -1;
		little = (p[5] == 1);
		if (elf_class == 1) {
			if (len < 52)
				return -1;
			ehsize = tftp_get_rd16(p + 40, little);
			off = tftp_get_rd32(p + 28, little);
			shoff = tftp_get_rd32(p + 32, little);
			shentsize = tftp_get_rd16(p + 46, little);
			shnum = tftp_get_rd16(p + 48, little);
			phentsize = tftp_get_rd16(p + 42, little);
			phnum = tftp_get_rd16(p + 44, little);
			if (ehsize != 52 || (phnum && phentsize < 32))
				return -1;
			*format = TFTP_GET_FORMAT_ELF32;
		} else {
			if (len < 64)
				return -1;
			ehsize = tftp_get_rd16(p + 52, little);
			off = tftp_get_rd64(p + 32, little);
			shoff = tftp_get_rd64(p + 40, little);
			shentsize = tftp_get_rd16(p + 58, little);
			shnum = tftp_get_rd16(p + 60, little);
			phentsize = tftp_get_rd16(p + 54, little);
			phnum = tftp_get_rd16(p + 56, little);
			if (ehsize != 64 || (phnum && phentsize < 56))
				return -1;
			*format = TFTP_GET_FORMAT_ELF64;
		}
		if (tftp_get_rd32(p + 20, little) != 1)
			return -1;
		if (phnum) {
			if (!phentsize || off > len)
				return -1;
			count = phnum;
			entsize = phentsize;
			if (count > (len - off) / entsize)
				return -1;
			table_bytes = count * entsize;
			if (table_bytes > len - off)
				return -1;
		} else if (off > len) {
			return -1;
		}
		if (shnum) {
			if (shentsize < ((elf_class == 1) ? 40 : 64) || shoff > len ||
			    (uint64_t)shnum > (len - shoff) / shentsize)
				return -1;
		} else if (shoff > len) {
			return -1;
		}
		return 0;
	}

	return -1;
}

static void tftp_get_print_sha256(const uchar digest[SHA256_SUM_LEN])
{
	unsigned int i;
	for (i = 0; i < SHA256_SUM_LEN; i++)
		printf("%02x", digest[i]);
}

static int tftp_get_finish_verified(const uchar *data, uint64_t size)
{
	uint32_t crc;
	uchar digest[SHA256_SUM_LEN];
	int format;

	if (TftpGetState != TFTP_GET_ACTIVE ||
	    size != (uint64_t)TftpGetReceivedSize ||
	    size > (uint64_t)TftpGetReservedSize ||
	    NetBootFileXferSize != TftpGetReceivedSize ||
	    !data || !size || size > 0xffffffffULL ||
	    size > (uint64_t)CONFIG_PA220_TFTP_MAX_FILE_SIZE
#ifdef CONFIG_TFTP_TSIZE
	    || (TftpTsize && (uint64_t)TftpTsize != size)
#endif
	    ) {
		tftp_get_fail("completed file length/state mismatch");
		return -1;
	}

	/* Publish nothing until the cache range and all checks have succeeded. */
	if (size > (uint64_t)(~0UL - (ulong)(uintptr_t)data)) {
		tftp_get_fail("checksum range overflow");
		return -1;
	}
	flush_dcache_range((ulong)(uintptr_t)data,
			   (ulong)(uintptr_t)data + (ulong)size);
	crc = crc32_wd(0, data, (uint)size, CHUNKSZ_SHA256);
	sha256_csum_wd(data, (uint)size, digest, CHUNKSZ_SHA256);
	if (tftp_get_classify(data, size, &format)) {
		printf("\nTFTP GET rejected: unsupported or malformed file header\n");
		tftp_get_fail("unsupported or malformed file format");
		return -1;
	}

	printf("\nTFTP GET verified: size=%lu format=%s CRC32=%08x SHA256=",
	       (ulong)size, tftp_get_format_name(format), crc);
	tftp_get_print_sha256(digest);
	puts("");
	TftpGetState = TFTP_GET_COMPLETE;
	return 0;
}
'''

GET_VERIFY_FINISH = r'''static int tftp_get_finish_verified(const uchar *data, uint64_t size);
static int tftp_get_finish(void)
{
 return tftp_get_finish_verified((const uchar *)(uintptr_t)load_addr,
                                (uint64_t)TftpGetReceivedSize);
}
'''

def git(root, *args, env=None):
    return subprocess.run(["git", "-C", str(root), *args], check=True,
                          stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                          text=True, env=env).stdout

def rep(source, old, new, label):
    if source.count(old) != 1:
        raise RuntimeError(label + " anchor count=" + str(source.count(old)))
    return source.replace(old, new, 1)

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--source-root", required=True)
    parser.add_argument("--output", required=True)
    args = parser.parse_args()
    root = pathlib.Path(args.source_root).resolve()
    if git(root, "rev-parse", "HEAD").strip() != PIN:
        raise RuntimeError("wrong PA-220 source pin")
    path = root / TFTP
    out = pathlib.Path(args.output)
    source = path.read_text()

    if "tftp_get_finish_verified" in source:
        if not out.is_file() or "tftp_get_finish_verified" not in out.read_text():
            raise RuntimeError("P6-1C source exists but generated patch is missing")
        print("P6-1C_ALREADY_GENERATED=verified")
        print("PATCH=" + str(out))
        print("PATCH_SHA256=" + hashlib.sha256(out.read_bytes()).hexdigest())
        print("DIFF_GENERATED_BY=git diff --binary")
        return 0

    if "tftp_get_reserve(void)" not in source or "TftpGetAbort" not in source:
        raise RuntimeError("P6-1B receive lifecycle is not present")
    if "#include <sha256.h>" in source or "#include <u-boot/crc.h>" in source:
        raise RuntimeError("unexpected partial P6-1C include state")

    with tempfile.TemporaryDirectory() as td:
        env = os.environ.copy()
        env["GIT_INDEX_FILE"] = str(pathlib.Path(td) / "index")
        git(root, "read-tree", "HEAD", env=env)
        git(root, "add", "--", str(TFTP), env=env)

        source = rep(source, "#include <net.h>",
                     "#include <net.h>\n#include <sha256.h>\n#include <u-boot/crc.h>",
                     "checksum includes")
        abort = '''void TftpGetAbort(void)
{
 if (TftpGetState == TFTP_GET_ACTIVE) tftp_get_fail("interrupted or aborted");
}
'''
        source = rep(source, abort, abort + GET_VERIFY_HELPERS,
                     "verification helpers")
        old_finish = '''static int tftp_get_finish(void)
{
 if (TftpGetState!=TFTP_GET_ACTIVE || TftpGetReceivedSize>TftpGetReservedSize || NetBootFileXferSize!=TftpGetReceivedSize
#ifdef CONFIG_TFTP_TSIZE
 || (TftpTsize && TftpTsize!=TftpGetReceivedSize)
#endif
 ) { TftpGetState=TFTP_GET_FAILED; TftpGetReceivedSize=0; TftpGetReservedSize=0; NetBootFileXferSize=0; (void)tftp_invalidate_get_metadata(); net_set_state(NETLOOP_FAIL); return -1; }
 TftpGetState=TFTP_GET_COMPLETE; return 0;
}
'''
        new_finish = GET_VERIFY_FINISH
        source = rep(source, old_finish, new_finish, "verified completion gate")

        path.write_text(source)
        patch = git(root, "diff", "--no-ext-diff", "--no-renames",
                    "--binary", "--", str(TFTP), env=env)
        if (not patch or "tftp_get_finish_verified" not in patch or
            "sha256_csum_wd" not in patch or "crc32_wd" not in patch or
            "TftpWriting" in patch or "saveenv" in patch):
            raise RuntimeError("incomplete P6-1C patch or PUT/WRQ behavior touched")
        out.parent.mkdir(parents=True, exist_ok=True)
        out.write_text(patch)

    print("PA220_SOURCE_PIN=" + PIN)
    print("P6_1C_SHA256_CRC32=integrated")
    print("P6_1C_READ_ONLY_FORMATS=legacy-uImage,FIT-FDT,ELF32,ELF64")
    print("P6_1C_LENGTH_AND_HEADER_BOUNDS=checked")
    print("P6_1C_FAILURE_INVALIDATES_METADATA=verified")
    print("PUT_WRQ_UNCHANGED=verified")
    print("PATCH=" + str(out))
    print("PATCH_SHA256=" + hashlib.sha256(out.read_bytes()).hexdigest())
    print("DIFF_GENERATED_BY=git diff --binary")
    return 0

if __name__ == "__main__":
    try:
        sys.exit(main())
    except Exception as exc:
        print("P6-1C patch generation failed: " + str(exc), file=sys.stderr)
        sys.exit(1)
