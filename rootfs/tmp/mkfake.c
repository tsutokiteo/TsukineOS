#include <stdio.h>
int main() {
    FILE *f = fopen("/tmp/hello.exe", "wb");
    unsigned char mz[64] = {'M','Z',0x90,0x00};
    fwrite(mz, 1, 64, f);
    /* e_lfanew at offset 0x3C = 0x80 */
    fseek(f, 0x3C, SEEK_SET);
    unsigned int lfanew = 0x80;
    fwrite(&lfanew, 4, 1, f);
    /* PE sig at 0x80 */
    fseek(f, 0x80, SEEK_SET);
    fwrite("PE\0\0", 4, 1, f);
    /* machine = 0x8664 (x64) */
    unsigned short machine = 0x8664;
    fwrite(&machine, 2, 1, f);
    fclose(f);
    return 0;
}