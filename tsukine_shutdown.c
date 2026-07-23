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
    printf(RED "[TsukineOS] Powering off...\n" RESET);
    reboot(RB_POWER_OFF);
    perror("shutdown failed");
    return 1;
}
