/* SPDX-License-Identifier: GPL-2.0+ */
#ifdef PA220_INSTALL_PLAN_HOST_TEST
#include <stdint.h>
#include <string.h>
#else
#include <common.h>
#endif
#include <pa220_install_plan.h>

static int add_u64(uint64_t a, uint64_t b, uint64_t *out)
{
    if (UINT64_MAX - a < b)
        return 0;
    *out = a + b;
    return 1;
}

static int name_equal(const char *a, const char *b)
{
    unsigned int i;
    if (!a || !b || !a[0] || !b[0])
        return 0;
    for (i = 0; i < PA220_PLAN_NAME_MAX; i++) {
        if (a[i] != b[i])
            return 0;
        if (!a[i])
            return 1;
    }
    return 0;
}

static int guid_is_zero(const uint8_t guid[16])
{
    unsigned int i;
    for (i = 0; i < 16; i++)
        if (guid[i])
            return 0;
    return 1;
}

static const struct pa220_manifest_component_policy *find_policy(
    const struct pa220_install_manifest *manifest, const char *name)
{
    uint32_t i;
    for (i = 0; i < manifest->component_count; i++)
        if (name_equal(manifest->components[i].name, name))
            return &manifest->components[i];
    return NULL;
}

static const struct pa220_partition *find_partition(
    const struct pa220_partition_table *table, uint32_t index)
{
    uint32_t i;
    for (i = 0; i < table->count; i++)
        if (table->parts[i].index == index)
            return &table->parts[i];
    return NULL;
}

const char *pa220_plan_status_name(enum pa220_plan_status status)
{
    switch (status) {
    case PA220_PLAN_OK: return "OK";
    case PA220_PLAN_NO_APPROVED_MANIFEST: return "NO_APPROVED_MANIFEST";
    case PA220_PLAN_BAD_MANIFEST: return "BAD_MANIFEST";
    case PA220_PLAN_BAD_PARTITION_TABLE: return "BAD_PARTITION_TABLE";
    case PA220_PLAN_BAD_COMPONENT: return "BAD_COMPONENT";
    case PA220_PLAN_BAD_SELECTION: return "BAD_SELECTION";
    case PA220_PLAN_TARGET_MISMATCH: return "TARGET_MISMATCH";
    case PA220_PLAN_CAPACITY: return "INSUFFICIENT_CAPACITY";
    case PA220_PLAN_OVERLAP: return "OVERLAP";
    default: return "INVALID";
    }
}

enum pa220_plan_status pa220_install_plan_build(
    const struct pa220_install_manifest *manifest,
    const struct pa220_partition_table *partitions,
    uint64_t image_size,
    const struct pa220_plan_component *components,
    uint32_t component_count,
    const struct pa220_target_selection *selections,
    uint32_t selection_count,
    struct pa220_install_plan *out)
{
    uint32_t i, j;
    if (!out)
        return PA220_PLAN_BAD_COMPONENT;
    memset(out, 0, sizeof(*out));
    out->status = PA220_PLAN_BAD_COMPONENT;

    /* No implicit/default policy: absence and unreviewed manifests fail closed. */
    if (!manifest || manifest->state != PA220_MANIFEST_APPROVED) {
        out->status = PA220_PLAN_NO_APPROVED_MANIFEST;
        return out->status;
    }
    if (!manifest->manifest_id[0] ||
        manifest->board_type != PA220_PLAN_BOARD_TYPE ||
        !manifest->component_count ||
        manifest->component_count > PA220_PLAN_MAX_COMPONENTS ||
        !partitions || partitions->status != PA220_PT_OK ||
        (partitions->format != PA220_PT_DOS_MBR &&
         partitions->format != PA220_PT_GPT) ||
        !partitions->count || partitions->count > PA220_PART_MAX) {
        out->status = PA220_PLAN_BAD_MANIFEST;
        return out->status;
    }
    if (!components || !component_count ||
        component_count > PA220_PLAN_MAX_COMPONENTS ||
        component_count != manifest->component_count ||
        !selections || selection_count != component_count ||
        !image_size) {
        out->status = PA220_PLAN_BAD_COMPONENT;
        return out->status;
    }

    for (i = 0; i < component_count; i++) {
        const struct pa220_plan_component *c = &components[i];
        const struct pa220_manifest_component_policy *policy =
            find_policy(manifest, c->name);
        const struct pa220_target_selection *selection = NULL;
        const struct pa220_partition *part;
        uint64_t source_end, required_sectors, target_end;

        if (!c->name[0] || !c->size_bytes || !c->integrity_verified ||
            !c->format_mask || !add_u64(c->source_offset, c->size_bytes,
                                        &source_end) ||
            source_end > image_size || !policy ||
            !(policy->format_mask & c->format_mask) ||
            c->size_bytes < policy->min_bytes ||
            c->size_bytes > policy->max_bytes ||
            !policy->allowed_kind_mask) {
            out->status = PA220_PLAN_BAD_COMPONENT;
            return out->status;
        }

        for (j = 0; j < i; j++) {
            uint64_t prior_end;
            if (!add_u64(components[j].source_offset,
                         components[j].size_bytes, &prior_end) ||
                (c->source_offset < prior_end &&
                 components[j].source_offset < source_end)) {
                out->status = PA220_PLAN_OVERLAP;
                return out->status;
            }
        }

        for (j = 0; j < selection_count; j++)
            if (name_equal(selections[j].component_name, c->name)) {
                if (selection) {
                    out->status = PA220_PLAN_BAD_SELECTION;
                    return out->status;
                }
                selection = &selections[j];
            }
        if (!selection || !selection->partition_index) {
            out->status = PA220_PLAN_BAD_SELECTION;
            return out->status;
        }
        for (j = 0; j < i; j++)
            if (out->items[j].partition_index == selection->partition_index) {
                out->status = PA220_PLAN_BAD_SELECTION;
                return out->status;
            }

        part = find_partition(partitions, selection->partition_index);
        if (!part || part->kind > PA220_PART_GPT ||
            !(policy->allowed_kind_mask & (1U << part->kind))) {
            out->status = PA220_PLAN_TARGET_MISMATCH;
            return out->status;
        }
        if (partitions->format == PA220_PT_DOS_MBR) {
            if (part->kind == PA220_PART_GPT ||
                !policy->allowed_mbr_type ||
                part->type != policy->allowed_mbr_type) {
                out->status = PA220_PLAN_TARGET_MISMATCH;
                return out->status;
            }
        } else if (part->kind != PA220_PART_GPT ||
                   guid_is_zero(policy->gpt_type_guid) ||
                   memcmp(part->type_guid, policy->gpt_type_guid, 16)) {
            out->status = PA220_PLAN_TARGET_MISMATCH;
            return out->status;
        }

        required_sectors = c->size_bytes / PA220_PLAN_SECTOR_SIZE;
        if (c->size_bytes % PA220_PLAN_SECTOR_SIZE) {
            if (required_sectors == UINT64_MAX) {
                out->status = PA220_PLAN_CAPACITY;
                return out->status;
            }
            required_sectors++;
        }
        if (required_sectors > part->sectors ||
            !add_u64(part->start_lba, part->sectors, &target_end) ||
            target_end > partitions->capacity) {
            out->status = PA220_PLAN_CAPACITY;
            return out->status;
        }

        out->items[i].partition_index = part->index;
        out->items[i].target_start_lba = part->start_lba;
        out->items[i].target_sectors = part->sectors;
        out->items[i].required_sectors = required_sectors;
        out->items[i].source_offset = c->source_offset;
        out->items[i].source_size = c->size_bytes;
        out->item_count++;
    }

    out->status = PA220_PLAN_OK;
    return out->status;
}
