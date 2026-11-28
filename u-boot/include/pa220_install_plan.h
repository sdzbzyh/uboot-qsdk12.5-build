/* SPDX-License-Identifier: GPL-2.0+ */
#ifndef __PA220_INSTALL_PLAN_H__
#define __PA220_INSTALL_PLAN_H__
#include <stdint.h>
#include <pa220_partition.h>

#define PA220_PLAN_MAX_COMPONENTS 8
#define PA220_PLAN_NAME_MAX 32
#define PA220_PLAN_SECTOR_SIZE 512U
#define PA220_PLAN_BOARD_TYPE 20021U

#define PA220_PLAN_FORMAT_OCTEON (1U << 0)
#define PA220_PLAN_FORMAT_UIMAGE (1U << 1)
#define PA220_PLAN_FORMAT_FIT    (1U << 2)
#define PA220_PLAN_FORMAT_ELF32  (1U << 3)
#define PA220_PLAN_FORMAT_ELF64  (1U << 4)

#define PA220_PLAN_KIND_PRIMARY (1U << PA220_PART_PRIMARY)
#define PA220_PLAN_KIND_LOGICAL (1U << PA220_PART_LOGICAL)
#define PA220_PLAN_KIND_GPT     (1U << PA220_PART_GPT)

enum pa220_manifest_state {
    PA220_MANIFEST_ABSENT = 0,
    PA220_MANIFEST_PRESENT_UNREVIEWED,
    PA220_MANIFEST_APPROVED
};

enum pa220_plan_status {
    PA220_PLAN_OK = 0,
    PA220_PLAN_NO_APPROVED_MANIFEST,
    PA220_PLAN_BAD_MANIFEST,
    PA220_PLAN_BAD_PARTITION_TABLE,
    PA220_PLAN_BAD_COMPONENT,
    PA220_PLAN_BAD_SELECTION,
    PA220_PLAN_TARGET_MISMATCH,
    PA220_PLAN_CAPACITY,
    PA220_PLAN_OVERLAP
};

struct pa220_manifest_component_policy {
    char name[PA220_PLAN_NAME_MAX];
    uint32_t format_mask;
    uint32_t allowed_kind_mask;
    uint8_t allowed_mbr_type;
    uint8_t gpt_type_guid[16];
    uint64_t min_bytes;
    uint64_t max_bytes;
};

struct pa220_install_manifest {
    enum pa220_manifest_state state;
    uint32_t board_type;
    uint32_t component_count;
    char manifest_id[PA220_PLAN_NAME_MAX];
    struct pa220_manifest_component_policy components[PA220_PLAN_MAX_COMPONENTS];
};

struct pa220_plan_component {
    char name[PA220_PLAN_NAME_MAX];
    uint32_t format_mask;
    uint64_t source_offset;
    uint64_t size_bytes;
    uint8_t integrity_verified;
};

struct pa220_target_selection {
    char component_name[PA220_PLAN_NAME_MAX];
    uint32_t partition_index;
};

struct pa220_plan_item {
    uint32_t partition_index;
    uint64_t target_start_lba;
    uint64_t target_sectors;
    uint64_t required_sectors;
    uint64_t source_offset;
    uint64_t source_size;
};

struct pa220_install_plan {
    enum pa220_plan_status status;
    uint32_t item_count;
    struct pa220_plan_item items[PA220_PLAN_MAX_COMPONENTS];
};

enum pa220_plan_status pa220_install_plan_build(
    const struct pa220_install_manifest *manifest,
    const struct pa220_partition_table *partitions,
    uint64_t image_size,
    const struct pa220_plan_component *components,
    uint32_t component_count,
    const struct pa220_target_selection *selections,
    uint32_t selection_count,
    struct pa220_install_plan *out);

const char *pa220_plan_status_name(enum pa220_plan_status status);
#endif
