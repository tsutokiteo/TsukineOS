#include <ncurses.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <stdio.h>

#define MAX_PROCS 256

typedef struct {
    int pid;
    char name[128];
    char user[16];
    char state;
    unsigned long vm_rss; 
    unsigned long utime;  
    unsigned long stime;  
    float cpu_percent;
    float mem_percent;
} Process;

Process procs[MAX_PROCS];
Process prev_procs[MAX_PROCS]; 
int proc_count = 0;
unsigned long total_mem_kb = 0;

void get_total_mem() {
    FILE *fp = fopen("/proc/meminfo", "r");
    if (!fp) return;
    char line[128];
    while (fgets(line, sizeof(line), fp)) {
        if (strncmp(line, "MemTotal:", 9) == 0) {
            sscanf(line, "MemTotal: %lu", &total_mem_kb);
            break;
        }
    }
    fclose(fp);
}

void get_username(uid_t uid, char *buf, int size) {
    if (uid == 0) strncpy(buf, "root", size - 1);
    else snprintf(buf, size, "user");
}

void scan_processes() {
    DIR *dir = opendir("/proc");
    if (!dir) return;
    struct dirent *entry;
    char path[256], line[512]; // 扩大 line 缓冲区，防止长行截断
    FILE *fp;
    int new_count = 0;

while ((entry = readdir(dir)) != NULL && new_count < MAX_PROCS) {
        int pid;
        unsigned long utime = 0, stime = 0; // <--- 【修复】在这里提前声明，提升作用域！
        if (sscanf(entry->d_name, "%d", &pid) == 1) {
            // 1. 读取 /proc/[pid]/stat
            snprintf(path, sizeof(path), "/proc/%d/stat", pid);
            fp = fopen(path, "r");
            if (!fp) continue;
            
            if (fgets(line, sizeof(line), fp)) {
                // 找到第一个 '(' 和最后一个 ')' 的位置
                char *start = strchr(line, '(');
                char *end = strrchr(line, ')');
                
                if (start && end && end > start) {
                    // 安全提取进程名
                    size_t len = end - start - 1;
                    if (len >= sizeof(procs[new_count].name)) len = sizeof(procs[new_count].name) - 1;
                    strncpy(procs[new_count].name, start + 1, len);
                    procs[new_count].name[len] = '\0';

                    // 从 ')' 之后开始解析状态和时间
                    unsigned long utime = 0, stime = 0;
                    char state = '?';
                    // 格式: ") S 1 1 1 0 -1 4194304 ... utime stime"
                    // 我们只需要跳过 12 个字段 (包含状态)，然后读取 utime 和 stime
                    sscanf(end + 2, "%c %*s %*s %*s %*s %*s %*s %*s %*s %*s %*s %lu %lu", 
                           &state, &utime, &stime);

                    procs[new_count].pid = pid;
                    procs[new_count].state = state;
                    procs[new_count].utime = utime;
                    procs[new_count].stime = stime;
                } else {
                    fclose(fp);
                    continue;
                }
            }
            fclose(fp);

            // 2. 读取 /proc/[pid]/status
            snprintf(path, sizeof(path), "/proc/%d/status", pid);
            fp = fopen(path, "r");
            if (fp) {
                uid_t uid = 0;
                while (fgets(line, sizeof(line), fp)) {
                    if (strncmp(line, "Uid:", 4) == 0) sscanf(line, "Uid: %u", &uid);
                    else if (strncmp(line, "VmRSS:", 6) == 0) sscanf(line, "VmRSS: %lu", &procs[new_count].vm_rss);
                }
                fclose(fp);
                get_username(uid, procs[new_count].user, sizeof(procs[new_count].user));
            }

            procs[new_count].mem_percent = (total_mem_kb > 0) ? 
                ((float)procs[new_count].vm_rss / total_mem_kb) * 100.0 : 0;

            procs[new_count].cpu_percent = 0.0;
            for (int i = 0; i < proc_count; i++) {
                if (prev_procs[i].pid == pid) {
                    unsigned long total_time = utime + stime;
                    unsigned long prev_total_time_proc = prev_procs[i].utime + prev_procs[i].stime;
                    unsigned long clock_tick = sysconf(_SC_CLK_TCK);
                    if (clock_tick > 0) {
                        procs[new_count].cpu_percent = 
                            ((float)(total_time - prev_total_time_proc) / clock_tick) * 100.0;
                    }
                    break;
                }
            }
            new_count++;
        }
    }
    closedir(dir);

    proc_count = new_count;
    memcpy(prev_procs, procs, sizeof(Process) * proc_count);
}

int main(int argc, char *argv[]) {
    int mode = 0; 
    if (argc > 1) {
        if (strcmp(argv[1], "-u") == 0) mode = 1;
        else if (strcmp(argv[1], "-c") == 0) mode = 2;
    }

    get_total_mem();
    initscr(); cbreak(); noecho(); curs_set(0); keypad(stdscr, TRUE); timeout(1000);

    scan_processes(); 
    sleep(1); 

    while (1) {
        scan_processes();
        clear(); box(stdscr, 0, 0);
        
        mvprintw(1, 2, " TsukineOS Task Manager (Real-Time) ");
        mvhline(2, 1, ACS_HLINE, COLS-2);

        mvprintw(4, 2, "PID      USER      CPU%%    MEM%%    STATE   COMMAND");
        mvprintw(5, 2, "------------------------------------------------------------");

        int max_display = LINES - 8; 
        if (max_display < 1) max_display = 1;
        int displayed = 0;

        for (int i = 0; i < proc_count && displayed < max_display; i++) {
            int is_kernel = (procs[i].name[0] == '[' || procs[i].pid == 2);
            if ((mode == 1 && is_kernel) || (mode == 2 && !is_kernel)) continue;

            mvprintw(6 + displayed, 2, "%-8d %-9s %-8.1f %-8.1f %-7c %s", 
                     procs[i].pid, procs[i].user, procs[i].cpu_percent, 
                     procs[i].mem_percent, procs[i].state, procs[i].name);
            displayed++;
        }

        const char *mode_str = (mode == 1) ? "[User Only]" : (mode == 2) ? "[Kernel Only]" : "[All]";
        mvprintw(LINES-2, 2, "Press 'q' to exit | %s | Displayed: %d", mode_str, displayed);
        refresh();

        int ch = getch();
        if (ch == 'q' || ch == 'Q') break;
    }

    endwin();
    return 0;
}