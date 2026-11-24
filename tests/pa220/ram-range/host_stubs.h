#ifndef PA220_LOAD_RANGE_HOST_STUBS_H
#define PA220_LOAD_RANGE_HOST_STUBS_H
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <limits.h>
typedef unsigned long ulong;
#define CVMX_BOOTMEM_NAME_LEN 128
#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))
#define OCTEON_BOOTLOADER_LOAD_MEM_NAME "__tmp_load"
typedef struct cvmx_bootmem_named_block_desc {
	uint64_t base_addr;
	uint64_t size;
	char name[CVMX_BOOTMEM_NAME_LEN];
} cvmx_bootmem_named_block_desc_t;
struct host_bd_info {
	uint64_t bi_uboot_ram_addr;
	unsigned long bi_uboot_ram_used_size;
};
struct host_gd_info {
	struct host_bd_info *bd;
};
extern struct host_gd_info *gd;
extern ulong load_reserved_addr;
extern ulong load_reserved_size;
const cvmx_bootmem_named_block_desc_t *
cvmx_bootmem_find_named_block(const char *name);
#endif
