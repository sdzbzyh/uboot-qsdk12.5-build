/*
 * PA-220 RAM write-range gate shared by future TFTP, HTTP and ELF paths.
 * The only writable window is the live __tmp_load bootmem named block.
 *
 * SPDX-License-Identifier: GPL-2.0+
 */
#include "octeon_load_range.h"

#ifdef PA220_LOAD_RANGE_HOST_TEST
#include "host_stubs.h"
#else
#include <common.h>
#include <asm/global_data.h>
#include <asm/arch/cvmx-bootmem.h>
#include <asm/arch/octeon_mem_map.h>
#endif

#ifndef PA220_LOAD_RANGE_HOST_TEST
#ifndef CONFIG_OCTEON_CN70XX
#error "PA-220 RAM range validator requires the fixed CN70XX DDR map"
#endif
DECLARE_GLOBAL_DATA_PTR;
#endif

extern ulong load_reserved_addr;
extern ulong load_reserved_size;

#define PA220_DDR0_BASE 0x000000000ULL
#define PA220_DDR0_SIZE 0x010000000ULL
#define PA220_DDR1_BASE 0x020000000ULL
#define PA220_DDR2_BASE 0x030000000ULL
#define PA220_DDR2_SIZE 0x7d0000000ULL

#define OCTEON_LOAD_U64_MAX             (~0ULL)
#define OCTEON_LOAD_LOW_RESERVED_SIZE  (1ULL << 20)
#define OCTEON_XKPHYS_REGION_MASK      (3ULL << 62)
#define OCTEON_XKPHYS_REGION_VALUE     (2ULL << 62)
#define OCTEON_XKPHYS_MBZ_MASK         (((1ULL << 10) - 1) << 49)
#define OCTEON_XKPHYS_PHYS_MASK        ((1ULL << 49) - 1)

struct octeon_load_block_snapshot {
	uint64_t base;
	uint64_t size;
	char name[CVMX_BOOTMEM_NAME_LEN];
};

/*
 * Normalize the PA-220 address forms to the physical domain used by bootmem.
 * board_octeon.c installs 1:1 addrmap entries for DDR0 [0, 256 MiB) and for
 * DDR1/DDR2 beginning at 0x20000000 (up to the 32-bit map limit).  The
 * [0x10000000, 0x20000000) boot-bus aperture is not DRAM.  KSEG0/KSEG1 map
 * only the first 512 MiB; XKPHYS CCA aliases carry physical bits 48:0.
 */
static int octeon_load_normalize_addr(uint64_t addr, uint64_t *physical,
				      uint64_t *alias_remaining)
{
	uint32_t low;

	if (!physical || !alias_remaining)
		return -1;

	if ((addr & OCTEON_XKPHYS_REGION_MASK) ==
	    OCTEON_XKPHYS_REGION_VALUE) {
		if (addr & OCTEON_XKPHYS_MBZ_MASK)
			return -1;
		*physical = addr & OCTEON_XKPHYS_PHYS_MASK;
		*alias_remaining = OCTEON_XKPHYS_PHYS_MASK - *physical + 1;
		return 0;
	}

	if ((addr >> 32) == 0 || (addr >> 32) == 0xffffffffULL) {
		low = (uint32_t)addr;
		if (low >= 0x80000000U && low < 0xa0000000U) {
			*physical = low & 0x1fffffffU;
			if (*physical >= PA220_DDR0_SIZE &&
			    *physical < PA220_DDR1_BASE)
				return -1;
			*alias_remaining = 0xa0000000ULL - low;
			if (*physical < PA220_DDR0_SIZE &&
			    *alias_remaining > PA220_DDR0_SIZE - *physical)
				*alias_remaining = PA220_DDR0_SIZE - *physical;
			return 0;
		}
		if (low >= 0xa0000000U && low < 0xc0000000U) {
			*physical = low & 0x1fffffffU;
			if (*physical >= PA220_DDR0_SIZE &&
			    *physical < PA220_DDR1_BASE)
				return -1;
			*alias_remaining = 0xc0000000ULL - low;
			if (*physical < PA220_DDR0_SIZE &&
			    *alias_remaining > PA220_DDR0_SIZE - *physical)
				*alias_remaining = PA220_DDR0_SIZE - *physical;
			return 0;
		}

		/* KSEG2/KSEG3 and the U-Boot link window are not RAM aliases. */
		if (low >= 0xc0000000U || (addr >> 32) == 0xffffffffULL)
			return -1;

		/* 32-bit identity aliases are only valid in the board DDR map. */
		if (low < PA220_DDR0_SIZE) {
			*physical = low;
			*alias_remaining = PA220_DDR0_SIZE - low;
			return 0;
		}
		if (low >= PA220_DDR1_BASE && low < 0x80000000U) {
			*physical = low;
			*alias_remaining = 0x80000000ULL - low;
			return 0;
		}
		return -1;
	}

	/* Reject unsupported 64-bit virtual/IO address forms. */
	return -1;
}

static int octeon_load_checked_end(uint64_t start, uint64_t size,
				   uint64_t *end)
{
	if (!size || !end || start > OCTEON_LOAD_U64_MAX - size)
		return -1;
	*end = start + size;
	return 0;
}

static int octeon_load_ranges_overlap(uint64_t a_start, uint64_t a_end,
				      uint64_t b_start, uint64_t b_end)
{
	return a_start < b_end && b_start < a_end;
}

/*
 * Prove that a named-block descriptor lies in physical DDR, not in the
 * boot-bus hole or beyond the installed RAM.  For CN70XX, DDR0 is followed
 * by the 256 MiB boot-bus aperture; DDR1 and DDR2 are contiguous from
 * PA220_DDR1_BASE.  gd->ram_size is the runtime total across those regions.
 * This validates the descriptor itself; __tmp_load remains the sole write
 * window authority.
 */
static int octeon_load_is_dram_range(uint64_t base, uint64_t end)
{
	uint64_t ram_size, ddr0_size, ddr0_end;
	uint64_t high_size, high_end, max_ddr_end;

	if (!gd)
		return -1;
	ram_size = (uint64_t)gd->ram_size;
	if (!ram_size)
		return -1;

	ddr0_size = ram_size < PA220_DDR0_SIZE ?
		    ram_size : PA220_DDR0_SIZE;
	ddr0_end = PA220_DDR0_BASE + ddr0_size;
	/* DDR0_BASE is zero; end <= ddr0_end also proves base is in DDR0. */
	if (end <= ddr0_end)
		return 0;

	if (ram_size <= ddr0_size)
		return -1;
	high_size = ram_size - ddr0_size;
	if (PA220_DDR1_BASE > OCTEON_LOAD_U64_MAX - high_size ||
	    PA220_DDR2_BASE > OCTEON_LOAD_U64_MAX - PA220_DDR2_SIZE)
		return -1;
	high_end = PA220_DDR1_BASE + high_size;
	max_ddr_end = PA220_DDR2_BASE + PA220_DDR2_SIZE;
	if (high_end > max_ddr_end)
		return -1;

	if (base >= PA220_DDR1_BASE && end <= high_end)
		return 0;
	return -1;
}

/*
 * cvmx_bootmem_find_named_block() returns a pointer to one static snapshot,
 * not to the named-block table.  Copy every field before making another
 * named-block query; never retain or reuse the returned pointer.
 *
 * Return 1 when found, 0 when absent, and -1 for a malformed snapshot.
 */
static int octeon_load_get_named_block(const char *name,
				       struct octeon_load_block_snapshot *out)
{
	const cvmx_bootmem_named_block_desc_t *desc;
	unsigned int i;

	if (!name || !out)
		return -1;

	desc = cvmx_bootmem_find_named_block(name);
	if (!desc)
		return 0;

	/* Immediate copy is mandatory: desc points to a shared static object. */
	out->base = desc->base_addr;
	out->size = desc->size;
	for (i = 0; i < CVMX_BOOTMEM_NAME_LEN; i++)
		out->name[i] = desc->name[i];

	if (strncmp(out->name, name, CVMX_BOOTMEM_NAME_LEN))
		return -1;
	if (!out->size)
		return -1;
	return 1;
}

static int octeon_load_validate_reserved_blocks(uint64_t load_base,
						uint64_t load_end)
{
	static const char * const reserved_names[] = {
		"__tmp_reserved_linux",
		"__tmp_uboot_exception",
		"__fdt",
		"__bootloader_env",
		"__low_reserved",
		"__mac_meminfo",
		"idle-core-loop"
	};
	unsigned int i;

	for (i = 0; i < ARRAY_SIZE(reserved_names); i++) {
		struct octeon_load_block_snapshot block;
		uint64_t block_end;
		int found;

		found = octeon_load_get_named_block(reserved_names[i], &block);
		if (found < 0)
			return -1;
		if (!found)
			continue;
		if (octeon_load_checked_end(block.base, block.size,
					    &block_end))
			return -1;
		if (octeon_load_ranges_overlap(load_base, load_end,
					       block.base, block_end))
			return -1;
	}
	return 0;
}

int octeon_load_range_check(uint64_t addr, uint64_t len)
{
	struct octeon_load_block_snapshot first, second, uboot_block;
	uint64_t load_end, uboot_end;
	uint64_t physical, alias_remaining, offset;
	uint64_t uboot_base, uboot_size;
	int found;

	/* Empty writes, uninitialized RAM state and wraparound are never valid. */
	if (!len || addr > OCTEON_LOAD_U64_MAX - len || !gd || !gd->bd)
		return -1;

	if (octeon_load_normalize_addr(addr, &physical, &alias_remaining))
		return -1;
	if (len > alias_remaining || physical > OCTEON_LOAD_U64_MAX - len)
		return -1;

	/*
	 * Read the live authoritative block twice.  This detects a descriptor
	 * changing during snapshot acquisition.  U-Boot's foreground command
	 * model serializes bootmem mutation; no other core runs bootmem writers.
	 */
	found = octeon_load_get_named_block(OCTEON_BOOTLOADER_LOAD_MEM_NAME,
					    &first);
	if (found != 1)
		return -1;
	found = octeon_load_get_named_block(OCTEON_BOOTLOADER_LOAD_MEM_NAME,
					    &second);
	if (found != 1 || first.base != second.base ||
	    first.size != second.size ||
	    strncmp(first.name, second.name, CVMX_BOOTMEM_NAME_LEN))
		return -1;

	if (!first.base ||
	    octeon_load_checked_end(first.base, first.size, &load_end) ||
	    octeon_load_is_dram_range(first.base, load_end))
		return -1;

	/* Legacy globals are consistency checks only, never the authority. */
	if ((uint64_t)load_reserved_addr != first.base ||
	    (uint64_t)load_reserved_size != first.size)
		return -1;

	/* The complete load window must exclude fixed low boot/FDT/exception data. */
	if (octeon_load_ranges_overlap(first.base, load_end, 0,
				       OCTEON_LOAD_LOW_RESERVED_SIZE))
		return -1;

	/*
	 * U-Boot code/data, malloc, stack, GD, bd and boot parameters share this
	 * board_octeon.c reservation.  Require its named-block record to match
	 * the live board-info range before trusting that exclusion.
	 */
	uboot_base = gd->bd->bi_uboot_ram_addr;
	uboot_size = (uint64_t)gd->bd->bi_uboot_ram_used_size;
	if (!uboot_base ||
	    octeon_load_checked_end(uboot_base, uboot_size, &uboot_end) ||
	    octeon_load_is_dram_range(uboot_base, uboot_end))
		return -1;
	found = octeon_load_get_named_block("__uboot_code_data", &uboot_block);
	if (found != 1 || uboot_block.base != uboot_base ||
	    uboot_block.size != uboot_size)
		return -1;
	if (octeon_load_ranges_overlap(first.base, load_end,
				       uboot_base, uboot_end))
		return -1;

	/* Other named allocations are checked explicitly as defense in depth. */
	if (octeon_load_validate_reserved_blocks(first.base, load_end))
		return -1;

	if (physical < first.base)
		return -1;
	offset = physical - first.base;
	if (offset > first.size || len > first.size - offset)
		return -1;

	return 0;
}
