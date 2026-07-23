#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/stat.h>

#define LOG_FILE "/var/log/tsukine_net.log"

// 守护进程创建函数
void become_daemon() {
    pid_t pid = fork();
    if (pid < 0) exit(EXIT_FAILURE);
    if (pid > 0) exit(EXIT_SUCCESS); 
    if (setsid() < 0) exit(EXIT_FAILURE); 
    chdir("/"); 
    umask(0); 
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);
    open("/dev/null", O_RDONLY);
    open("/dev/null", O_WRONLY);
    open("/dev/null", O_WRONLY);
}

// 【终极修复】纯底层读取，彻底抛弃 strdup！
void get_primary_interface(char *buf, int buf_size) {
    FILE *fp = fopen("/proc/net/dev", "r");
    if (!fp) return;

    char line[256];
    fgets(line, sizeof(line), fp); // 跳过表头1
    fgets(line, sizeof(line), fp); // 跳过表头2

    while (fgets(line, sizeof(line), fp)) {
        char *colon = strchr(line, ':');
        if (colon) {
            *colon = '\0'; 
            char *name = line;
            while (*name == ' ') name++; // 跳过空格
            
            if (strcmp(name, "lo") != 0) {
                strncpy(buf, name, buf_size - 1);
                buf[buf_size - 1] = '\0'; // 确保字符串安全结束
                fclose(fp);
                return;
            }
        }
    }
    fclose(fp);
}

int main() {
    become_daemon();

    FILE *log_fp = fopen(LOG_FILE, "w"); 
    if (!log_fp) return 1;

    fprintf(log_fp, "[TsukineOS Daemon] Network service started.\n");

    // 1. 动态获取网卡 (使用固定数组，绝对安全)
    char iface[32] = {0};
    get_primary_interface(iface, sizeof(iface));

    if (iface[0] == '\0') {
        fprintf(log_fp, "[ERROR] No physical network interface found!\n");
        fclose(log_fp);
        return 1;
    }
    fprintf(log_fp, "[INFO] Primary interface detected: %s\n", iface);

    // 2. 配置网络
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) { 
        fprintf(log_fp, "[ERROR] Socket creation failed.\n"); 
        fclose(log_fp); 
        return 1; 
    }

    struct ifreq ifr;
    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, iface, IFNAMSIZ);

    // 激活网卡
    if (ioctl(sock, SIOCGIFFLAGS, &ifr) == 0) {
        ifr.ifr_flags |= (IFF_UP | IFF_RUNNING);
        ioctl(sock, SIOCSIFFLAGS, &ifr);
        fprintf(log_fp, "[INFO] %s is now UP.\n", iface);
    }

    // 配置 IP
    struct sockaddr_in *addr = (struct sockaddr_in *)&ifr.ifr_addr;
    addr->sin_family = AF_INET;
    addr->sin_addr.s_addr = inet_addr("10.0.2.15");
    ioctl(sock, SIOCSIFADDR, &ifr);

    // 配置掩码
    addr->sin_addr.s_addr = inet_addr("255.255.255.0");
    ioctl(sock, SIOCSIFNETMASK, &ifr);

    fprintf(log_fp, "[INFO] IP assigned to %s: 10.0.2.15\n", iface);
    fprintf(log_fp, "[SUCCESS] Network configuration complete.\n");

    close(sock);
    fclose(log_fp);

    // 守护进程休眠
    while(1) sleep(3600); 

    return 0;
}