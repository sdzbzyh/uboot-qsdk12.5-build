#ifndef TEST_COMMON_H
#define TEST_COMMON_H
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <arpa/inet.h>
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef int32_t s32;
typedef uint8_t uchar;
typedef uint16_t ushort;
typedef uint32_t uint;
typedef uint32_t IPaddr_t;
#define __aligned(x) __attribute__((aligned(x)))
#define PKTALIGN 8
#endif
