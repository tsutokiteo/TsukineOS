#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <unistd.h>

int main() {
    int sock;
    struct ifreq ifr;

    // 1. 创建一个 Socket
    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) { perror("socket"); return 1; }

    // 2. 设置目标网卡为 eth0
    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, "eth0", IFNAMSIZ);

    // 3. 获取当前网卡标志
    if (ioctl(sock, SIOCGIFFLAGS, &ifr) < 0) { perror("SIOCGIFFLAGS"); return 1; }

    // 4. 添加 IFF_UP 和 IFF_RUNNING 标志，激活网卡
    ifr.ifr_flags |= (IFF_UP | IFF_RUNNING);
    if (ioctl(sock, SIOCSIFFLAGS, &ifr) < 0) { perror("SIOCSIFFLAGS"); return 1; }
    
    printf("[TsukineOS] eth0 is now UP!\n");

    // 5. 手动配置 IP 地址为 10.0.2.15 (QEMU user 网络的默认 IP)
    struct sockaddr_in *addr = (struct sockaddr_in *)&ifr.ifr_addr;
    addr->sin_family = AF_INET;
    addr->sin_addr.s_addr = inet_addr("10.0.2.15");
    if (ioctl(sock, SIOCSIFADDR, &ifr) < 0) { perror("SIOCSIFADDR"); return 1; }

    // 6. 设置子网掩码
    addr->sin_addr.s_addr = inet_addr("255.255.255.0");
    if (ioctl(sock, SIOCSIFNETMASK, &ifr) < 0) { perror("SIOCSIFNETMASK"); return 1; }

    printf("[TsukineOS] IP assigned: 10.0.2.15\n");
    close(sock);
    return 0;
}
