#ifndef PA220_HOST_GLOBAL_DATA_H
#define PA220_HOST_GLOBAL_DATA_H
#include <common.h>
struct bd_info { u64 bi_memstart; u64 bi_memsize; };
struct global_data { struct bd_info *bd; u64 cpu_clk; };
extern struct global_data *gd;
#define DECLARE_GLOBAL_DATA_PTR extern struct global_data *gd
#endif
