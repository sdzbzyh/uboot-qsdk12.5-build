#ifndef __HTTP_HANDLER_TEST_COMMON_H__
#define __HTTP_HANDLER_TEST_COMMON_H__
#include <stddef.h>
#include <string.h>
#include <linux/types.h>
typedef unsigned long ulong;
typedef u64 phys_size_t;
typedef struct bd_info { ulong bi_memstart; phys_size_t bi_memsize; } bd_t;
typedef struct global_data { bd_t *bd; ulong cpu_clk; } gd_t;
extern gd_t *gd;
ulong get_timer(ulong base);
#endif
