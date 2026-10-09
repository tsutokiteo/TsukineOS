#include <netinet/ip_icmp.h>
#include <netinet/ip.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/stat.h>

#define LOG_FILE "/var/log/tsukine_net.log"
#define CONFIG_FILE "/etc/network/interfaces"

FILE *log_fp;

void log_msg(const char *msg) {
    if (log_fp) fprintf(log_fp, "[%s] %s\n", __func__, msg);
}

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
    unsigned short *p = (unsigned short *)&icmp_hdr;
    unsigned int sum = 0;
    for (int i = 0; i < sizeof(icmp_hdr) / 2; i++) sum += p[i];
    sum = (sum >> 16) + (sum & 0xFFFF);
    icmp_hdr.icmp_cksum = ~sum;
    char packet[64];
    memcpy(packet, &icmp_hdr, sizeof(icmp_hdr));
    sendto(sock, packet, sizeof(icmp_hdr), 0, (struct sockaddr *)&dest, sizeof(dest));
    struct timeval tv = {0, 300000};
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(sock, &fds);
    int ret = select(sock + 1, &fds, NULL, NULL, &tv);
    close(sock);
    return (ret > 0);
}

void auto_set_gateway(const char *iface, const char *ip) {
    char ip_copy[16];
    strncpy(ip_copy, ip, sizeof(ip_copy));
    char *last_dot = strrchr(ip_copy, '.');
    if (!last_dot) return;
    *last_dot = '\0';
    const char *candidates[] = {".1", ".254", ".2", ".253", ".3"};
    char gateway[16], cmd[64];
    for (int i = 0; i < 5; i++) {
        snprintf(gateway, sizeof(gateway), "%s%s", ip_copy, candidates[i]);
        if (ping_check(gateway)) {
            snprintf(cmd, sizeof(cmd), "ip route add default via %s dev %s", gateway, iface);
            system(cmd);
            log_msg(gateway);
            return;
        }
    }
}

int parse_config(const char *iface, char *mode, char *ip, char *mask, char *gw) {
    FILE *fp = fopen(CONFIG_FILE, "r");
    if (!fp) return 0;
    char line[256];
    int found = 0;
    while (fgets(line, sizeof(line), fp)) {
        if (strstr(line, iface) && strstr(line, "inet")) {
            found = 1;
            if (strstr(line, "dhcp")) {
                strcpy(mode, "dhcp");
                fclose(fp);
                return 1;
            }
            if (strstr(line, "static")) {
                strcpy(mode, "static");
                while (fgets(line, sizeof(line), fp)) {
                    if (strstr(line, "address")) sscanf(line, "%*s %s", ip);
                    if (strstr(line, "netmask")) sscanf(line, "%*s %s", mask);
                    if (strstr(line, "gateway")) sscanf(line, "%*s %s", gw);
                    if (strchr(line, '}') || strchr(line, '\n') == line) break;
                }
                fclose(fp);
                return 1;
            }
        }
    }
    fclose(fp);
    return 0;
}

int main() {
    become_daemon();
    log_fp = fopen(LOG_FILE, "w");
    if (!log_fp) return 1;
    log_msg("Network service started");

    char iface[32] = {0};
    get_primary_interface(iface, sizeof(iface));
    if (iface[0] == '\0') {
        log_msg("ERROR: No interface found");
        fclose(log_fp);
        return 1;
    }
    log_msg(iface);

    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        log_msg("ERROR: socket");
        fclose(log_fp);
        return 1;
    }

    struct ifreq ifr;
    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, iface, IFNAMSIZ);

    if (ioctl(sock, SIOCGIFFLAGS, &ifr) == 0) {
        ifr.ifr_flags |= (IFF_UP | IFF_RUNNING);
        ioctl(sock, SIOCSIFFLAGS, &ifr);
    }
    close(sock);

    char mode[16] = {0}, ip[16] = {0}, mask[16] = {0}, gw[16] = {0};

    if (parse_config(iface, mode, ip, mask, gw)) {
        if (strcmp(mode, "dhcp") == 0) {
            char cmd[128];
            snprintf(cmd, sizeof(cmd), "udhcpc -i %s -s /etc/udhcpc.script", iface);
            system(cmd);
            log_msg("DHCP mode");
        } else if (strcmp(mode, "static") == 0) {
            char cmd[128];
            snprintf(cmd, sizeof(cmd), "ip addr add %s/%s dev %s", ip, mask, iface);
            system(cmd);
            if (strlen(gw) > 0) {
                snprintf(cmd, sizeof(cmd), "ip route add default via %s dev %s", gw, iface);
                system(cmd);
            }
            log_msg("Static mode");
        }
    } else {
        // Fallback: 硬编码 + 自动网关探测
        char cmd[128];
        snprintf(cmd, sizeof(cmd), "ip addr add 10.0.2.15/24 dev %s", iface);
        system(cmd);
        auto_set_gateway(iface, "10.0.2.15");
        log_msg("Fallback mode (10.0.2.15 + auto gateway)");
    }

    log_msg("Network configuration done");
    fclose(log_fp);

    while(1) sleep(3600);
    return 0;
}