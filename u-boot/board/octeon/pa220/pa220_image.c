/* SPDX-License-Identifier: GPL-2.0+ */
#include <stdint.h>
#include <stddef.h>
#ifdef PA220_IMAGE_HOST_TEST
#include <string.h>
#else
#include <common.h>
#endif
#include "pa220_image.h"

#define PA220_BOARD_TYPE 20021U
#define OCTEON_BOOT_MAGIC 0x424f4f54U
#define OCTEON_HEADER_MIN 192U
#define OCTEON_HEADER_MAX 0x200U
#define OCTEON_IMAGE_MAX 0x200000U
#define UIMAGE_MAGIC 0x27051956U
#define PA220_FDT_MAGIC 0xd00dfeedU
#define ELF_PT_LOAD 1U
#define ELF_EM_MIPS 8U

static uint16_t rd16(const uint8_t *p, int le)
{
    return le ? (uint16_t)p[0] | ((uint16_t)p[1] << 8) :
        ((uint16_t)p[0] << 8) | p[1];
}
static uint32_t rd32(const uint8_t *p, int le)
{
    if (le) return (uint32_t)p[0] | ((uint32_t)p[1] << 8) |
        ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
    return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) |
        ((uint32_t)p[2] << 8) | p[3];
}
static uint64_t rd64(const uint8_t *p, int le)
{
    uint64_t hi = le ? rd32(p + 4, 1) : rd32(p, 0);
    uint64_t lo = le ? rd32(p, 1) : rd32(p + 4, 0);
    return (hi << 32) | lo;
}
static uint32_t crc32_ieee(uint32_t crc, const uint8_t *p, uint64_t len)
{
    uint64_t i;
    unsigned int bit;
    crc ^= 0xffffffffU;
    for (i = 0; i < len; i++) {
        crc ^= p[i];
        for (bit = 0; bit < 8; bit++)
            crc = (crc >> 1) ^ ((crc & 1U) ? 0xedb88320U : 0U);
    }
    return crc ^ 0xffffffffU;
}
static int range_ok(uint64_t off, uint64_t len, uint64_t total)
{
    return off <= total && len <= total - off;
}
static int add_ok(uint64_t a, uint64_t b)
{
    return b <= UINT64_MAX - a;
}
const char *pa220_image_format_name(enum pa220_image_format f)
{
    switch (f) {
    case PA220_IMAGE_OCTEON_BOOTLOADER: return "OCTEON bootloader";
    case PA220_IMAGE_LEGACY_UIMAGE: return "legacy uImage";
    case PA220_IMAGE_FIT_FDT: return "FIT/FDT";
    case PA220_IMAGE_ELF32: return "ELF32";
    case PA220_IMAGE_ELF64: return "ELF64";
    default: return "unsupported";
    }
}
const char *pa220_image_type_name(uint32_t t)
{
    switch (t) {
    case 1: return "Stage 2";
    case 2: return "Stage 3";
    case 3: return "NOR bootloader";
    case 4: return "PCI bootloader";
    case 5: return "U-Boot environment";
    case 6: return "Pre-U-Boot";
    case 7: return "Stage 1";
    default: return "unknown";
    }
}
static int parse_octeon(const uint8_t *p, uint64_t n, struct pa220_image_info *o)
{
    uint32_t hcrc, dcrc, crc, dlen;
    uint16_t hlen, major, minor, board, type;
    uint64_t total;
    if (n < OCTEON_HEADER_MIN || rd32(p + 8, 0) != OCTEON_BOOT_MAGIC) return -1;
    hlen = rd16(p + 16, 0); major = rd16(p + 18, 0);
    minor = rd16(p + 20, 0); board = rd16(p + 22, 0);
    dlen = rd32(p + 24, 0); dcrc = rd32(p + 28, 0);
    hcrc = rd32(p + 12, 0); type = rd16(p + 44, 0);
    if (rd32(p + 4, 0) != 0 || hlen < OCTEON_HEADER_MIN ||
        hlen > OCTEON_HEADER_MAX || hlen > n || major != 1 || minor < 1 ||
        dlen > n - hlen || !add_ok(hlen, dlen)) return -1;
    total = (uint64_t)hlen + dlen;
    if (total != n || total > OCTEON_IMAGE_MAX || type < 1 || type > 7) return -1;
    crc = crc32_ieee(0, p, 12);
    crc = crc32_ieee(crc, p + 16, hlen - 16);
    if (crc != hcrc || crc32_ieee(0, p + hlen, dlen) != dcrc) return -1;
    memset(o, 0, sizeof(*o));
    o->format = PA220_IMAGE_OCTEON_BOOTLOADER;
    o->total_size = n; o->header_size = hlen; o->payload_size = dlen;
    o->header_crc = hcrc; o->data_crc = dcrc; o->crc_valid = 1;
    o->board_type = board; o->board_compatible = (board == 0 || board == PA220_BOARD_TYPE);
    o->image_type = type; o->image_flags = rd32(p + 40, 0);
    o->load_address = rd64(p + 32, 0);
    return 0;
}
static int parse_uimage(const uint8_t *p, uint64_t n, struct pa220_image_info *o)
{
    uint8_t hdr[64];
    uint32_t size, hcrc, dcrc;
    if (n < 64 || rd32(p, 0) != UIMAGE_MAGIC) return -1;
    size = rd32(p + 12, 0); hcrc = rd32(p + 4, 0); dcrc = rd32(p + 24, 0);
    if ((uint64_t)size != n - 64) return -1;
    memcpy(hdr, p, sizeof(hdr)); memset(hdr + 4, 0, 4);
    if (crc32_ieee(0, hdr, sizeof(hdr)) != hcrc ||
        crc32_ieee(0, p + 64, size) != dcrc) return -1;
    memset(o, 0, sizeof(*o));
    o->format = PA220_IMAGE_LEGACY_UIMAGE; o->total_size = n;
    o->header_size = 64; o->payload_size = size;
    o->header_crc = hcrc; o->data_crc = dcrc; o->crc_valid = 1;
    o->load_address = rd32(p + 16, 0); o->entry_address = rd32(p + 20, 0);
    o->architecture_compatible = (p[29] == 5 || p[29] == 6);
    o->endian_big = 1; o->image_type = p[30];
    return 0;
}
static int parse_fdt(const uint8_t *p, uint64_t n, struct pa220_image_info *o)
{
    uint32_t total, so, sto, ro, ss, sz, ver, last;
    if (n < 40 || rd32(p, 0) != PA220_FDT_MAGIC) return -1;
    total = rd32(p + 4, 0); so = rd32(p + 8, 0); sto = rd32(p + 12, 0);
    ro = rd32(p + 16, 0); ver = rd32(p + 20, 0); last = rd32(p + 24, 0);
    ss = rd32(p + 32, 0); sz = rd32(p + 36, 0);
    if (total < 40 || total > n || ver < 16 || ver > 17 ||
        last < 16 || last > ver || (ro & 7) || ro < 40 ||
        !range_ok(ro, 16, total) || so < 40 || sto < 40 ||
        !range_ok(so, sz, total) || !range_ok(sto, ss, total)) return -1;
    memset(o, 0, sizeof(*o)); o->format = PA220_IMAGE_FIT_FDT;
    o->total_size = n; o->header_size = 40; o->payload_size = total - 40;
    return 0;
}
static int parse_elf(const uint8_t *p, uint64_t n, struct pa220_image_info *o)
{
    int cls, little;
    uint16_t type, machine, ehsize, phentsize, phnum, shentsize, shnum;
    uint64_t entry, phoff, shoff, i;
    uint32_t count = 0;
    if (n < 16 || p[0] != 0x7f || p[1] != 'E' || p[2] != 'L' || p[3] != 'F') return -1;
    cls = p[4]; if ((cls != 1 && cls != 2) || (p[5] != 1 && p[5] != 2) || p[6] != 1) return -1;
    little = p[5] == 1;
    if (cls == 1) {
        if (n < 52) return -1;
        type=rd16(p+16,little); machine=rd16(p+18,little); entry=rd32(p+24,little);
        phoff=rd32(p+28,little); shoff=rd32(p+32,little); ehsize=rd16(p+40,little);
        phentsize=rd16(p+42,little); phnum=rd16(p+44,little); shentsize=rd16(p+46,little); shnum=rd16(p+48,little);
        if (ehsize != 52 || (phnum && phentsize < 32) || (shnum && shentsize < 40)) return -1;
    } else {
        if (n < 64) return -1;
        type=rd16(p+16,little); machine=rd16(p+18,little); entry=rd64(p+24,little);
        phoff=rd64(p+32,little); shoff=rd64(p+40,little); ehsize=rd16(p+52,little);
        phentsize=rd16(p+54,little); phnum=rd16(p+56,little); shentsize=rd16(p+58,little); shnum=rd16(p+60,little);
        if (ehsize != 64 || (phnum && phentsize < 56) || (shnum && shentsize < 64)) return -1;
    }
    if (rd32(p+20,little) != 1 || phnum == 0xffff || shnum == 0xffff) return -1;
    if (phnum && !range_ok(phoff, (uint64_t)phentsize * phnum, n)) return -1;
    if (!phnum && phoff > n) return -1;
    if (shnum && !range_ok(shoff, (uint64_t)shentsize * shnum, n)) return -1;
    if (!shnum && shoff > n) return -1;
    memset(o, 0, sizeof(*o)); o->format = cls == 1 ? PA220_IMAGE_ELF32 : PA220_IMAGE_ELF64;
    o->total_size=n; o->header_size=ehsize; o->payload_size=n-ehsize;
    o->entry_address=entry; o->elf_machine=machine; o->elf_type=type;
    o->architecture_compatible=(machine==ELF_EM_MIPS && !little); o->endian_big=!little;
    for (i=0; i<phnum; i++) {
        const uint8_t *q=p+phoff+i*phentsize;
        uint32_t pt=rd32(q,little), flags;
        uint64_t off, load, filesz, memsz;
        if (cls==1) { off=rd32(q+4,little); load=rd32(q+12,little); filesz=rd32(q+16,little); memsz=rd32(q+20,little); flags=rd32(q+24,little); }
        else { flags=rd32(q+4,little); off=rd64(q+8,little); load=rd64(q+24,little); filesz=rd64(q+32,little); memsz=rd64(q+40,little); }
        if (pt != ELF_PT_LOAD) continue;
        if (filesz > memsz || !range_ok(off, filesz, n) || !add_ok(load, memsz)) return -1;
        if (count < PA220_IMAGE_MAX_SEGMENTS) {
            struct pa220_image_segment *seg=&o->segments[count];
            seg->file_offset=off; seg->load_address=load; seg->file_size=filesz; seg->memory_size=memsz; seg->flags=flags;
            o->stored_segment_count++;
        }
        count++;
    }
    o->segment_count=count;
    return 0;
}
int pa220_image_parse(const uint8_t *data, uint64_t size, struct pa220_image_info *info)
{
    if (!data || !info || !size) return -1;
    memset(info, 0, sizeof(*info));
    if (size >= 12 && rd32(data + 8, 0) == OCTEON_BOOT_MAGIC) return parse_octeon(data, size, info);
    if (size >= 4 && rd32(data, 0) == UIMAGE_MAGIC) return parse_uimage(data, size, info);
    if (size >= 4 && rd32(data, 0) == PA220_FDT_MAGIC) return parse_fdt(data, size, info);
    if (size >= 4 && data[0] == 0x7f && data[1]=='E' && data[2]=='L' && data[3]=='F') return parse_elf(data, size, info);
    return -1;
}
#ifndef PA220_IMAGE_HOST_TEST
#include <environment.h>
#include <command.h>
#include <asm/arch/octeon_load_range.h>
int pa220_image_preview(void)
{
    ulong addr = getenv_hex("fileaddr", 0), size = getenv_hex("filesize", 0);
    struct pa220_image_info info;
    uint32_t i;
    if (!addr || !size) { puts("pa220 image: no completed fileaddr/filesize metadata\n"); return 1; }
    if (size > (~0UL - addr) || octeon_load_range_check((uint64_t)addr, (uint64_t)size)) {
        puts("pa220 image: file range is outside the validated __tmp_load reservation\n"); return 1;
    }
    flush_dcache_range(addr, addr + size);
    if (pa220_image_parse((const uint8_t *)(uintptr_t)addr, size, &info)) {
        puts("pa220 image: unsupported or malformed image; no plan generated\n"); return 1;
    }
    printf("PA-220 image preview (read-only)\nformat: %s\nsize: 0x%llx (%llu bytes)\n",
           pa220_image_format_name(info.format), (unsigned long long)info.total_size, (unsigned long long)info.total_size);
    if (info.format == PA220_IMAGE_OCTEON_BOOTLOADER) {
        printf("header: 0x%llx payload: 0x%llx\nboard_type: %u (%s)\nimage_type: %s (%u)\nlink_address: 0x%llx\nheader_crc/data_crc: PASS\n",
               (unsigned long long)info.header_size, (unsigned long long)info.payload_size, info.board_type,
               info.board_compatible ? "compatible" : "NOT compatible", pa220_image_type_name(info.image_type), info.image_type,
               (unsigned long long)info.load_address);
    } else if (info.format == PA220_IMAGE_LEGACY_UIMAGE) {
        printf("header: 64 payload: 0x%llx load: 0x%llx entry: 0x%llx\narchitecture: %s\nheader_crc/data_crc: PASS\n",
               (unsigned long long)info.payload_size, (unsigned long long)info.load_address,
               (unsigned long long)info.entry_address, info.architecture_compatible ? "MIPS candidate" : "not PA-220 MIPS");
    } else if (info.format == PA220_IMAGE_FIT_FDT) {
        puts("FDT bounds: PASS; FIT configuration/subimage semantics: NOT YET ANALYZED\n");
    } else {
        printf("ELF type: %u machine: %u entry: 0x%llx\narchitecture: %s\nPT_LOAD segments: %u\n",
               info.elf_type, info.elf_machine, (unsigned long long)info.entry_address,
               info.architecture_compatible ? "MIPS big-endian candidate" : "not PA-220 MIPS big-endian",
               info.segment_count);
        for (i=0; i<info.stored_segment_count; i++)
            printf("  [%u] load=0x%llx file=0x%llx mem=0x%llx offset=0x%llx flags=0x%x\n", i,
                   (unsigned long long)info.segments[i].load_address,
                   (unsigned long long)info.segments[i].file_size,
                   (unsigned long long)info.segments[i].memory_size,
                   (unsigned long long)info.segments[i].file_offset, info.segments[i].flags);
        if (info.segment_count > info.stored_segment_count) puts("  additional segments omitted\n");
    }
    puts("target mapping: UNMAPPED (no approved PA-220 install manifest)\n");
    puts("actions: NONE; no device/partition writes, image load or execution performed\n");
    if (info.format == PA220_IMAGE_OCTEON_BOOTLOADER && !info.board_compatible)
        return 1;
    if ((info.format == PA220_IMAGE_LEGACY_UIMAGE ||
         info.format == PA220_IMAGE_ELF32 || info.format == PA220_IMAGE_ELF64) &&
        !info.architecture_compatible)
        return 1;
    return 0;
}
#endif
