#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <limits.h>
#include <stdlib.h>
#define PA220_LOAD_RANGE_HOST_TEST 1
#include "host_stubs.h"
#include "../../../u-boot/arch/mips/include/asm/arch-octeon/octeon_load_range.h"
#define LOAD_BASE 0x20000000ULL
#define LOAD_SIZE 0x10000000ULL
#define UBOOT_BASE 0x1f2000000ULL
#define UBOOT_SIZE 0x01000000UL
#define MAX_BLOCKS 16
struct mock_block {
	char name[CVMX_BOOTMEM_NAME_LEN];
	uint64_t base;
	uint64_t size;
};
static struct mock_block blocks[MAX_BLOCKS];
static unsigned int block_count;
static cvmx_bootmem_named_block_desc_t static_desc;
static int missing_load, invalid_load, zero_base_load, wrong_name_load;
static int zero_optional, change_load_on_second_read, malformed_optional;
static unsigned int load_queries, cases, assertions, failures, writes;
static unsigned char write_guard[32];
static struct host_bd_info board_data;
static struct host_gd_info global_data;
struct host_gd_info *gd = &global_data;
ulong load_reserved_addr, load_reserved_size;
static void check(int condition, const char *label)
{
	assertions++;
	if (!condition) { failures++; printf("FAIL: %s\n", label); }
}
static void add_block(const char *name, uint64_t base, uint64_t size)
{
	struct mock_block *b;
	if (block_count >= MAX_BLOCKS) abort();
	b = &blocks[block_count++];
	memset(b, 0, sizeof(*b));
	strncpy(b->name, name, sizeof(b->name) - 1);
	b->base = base; b->size = size;
}
static void remove_block(const char *name)
{
	unsigned int i;
	for (i = 0; i < block_count; i++) {
		if (!strcmp(blocks[i].name, name)) {
			memmove(&blocks[i], &blocks[i + 1],
				(block_count - i - 1) * sizeof(blocks[0]));
			block_count--; return;
		}
	}
}
static void reset_fixture(void)
{
	block_count = 0; missing_load = invalid_load = zero_base_load = 0;
	wrong_name_load = zero_optional = change_load_on_second_read = 0;
	malformed_optional = 0; load_queries = writes = 0;
	memset(write_guard, 0x5a, sizeof(write_guard));
	board_data.bi_uboot_ram_addr = UBOOT_BASE;
	board_data.bi_uboot_ram_used_size = UBOOT_SIZE;
	global_data.bd = &board_data;
	load_reserved_addr = (ulong)LOAD_BASE; load_reserved_size = (ulong)LOAD_SIZE;
	add_block("__uboot_code_data", UBOOT_BASE, UBOOT_SIZE);
	add_block("__tmp_load", LOAD_BASE, LOAD_SIZE);
	add_block("__tmp_reserved_linux", 0x00100000ULL, 0x0f000000ULL);
	add_block("__fdt", 0x00080000ULL, 0x00020000ULL);
	add_block("idle-core-loop", 0x000ffff0ULL, 0x10ULL);
	add_block("__low_reserved", 0, 0x00100000ULL);
}
const cvmx_bootmem_named_block_desc_t *
cvmx_bootmem_find_named_block(const char *name)
{
	unsigned int i; struct mock_block *b = NULL;
	if (!strcmp(name, OCTEON_BOOTLOADER_LOAD_MEM_NAME)) {
		load_queries++; if (missing_load) return NULL;
	}
	for (i = 0; i < block_count; i++)
		if (!strcmp(blocks[i].name, name)) { b = &blocks[i]; break; }
	if (!b) return NULL;
	memset(&static_desc, 0, sizeof(static_desc));
	static_desc.base_addr = b->base; static_desc.size = b->size;
	memcpy(static_desc.name, b->name, sizeof(static_desc.name));
	if (!strcmp(name, OCTEON_BOOTLOADER_LOAD_MEM_NAME)) {
		if (invalid_load) static_desc.size = 0;
		if (zero_base_load) static_desc.base_addr = 0;
		if (wrong_name_load) static_desc.name[0] = 'X';
		if (change_load_on_second_read && (load_queries % 2) == 0)
			static_desc.base_addr += 0x1000;
	}
	if (zero_optional && !strcmp(name, "__tmp_reserved_linux"))
		static_desc.size = 0;
	if (malformed_optional && !strcmp(name, "__tmp_reserved_linux")) {
		static_desc.base_addr = UINT64_MAX - 0x10;
		static_desc.size = 0x20;
	}
	return &static_desc;
}
static int guarded_write(uint64_t addr, uint64_t len)
{
	if (octeon_load_range_check(addr, len)) return -1;
	writes++; memset(write_guard, 0xa5, sizeof(write_guard)); return 0;
}
static void expect_pass(const char *name, uint64_t addr, uint64_t len)
{
	unsigned int before = writes; cases++;
	check(octeon_load_range_check(addr, len) == 0, name);
	check(guarded_write(addr, len) == 0, "accepted range reaches guarded write");
	check(writes == before + 1, "accepted range increments write count");
}
static void expect_reject(const char *name, uint64_t addr, uint64_t len)
{
	unsigned int before = writes; unsigned char saved[sizeof(write_guard)];
	memcpy(saved, write_guard, sizeof(saved)); cases++;
	check(octeon_load_range_check(addr, len) != 0, name);
	check(guarded_write(addr, len) != 0, "rejected range blocks guarded write");
	check(writes == before, "rejected range performs no write");
	check(!memcmp(saved, write_guard, sizeof(saved)),
	      "rejected range leaves destination unchanged");
}
static void expect_state_reject(const char *name)
{
	unsigned int before = writes; unsigned char saved[sizeof(write_guard)];
	memcpy(saved, write_guard, sizeof(saved)); cases++;
	check(octeon_load_range_check(LOAD_BASE + 0x100, 0x20) != 0, name);
	check(guarded_write(LOAD_BASE + 0x100, 0x20) != 0,
	      "invalid state blocks guarded write");
	check(writes == before, "invalid state performs no write");
	check(!memcmp(saved, write_guard, sizeof(saved)),
	      "invalid state leaves destination unchanged");
}
int main(void)
{
	reset_fixture(); expect_pass("legal interior range", LOAD_BASE + 0x100, 0x100);
	reset_fixture(); expect_pass("start exactly at allowed base", LOAD_BASE, 1);
	reset_fixture(); expect_pass("end exactly at allowed end", LOAD_BASE + LOAD_SIZE - 16, 16);
	reset_fixture(); expect_reject("start below allowed range", LOAD_BASE - 1, 1);
	reset_fixture(); expect_reject("start at exclusive end", LOAD_BASE + LOAD_SIZE, 1);
	reset_fixture(); expect_reject("range crosses exclusive end", LOAD_BASE + LOAD_SIZE - 4, 8);
	reset_fixture(); expect_reject("zero length", LOAD_BASE, 0);
	reset_fixture(); expect_reject("64-bit checked-add overflow", UINT64_MAX - 3, 8);
	reset_fixture(); expect_reject("address wrap at UINT64_MAX", UINT64_MAX, 1);
	reset_fixture(); missing_load = 1; expect_state_reject("named descriptor missing");
	reset_fixture(); invalid_load = 1; expect_state_reject("named descriptor has zero size");
	reset_fixture(); zero_base_load = 1; load_reserved_addr = 0;
	expect_state_reject("named descriptor has zero base");
	reset_fixture(); wrong_name_load = 1; expect_state_reject("named descriptor name mismatch");
	reset_fixture(); change_load_on_second_read = 1; expect_state_reject("named descriptor changes between snapshots");
	reset_fixture(); load_reserved_addr++; expect_state_reject("load_reserved_addr disagrees with descriptor");
	reset_fixture(); load_reserved_size--; expect_state_reject("load_reserved_size disagrees with descriptor");
	reset_fixture(); expect_pass("cached XKPHYS alias", 0x8000000020000100ULL, 0x20);
	reset_fixture(); expect_pass("uncached XKPHYS CCA alias", 0x9800000020000100ULL, 0x20);
	reset_fixture(); expect_reject("historical 0x81000000 KSEG alias is outside current block", 0x81000000ULL, 0x100);
	reset_fixture(); expect_reject("uncached KSEG1 alias is outside current block", 0xa1000000ULL, 0x100);
	reset_fixture(); expect_reject("unsupported U-Boot link address domain", 0xc0000000ULL, 0x100);
	reset_fixture(); expect_reject("XKPHYS reserved MBZ bits are rejected", 0x8002000020000100ULL, 0x20);
	reset_fixture(); remove_block("__tmp_load"); add_block("__tmp_load", 0x1ffff000ULL, 0x1000ULL);
	load_reserved_addr = 0x1ffff000UL; load_reserved_size = 0x1000UL;
	remove_block("__tmp_reserved_linux");
	expect_reject("KSEG alias may not cross cached/uncached segment boundary", 0x9fffffffULL, 2);
	reset_fixture(); remove_block("__tmp_reserved_linux");
	add_block("__tmp_reserved_linux", LOAD_BASE + 0x1000, 0x1000);
	expect_state_reject("load window overlaps Linux reserved named block");
	reset_fixture(); board_data.bi_uboot_ram_addr = LOAD_BASE + 0x1000;
	board_data.bi_uboot_ram_used_size = 0x1000;
	remove_block("__uboot_code_data");
	add_block("__uboot_code_data", board_data.bi_uboot_ram_addr,
		  board_data.bi_uboot_ram_used_size);
	expect_state_reject("load window overlaps U-Boot relocation/heap/stack/GD range");
	reset_fixture(); malformed_optional = 1;
	expect_state_reject("reserved descriptor end overflow is rejected");
	reset_fixture(); zero_optional = 1;
	expect_state_reject("reserved descriptor zero size is rejected");
	reset_fixture(); remove_block("__uboot_code_data");
	expect_state_reject("missing U-Boot reservation descriptor is rejected");
	reset_fixture(); board_data.bi_uboot_ram_addr = UINT64_MAX - 0x10;
	board_data.bi_uboot_ram_used_size = 0x20;
	remove_block("__uboot_code_data");
	add_block("__uboot_code_data", board_data.bi_uboot_ram_addr,
		  board_data.bi_uboot_ram_used_size);
	expect_state_reject("U-Boot reservation end overflow is rejected");
	reset_fixture(); load_reserved_addr = (ulong)(UINT64_MAX - 0x10);
	load_reserved_size = 0x20; remove_block("__tmp_load");
	add_block("__tmp_load", UINT64_MAX - 0x10, 0x20);
	expect_state_reject("load descriptor base plus size overflow");
	/* KSEG cached/uncached aliases normalize to the same low physical range. */
	reset_fixture(); remove_block("__tmp_load");
	add_block("__tmp_load", 0x01000000ULL, 0x10000ULL);
	load_reserved_addr = 0x01000000UL; load_reserved_size = 0x10000UL;
	remove_block("__tmp_reserved_linux");
	expect_pass("cached KSEG0 alias normalization", 0x81000020ULL, 0x20);
	reset_fixture(); remove_block("__tmp_load");
	add_block("__tmp_load", 0x01000000ULL, 0x10000ULL);
	load_reserved_addr = 0x01000000UL; load_reserved_size = 0x10000UL;
	remove_block("__tmp_reserved_linux");
	expect_pass("uncached KSEG1 alias normalization", 0xa1000020ULL, 0x20);
	printf("HOST RAM RANGE: cases=%u assertions=%u failures=%u writes=%u\n",
	       cases, assertions, failures, writes);
	return failures ? 1 : 0;
}
