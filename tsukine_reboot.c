#include <stdio.h>
#include <unistd.h>
#include <sys/reboot.h>

#define CYAN "\x1B[36m"
#define RED "\x1B[31m"
#define RESET "\x1B[0m"

int main() {
    printf(CYAN "       .#####.\n"
           "     ##@@@@@@\n"
           "    #@@@@@@\n"
           "   #@@@@@@\n"
           "   #@@@@@@@\n"
           "    #@@@@@@@\n"
           "     ##@@@@@@@\n"
           "       .#######\n" RESET);
    printf("Syncing disks...\n");
    sync();
    printf(RED "[TsukineOS] Rebooting system...\n" RESET);
    reboot(RB_AUTOBOOT);
    perror("reboot failed");
    return 1;
}
