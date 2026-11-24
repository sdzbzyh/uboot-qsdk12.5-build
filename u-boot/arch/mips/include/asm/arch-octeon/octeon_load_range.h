/* SPDX-License-Identifier: GPL-2.0+ */
#ifndef __OCTEON_LOAD_RANGE_H__
#define __OCTEON_LOAD_RANGE_H__

#ifdef PA220_LOAD_RANGE_HOST_TEST
#include <stdint.h>
#else
#include <common.h>
#endif

/* Return 0 only when the complete half-open write range is safe. */
int octeon_load_range_check(uint64_t addr, uint64_t len);

#endif /* __OCTEON_LOAD_RANGE_H__ */
