/*
 * loader.c — TsukiNetaOS .del loader
 *
 * 用法:
 *   ./loader syscall.del          # 安静模式
 *   ./loader -v syscall.del       # 详细模式，dump 全表
 */

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>

#include "tsukine_abi.h"
#include "mkdel_compat.h"

/* === 日志分级 === */
static int g_verbose = 0;

#define LOG_ERR(...)   fprintf(stderr, __VA_ARGS__)
#define LOG_INFO(...)  fprintf(stderr, __VA_ARGS__)
#define LOG_DBG(...)   do { if (g_verbose) fprintf(stderr, __VA_ARGS__); } while(0)

/* === .del 文件头 === */
struct del_header {
    uint32_t magic;       /* 0x4C454444 */
    uint16_t abi_ver;
    uint16_t entry_count;
    uint32_t flags;
} __attribute__((packed));

struct del_entry {
    uint32_t name_off;
    uint16_t op;
    uint16_t flags;
    uint32_t size;
} __attribute__((packed));

/* === 内建 tsukine 原生 ABI（从 .del 解析） === */
static struct {
    const char *name;
    uint16_t    abi_nr;
    uint8_t     flags;
} native_entries[64];
static int native_count = 0;

/* === dispatch 桩 === */
static int dispatch_win_call(uint16_t win_ssn)
{
    for (size_t i = 0; i < compat_map_count; i++) {
        if (compat_map[i].win_ssn == win_ssn) {
            LOG_DBG("dispatch: ssn=0x%04X -> sys=%u sub=0x%04X\n",
                    win_ssn, compat_map[i].sys_nr, compat_map[i].subcmd);
            return -1; /* kernel handler not implemented yet */
        }
    }
    return -2;
}

static int parse_del(const char *path)
{
    int fd = open(path, O_RDONLY);
    if (fd < 0) {
        perror("open");
        return -1;
    }

    struct stat st;
    if (fstat(fd, &st) < 0) {
        perror("fstat");
        close(fd);
        return -1;
    }

    void *map = mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (map == MAP_FAILED) {
        perror("mmap");
        close(fd);
        return -1;
    }

    if (st.st_size < sizeof(struct del_header)) {
        LOG_ERR("loader: file too small\n");
        munmap(map, st.st_size);
        close(fd);
        return -1;
    }

    struct del_header *hdr = (struct del_header *)map;

    if (hdr->magic != 0x4C454444) {
        LOG_ERR("loader: bad magic 0x%08X (expected 0x4C454444)\n", hdr->magic);
        munmap(map, st.st_size);
        close(fd);
        return -1;
    }

    LOG_INFO("loader: .del loaded ===\n");
    LOG_INFO("Magic: 0x%08X | ABI ver: %u | Entries: %u\n",
             hdr->magic, hdr->abi_ver, hdr->entry_count);

    char *strpool = (char *)map + sizeof(struct del_header)
                  + hdr->entry_count * sizeof(struct del_entry);

    for (uint32_t i = 0; i < hdr->entry_count; i++) {
        struct del_entry *e = (struct del_entry *)
            ((char *)map + sizeof(struct del_header) + i * sizeof(struct del_entry));

        LOG_DBG("DEBUG loader: [%u] raw name_off=%u, op=%u, flags=%u, size=%u\n",
                i, e->name_off, e->op, e->flags, e->size);

        const char *name = NULL;
        if (e->name_off != 0)
            name = strpool + e->name_off;

        if (name)
            LOG_DBG("DEBUG loader: name resolves to \"%s\"\n", name);

        switch (e->op) {
        case 1:
            if (native_count < 64 && name) {
                native_entries[native_count++] = (typeof(native_entries[0])){
                    .name   = name,
                    .abi_nr = (uint16_t)e->size,
                    .flags  = (uint8_t)e->flags,
                };
                LOG_DBG("  [ADD] abi_nr=0x%04X flags=0x%02X name=%s\n",
                        (uint16_t)e->size, e->flags, name);
            }
            break;

        case 10:
            LOG_DBG("  [NOTE] abi_nr=0x%04X flags=0x%02X\n", e->name_off, e->flags);
            if (name)
                LOG_DBG("    msg: \"%s\"\n", name);
            break;

        case 11:
            LOG_DBG("  [REQUIRE] abi_nr=0x%04X flags=0x%02X\n", e->name_off, e->flags);
            LOG_DBG("    requires abi_ver >= %u\n", e->size);
            break;

        default:
            LOG_DBG("  [UNK] op=%u\n", e->op);
            break;
        }
    }

    munmap(map, st.st_size);
    close(fd);
    return 0;
}

int main(int argc, char *argv[])
{
    /* 解析 -v / --verbose */
    {
        int new_argc = 1;
        for (int i = 1; i < argc; i++) {
            if (!strcmp(argv[i], "-v") || !strcmp(argv[i], "--verbose")) {
                g_verbose = 1;
            } else {
                argv[new_argc++] = argv[i];
            }
        }
        argc = new_argc;
    }

    if (argc < 2) {
        LOG_ERR("usage: %s [-v] <file.del>\n", argv[0]);
        return 1;
    }

    if (parse_del(argv[1]) < 0)
        return 1;

    /* 摘要 */
    {
        int stub_count = 0;
        for (size_t i = 0; i < compat_map_count; i++)
            if (compat_map[i].subcmd == TSUKI_SUB_UNIMPLEMENTED)
                stub_count++;

        printf("loader: OK | compat=%zu total, %zu implemented, %d stub\n",
               compat_map_count, compat_map_count - (size_t)stub_count, stub_count);
    }

    /* 详细模式：dump 全表 */
    if (g_verbose) {
        printf("--- Compat table (%zu entries) ---\n", compat_map_count);
        for (size_t i = 0; i < compat_map_count; i++)
            printf("  %-48s win_ssn=0x%04X -> sys=%u sub=0x%04X\n",
                   compat_map[i].name ? compat_map[i].name : "(null)",
                   compat_map[i].win_ssn,
                   compat_map[i].sys_nr,
                   compat_map[i].subcmd);
    }

    /* Dispatch 测试 */
    printf("Dispatch test (WIN_NtClose=0x000C): ret=%d (kernel handler not implemented yet, expected)\n",
           dispatch_win_call(0x000C));

    return 0;
}