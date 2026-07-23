#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/ip_icmp.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <errno.h>

#define PACKET_SIZE 64
#define DEFAULT_COUNT 10 // 默认最多发送 10 次，防止死循环

unsigned short checksum(void *b, int len) {
    unsigned short *buf = b;
    unsigned int sum = 0;
    unsigned short result;
    for (sum = 0; len > 1; len -= 2) sum += *buf++;
    if (len == 1) sum += *(unsigned char *)buf;
    sum = (sum >> 16) + (sum & 0xFFFF);
    sum += (sum >> 16);
    result = ~sum;
    return result;
}

void print_usage() {
    printf("Usage: tsukine_ping [-c count] <IP_ADDRESS>\n");
    printf("  -c count : Number of packets to send (Default: %d)\n", DEFAULT_COUNT);
}

int main(int argc, char *argv[]) {
    int count = DEFAULT_COUNT;
    char *target_ip = NULL;

    // 1. 解析命令行参数
    if (argc < 2) { print_usage(); return 1; }
    
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-c") == 0 && i + 1 < argc) {
            count = atoi(argv[++i]);
            if (count <= 0) count = DEFAULT_COUNT;
        } else {
            target_ip = argv[i];
        }
    }

    if (!target_ip) { print_usage(); return 1; }

    // 2. 创建 Raw Socket
    int sock = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
    if (sock < 0) {
        printf("Error: Failed to create raw socket. (Need root privileges)\n");
        return 1;
    }

    struct sockaddr_in dest_addr;
    memset(&dest_addr, 0, sizeof(dest_addr));
    dest_addr.sin_family = AF_INET;
    if (inet_pton(AF_INET, target_ip, &dest_addr.sin_addr) <= 0) {
        printf("Error: Invalid IP address.\n");
        close(sock);
        return 1;
    }

    printf("PING %s: %d data bytes\n", target_ip, PACKET_SIZE);

    char buffer[PACKET_SIZE];
    struct icmp *icmp = (struct icmp *)buffer;
    struct timeval tv_out, tv_in;
    int seq = 1;
    int received = 0;

    // 3. 发送循环 (受 count 限制，绝对安全)
    while (seq <= count) {
        memset(buffer, 0, PACKET_SIZE);
        icmp->icmp_type = ICMP_ECHO;
        icmp->icmp_code = 0;
        icmp->icmp_id = getpid() & 0xFFFF;
        icmp->icmp_seq = seq;
        gettimeofday(&tv_out, NULL);
        icmp->icmp_cksum = 0;
        icmp->icmp_cksum = checksum(buffer, PACKET_SIZE);

        if (sendto(sock, buffer, PACKET_SIZE, 0, (struct sockaddr *)&dest_addr, sizeof(dest_addr)) < 0) {
            printf("Send failed: %s\n", strerror(errno));
        } else {
            struct sockaddr_in src_addr;
            socklen_t addr_len = sizeof(src_addr);
            fd_set readfds;
            struct timeval timeout = {1, 0}; // 1秒超时

            FD_ZERO(&readfds);
            FD_SET(sock, &readfds);
            if (select(sock + 1, &readfds, NULL, NULL, &timeout) > 0) {
                if (recvfrom(sock, buffer, PACKET_SIZE, 0, (struct sockaddr *)&src_addr, &addr_len) > 0) {
                    gettimeofday(&tv_in, NULL);
                    double rtt = (tv_in.tv_sec - tv_out.tv_sec) * 1000.0 + 
                                 (tv_in.tv_usec - tv_out.tv_usec) / 1000.0;
                    printf("%d bytes from %s: icmp_seq=%d time=%.2f ms\n", 
                           PACKET_SIZE, inet_ntoa(src_addr.sin_addr), icmp->icmp_seq, rtt);
                    received++;
                }
            } else {
                printf("Request timeout for icmp_seq %d\n", seq);
            }
        }
        seq++;
        if (seq <= count) sleep(1); 
    }

    // 4. 打印统计信息并优雅退出
    printf("\n--- %s ping statistics ---\n", target_ip);
    printf("%d packets transmitted, %d received, %.0f%% packet loss\n", 
           count, received, ((float)(count - received) / count) * 100.0);
    
    close(sock);
    return 0;
}