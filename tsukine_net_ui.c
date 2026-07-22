#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <termios.h>
#include <fcntl.h>
#include <time.h>

// UI 颜色定义
#define CYAN "\x1B[36m"
#define GREEN "\x1B[32m"
#define YELLOW "\x1B[33m"
#define RED "\x1B[31m"
#define RESET "\x1B[0m"

// 格式化网速
void format_speed(unsigned long long bytes_per_sec, char *buf) {
    if (bytes_per_sec > 1048576) sprintf(buf, "%.2f MB/s", (double)bytes_per_sec / 1048576.0);
    else if (bytes_per_sec > 1024) sprintf(buf, "%.2f KB/s", (double)bytes_per_sec / 1024.0);
    else sprintf(buf, "%llu B/s", bytes_per_sec);
}

// 开启终端原始模式 (非阻塞读取按键)
void enable_raw_mode(struct termios *orig_termios) {
    struct termios raw;
    tcgetattr(STDIN_FILENO, orig_termios);
    raw = *orig_termios;
    raw.c_lflag &= ~(ECHO | ICANON); // 关闭回显和行缓冲
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
    
    // 设置 stdin 为非阻塞
    int flags = fcntl(STDIN_FILENO, F_GETFL, 0);
    fcntl(STDIN_FILENO, F_SETFL, flags | O_NONBLOCK);
}

// 恢复终端正常模式
void disable_raw_mode(struct termios *orig_termios) {
    tcsetattr(STDIN_FILENO, TCSAFLUSH, orig_termios);
}

int main() {
    struct termios orig_termios;
    enable_raw_mode(&orig_termios); // 开启原始模式

    FILE *fp;
    char line[256], iface[32];
    unsigned long long rx_bytes, tx_bytes, rx_packets, tx_packets;
    unsigned long long prev_rx = 0, prev_tx = 0;
    char rx_speed[32], tx_speed[32];
    char c;

    // 启动时清屏
    printf("\033[H\033[J"); 

    while (1) {
        // 1. 非阻塞读取按键
        if (read(STDIN_FILENO, &c, 1) == 1) {
            // 只要按下任意键 (或者特指 'q')，就退出
            break; 
        }

        // 2. 读取 /proc/net/dev
        fp = fopen("/proc/net/dev", "r");
        if (!fp) break;
        fgets(line, sizeof(line), fp); fgets(line, sizeof(line), fp); // 跳过表头
        while (fgets(line, sizeof(line), fp)) {
            if (strstr(line, "eth0")) {
                sscanf(line, " %[^:]: %llu %llu %*u %*u %*u %*u %*u %*u %llu %llu", 
                       iface, &rx_bytes, &rx_packets, &tx_bytes, &tx_packets);
                break;
            }
        }
        fclose(fp);

        // 3. 计算吞吐量
        unsigned long long rx_diff = (prev_rx == 0) ? 0 : (rx_bytes - prev_rx);
        unsigned long long tx_diff = (prev_tx == 0) ? 0 : (tx_bytes - prev_tx);
        prev_rx = rx_bytes; prev_tx = tx_bytes;
        format_speed(rx_diff, rx_speed);
        format_speed(tx_diff, tx_speed);

        // 4. 刷新 UI 界面
        printf("\033[H\033[J"); // 清屏并移动光标到左上角
        printf(CYAN "     .#####.\n"
               "     ##@@@@@@\n"
               "    #@@@@@@\n"
               "   #@@@@@@\n"
               "   #@@@@@@@\n"
               "    #@@@@@@@\n"
               "     ##@@@@@@@\n"
               "       .#######\n" RESET);
        printf(GREEN "   TsukineOS Network Monitor\n" RESET);
        printf("=================================\n");
        printf(" Interface : " YELLOW "eth0 (virtio-net)\n" RESET);
        printf(" RX Speed  : " GREEN "%s\n" RESET, rx_speed);
        printf(" TX Speed  : " GREEN "%s\n" RESET, tx_speed);
        printf("---------------------------------\n");
        printf(" Total RX  : %llu Bytes (%llu pkts)\n", rx_bytes, rx_packets);
        printf(" Total TX  : %llu Bytes (%llu pkts)\n", tx_bytes, tx_packets);
        printf("=================================\n");
        printf(RED " Press ANY KEY to exit\n" RESET); // 提示按任意键退出

        usleep(500000); // 休眠 0.5 秒，让 UI 刷新更流畅
    }

    disable_raw_mode(&orig_termios); // 退出前恢复终端正常模式
    printf("\033[H\033[J"); // 退出时清屏
    printf("TsukineOS Network Monitor exited.\n");
    return 0;
}