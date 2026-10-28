#include <stdio.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <errno.h>

#ifndef __NR_tsukine_gui
#define __NR_tsukine_gui 550
#endif

int main(void)
{
    long ret;
    printf("Calling tsukine_gui(CREATE_WINDOW)\n");
    ret = syscall(__NR_tsukine_gui, 0x1001, (unsigned long)"Hello", 800, 600, 0, 0);
    if (ret < 0)
        perror("syscall failed");
    else
        printf("OK: syscall returned %ld\n", ret);
    return 0;
}
