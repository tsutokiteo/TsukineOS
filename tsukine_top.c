#include <ncurses.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int main() {
    initscr();            // 初始化 ncurses
    cbreak();             // 禁用行缓冲
    noecho();             // 关闭输入回显
    curs_set(0);          // 隐藏光标
    keypad(stdscr, TRUE); // 启用键盘特殊按键
    timeout(1000);        // 每秒刷新一次界面

    while (1) {
        clear();          // 清屏
        box(stdscr, 0, 0);// 画边框
        
        // 打印标题
        mvprintw(1, 2, " TsukineOS Task Manager (Alpha 0.0.1) ");
        mvhline(2, 1, ACS_HLINE, COLS-2); // 画水平分隔线

        // 模拟任务列表
        mvprintw(4, 2, "PID   USER      CPU%%   MEM%%   COMMAND");
        mvprintw(5, 2, "------------------------------------------------");
        mvprintw(6, 2, "1     root      0.0     0.1     init");
        mvprintw(7, 2, "2     root      0.0     0.0     kthreadd");
        mvprintw(8, 2, "100   dmin      1.2     2.5     tsukine_top");
        mvprintw(9, 2, "101   dmin      0.5     1.0     busybox sh");

        // 打印底部提示
        mvprintw(LINES-2, 2, "Press 'q' to exit | TsukineOS Alpha 0.0.1");
        
        refresh();        // 刷新屏幕显示

        // 监听键盘输入
        int ch = getch();
        if (ch == 'q' || ch == 'Q') {
            break;
        }
    }

    endwin();             // 结束 ncurses，恢复终端
    return 0;
}
