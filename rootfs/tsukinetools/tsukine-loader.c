#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/ioctl.h>

/* 和内核 tsukine_abi.h 对齐 */
struct tsukine_req {
    unsigned int win_ssn;
    unsigned int tsuki_sys;
    unsigned int subcmd;
    unsigned char payload[256];
    int result;
};

#define TSUKINE_IOC_MAGIC 'T'
#define TSUKINE_IOC_CALL   _IOWR(TSUKINE_IOC_MAGIC, 1, struct tsukine_req)

static int g_fd = -1;

static void tsukine_send(unsigned int win_ssn, unsigned int sys,
                         unsigned int subcmd, const void *data, int dlen)
{
    struct tsukine_req req = {
        .win_ssn = win_ssn,
        .tsuki_sys = sys,
        .subcmd = subcmd,
    };
    if (dlen > 0 && dlen <= 256)
        memcpy(req.payload, data, dlen);

    if (g_fd < 0) g_fd = open("/dev/tsukine", O_RDWR);
    if (g_fd < 0) { perror("open /dev/tsukine"); return; }

    ioctl(g_fd, TSUKINE_IOC_CALL, &req);
    printf("[loader] win_ssn=0x%04X sys=%u sub=0x%04X -> kernel ret=%d\n",
           win_ssn, sys, subcmd, req.result);
}

/* 模拟 ntdll 桩：WriteFile */
void NtWriteFile_stub(const char *msg)
{
    /* win_ssn 随便先编一个，后面填真实号 */
    tsukine_send(0x1001, 551, 0x0001, msg, strlen(msg)+1);
}

int main(int argc, char **argv)
{
    if (argc < 2) {
        printf("usage: %s app.exe\n", argv[0]);
        return 1;
    }

    int fd = open(argv[1], O_RDONLY);
    if (fd < 0) { perror("open exe"); return 1; }

    /* 读 PE 头 512 字节足够 */
    unsigned char hdr[512];
    read(fd, hdr, sizeof(hdr));
    close(fd);

    if (hdr[0] != 'M' || hdr[1] != 'Z') {
        printf("not a PE file (no MZ)\n");
        return 1;
    }

    /* 找 NT 头 */
    unsigned int e_lfanew = *(unsigned int*)(hdr + 0x3C);
    printf("e_lfanew = 0x%X\n", e_lfanew);
    if (memcmp(hdr + e_lfanew, "PE\0\0", 4) != 0) {
        printf("not PE signature\n");
        return 1;
    }

    printf("PE OK, arch=%d\n",
        *(unsigned short*)(hdr + e_lfanew + 4) == 0x8664 ? 64 : 32);

    /* 假装 exe 里调用了 WriteFile("hello from exe\\n") */
    NtWriteFile_stub("hello from exe\n");

    printf("loader done\n");
    return 0;
}