#pragma once
#include <stdint.h>

#define DEL_MAGIC      0x4C454444
#define DEL_FORMAT_VER 1

#define DEL_OP_ADD        0x01
#define DEL_OP_REMOVE     0x02
#define DEL_OP_MODIFY     0x03
#define DEL_OP_RENAME     0x04
#define DEL_OP_DEPRECATE  0x05
#define DEL_OP_REPLACE    0x06
#define DEL_OP_BULK_REMOVE 0x07
#define DEL_OP_BULK_ADD   0x08
#define DEL_OP_SECTION    0x09
#define DEL_OP_NOTE       0x0A
#define DEL_OP_REQUIRE    0x0B

#define DEL_F_COMPAT     0x01

typedef struct {
    uint32_t magic;
    uint32_t format_ver;
    uint32_t abi_ver;
    uint32_t entry_count;
    uint32_t entry_off;
    uint32_t string_off;
    uint32_t string_size;
} del_header_t;

typedef struct {
    uint32_t abi_nr;
    uint8_t  op;
    uint8_t  flags;
    uint16_t _reserved;
    uint32_t name_off;
    uint32_t size;
} del_entry_t;