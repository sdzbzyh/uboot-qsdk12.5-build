/* SPDX-License-Identifier: GPL-2.0+ */
#ifndef __PA220_IMAGE_H__
#define __PA220_IMAGE_H__
#include <stdint.h>
#define PA220_IMAGE_MAX_SEGMENTS 8
enum pa220_image_format {
    PA220_IMAGE_UNKNOWN = 0, PA220_IMAGE_OCTEON_BOOTLOADER,
    PA220_IMAGE_LEGACY_UIMAGE, PA220_IMAGE_FIT_FDT,
    PA220_IMAGE_ELF32, PA220_IMAGE_ELF64
};
struct pa220_image_segment {
    uint64_t file_offset, load_address, file_size, memory_size;
    uint32_t flags;
};
struct pa220_image_info {
    enum pa220_image_format format;
    uint64_t total_size, header_size, payload_size, load_address, entry_address;
    uint32_t header_crc, data_crc, board_type, image_type, image_flags;
    uint32_t elf_machine, elf_type, segment_count, stored_segment_count;
    uint8_t crc_valid, board_compatible, architecture_compatible, endian_big;
    struct pa220_image_segment segments[PA220_IMAGE_MAX_SEGMENTS];
};
int pa220_image_parse(const uint8_t *data, uint64_t size, struct pa220_image_info *info);
const char *pa220_image_format_name(enum pa220_image_format format);
const char *pa220_image_type_name(uint32_t image_type);
#ifndef PA220_IMAGE_HOST_TEST
int pa220_image_preview(void);
#endif
#endif
