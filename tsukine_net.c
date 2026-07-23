#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <netinet/ip_icmp.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/stat.h>

#define LOG_FILE "/var/log/tsukine_net.log"
#define PACKET_SIZE 64

// 守护进程化
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

// 获取第一个非 lo 网卡名
void get_primary_interface(char *buf, int buf_size) {
    FILE *fp = fopen("/proc/net/dev", "r");
    if (!fp) return;
    char line[256];
    fgets(line, sizeof(line), fp);
    fgets(line, sizeof(line), fp);
    while (fgets(line, sizeof(line), fp)) {
        char *colon = strchr(line, ':');
        if (colon) {
            *colon = '\0';
            char *name = line;
            while (*name == ' ') name++;
            if (strcmp(name, "lo") != 0) {
                strncpy(buf, name, buf_size - 1);
                buf[buf_size - 1] = '\0';
                fclose(fp);
                return;
            }
        }
    }
    fclose(fp);
}

// 获取当前网卡 IP
void get_interface_ip(const char *iface, char *ip_buf, int buf_size) {
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) return;
    struct ifreq ifr;
    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, iface, IFNAMSIZ);
    if (ioctl(sock, SIOCGIFADDR, &ifr) == 0) {
        struct sockaddr_in *addr = (struct sockaddr_in *)&ifr.ifr_addr;
        strncpy(ip_buf, inet_ntoa(addr->sin_addr), buf_size - 1);
        ip_buf[buf_size - 1] = '\0';
    }
    close(sock);
}

// 简单的 ping 检测（只发一个包，检查是否收到回包）
int ping_check(const char *ip) {
    int sock = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
    if (sock < 0) return 0;

    struct sockaddr_in dest;
    memset(&dest, 0, sizeof(dest));
    dest.sin_family = AF_INET;
    inet_pton(AF_INET, ip, &dest.sin_addr);

    struct icmp icmp_hdr;
    memset(&icmp_hdr, 0, sizeof(icmp_hdr));
    icmp_hdr.icmp_type = ICMP_ECHO;
    icmp_hdr.icmp_id = getpid() & 0xFFFF;
    icmp_hdr.icmp_seq = 1;
    icmp_hdr.icmp_cksum = 0;
    // 简单 checksum
    unsigned short *p = (unsigned short *)&icmp_hdr;
    unsigned int sum = 0;
    for (int i = 0; i < sizeof(icmp_hdr) / 2; i++) sum += p[i];
    sum = (sum >> 16) + (sum & 0xFFFF);
    icmp_hdr.icmp_cksum = ~sum;

    char packet[64];
    memcpy(packet, &icmp_hdr, sizeof(icmp_hdr));

    sendto(sock, packet, sizeof(icmp_hdr), 0, (struct sockaddr *)&dest, sizeof(dest));

    struct timeval tv = {0, 300000};  // 300ms 超时
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(sock, &fds);
    int ret = select(sock + 1, &fds, NULL, NULL, &tv);

    close(sock);
    return (ret > 0);
}

// 自动检测并设置网关
void auto_set_gateway(const char *iface, FILE *log_fp) {
    char ip[16] = {0};
    get_interface_ip(iface, ip, sizeof(ip));
    if (ip[0] == '\0') {
        fprintf(log_fp, "[WARNING] Cannot get IP address for %s\n", iface);
        return;
    }

    char ip_copy[16];
    strncpy(ip_copy, ip, sizeof(ip_copy));
    char *last_dot = strrchr(ip_copy, '.');
    if (!last_dot) return;
    *last_dot = '\0';

    const char *candidates[] = {".1", ".254", ".2", ".253", ".3"};
    char gateway[16];
    char cmd[64];

    for (int i = 0; i < 5; i++) {
        snprintf(gateway, sizeof(gateway), "%s%s", ip_copy, candidates[i]);
        fprintf(log_fp, "[INFO] Testing gateway: %s ...\n", gateway);
        if (ping_check(gateway)) {
            snprintf(cmd, sizeof(cmd), "ip route add default via %s dev %s", gateway, iface);
            system(cmd);
            fprintf(log_fp, "[INFO] Gateway set to: %s\n", gateway);
            return;
        }
    }
    fprintf(log_fp, "[WARNING] No reachable gateway found.\n");
}

int main() {
    become_daemon();

    FILE *log_fp = fopen(LOG_FILE, "w");
    if (!log_fp) return 1;

    fprintf(log_fp, "[TsukineOS Daemon] Network service started.\n");

    char iface[32] = {0};
    get_primary_interface(iface, sizeof(iface));
    if (iface[0] == '\0') {
        fprintf(log_fp, "[ERROR] No network interface found.\n");
        fclose(log_fp);
        return 1;
    }
    fprintf(log_fp, "[INFO] Interface detected: %s\n", iface);

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
        fprintf(log_fp, "[INFO] %s is UP.\n", iface);
    }

    // 设置 IP (QEMU 默认 IP，可在此处替换为动态获取)
    struct sockaddr_in *addr = (struct sockaddr_in *)&ifr.ifr_addr;
    addr->sin_family = AF_INET;
    addr->sin_addr.s_addr = inet_addr("10.0.2.15");
    ioctl(sock, SIOCSIFADDR, &ifr);

    addr->sin_addr.s_addr = inet_addr("255.255.255.0");
    ioctl(sock, SIOCSIFNETMASK, &ifr);

    fprintf(log_fp, "[INFO] IP set to 10.0.2.15\n");
    close(sock);

    // 自动检测并配置网关
    auto_set_gateway(iface, log_fp);

    fprintf(log_fp, "[SUCCESS] Network configuration complete.\n");
    fclose(log_fp);

    while (1) sleep(3600);
    return 0;
}