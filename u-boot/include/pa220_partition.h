/* SPDX-License-Identifier: GPL-2.0+ */
#ifndef __PA220_PARTITION_H__
#define __PA220_PARTITION_H__
#include <stdint.h>
#define PA220_PART_MAX 32
#define PA220_PART_SECTOR_SIZE 512
typedef int (*pa220_sector_read_fn)(void *ctx, uint64_t lba, uint8_t sector[512]);
enum pa220_pt_status { PA220_PT_OK=0, PA220_PT_UNKNOWN, PA220_PT_AMBIGUOUS, PA220_PT_CORRUPT, PA220_PT_IO };
enum pa220_pt_format { PA220_PT_NONE=0, PA220_PT_DOS_MBR, PA220_PT_GPT };
enum pa220_part_kind { PA220_PART_PRIMARY=1, PA220_PART_LOGICAL, PA220_PART_GPT };
struct pa220_partition { uint32_t index; uint64_t start_lba, sectors; uint8_t type, kind; uint8_t type_guid[16], unique_guid[16]; uint8_t name_utf16le[72]; uint64_t attributes; };
struct pa220_partition_table { enum pa220_pt_status status; enum pa220_pt_format format; uint32_t count; uint64_t capacity; uint8_t disk_guid[16]; struct pa220_partition parts[PA220_PART_MAX]; };
int pa220_partition_scan(pa220_sector_read_fn read_sector, void *ctx, uint64_t capacity,
                         struct pa220_partition_table *out);
#endif
