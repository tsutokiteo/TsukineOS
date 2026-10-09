// desktop.c - TsukineOS Desktop Launcher (TUI)
#include <ncurses.h>
#include <stdlib.h>
#include <string.h>

#define MAX_ITEMS 3

char *menu_items[] = {
    "Task Manager",
    "Guardian (Placeholder)",
    "Exit"
};

int main() {
    int choice = 0;
    int key;

    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    curs_set(0);

    while (1) {
        clear();
        attron(A_BOLD | A_UNDERLINE);
        mvprintw(0, 10, "TsukineOS Desktop Launcher");
        attroff(A_BOLD | A_UNDERLINE);
        mvprintw(2, 10, "Use Up/Down arrows, Enter to confirm");

        for (int i = 0; i < MAX_ITEMS; i++) {
            if (i == choice)
                attron(A_REVERSE);
            mvprintw(4 + i, 15, "%s", menu_items[i]);
            if (i == choice)
                attroff(A_REVERSE);
        }

        refresh();

        key = getch();
        switch (key) {
            case KEY_UP:
                choice = (choice - 1 + MAX_ITEMS) % MAX_ITEMS;
                break;
            case KEY_DOWN:
                choice = (choice + 1) % MAX_ITEMS;
                break;
            case 10: // Enter
                if (choice == 0) {
                    endwin();
                    system("./taskmgr_gui &");
                    initscr();
                    cbreak();
                    noecho();
                    keypad(stdscr, TRUE);
                    curs_set(0);
                } else if (choice == 1) {
                    endwin();
                    printf("Guardian placeholder. Not implemented yet.\n");
                    getchar();
                    initscr();
                    cbreak();
                    noecho();
                    keypad(stdscr, TRUE);
                    curs_set(0);
                } else if (choice == 2) {
                    endwin();
                    printf("Exit desktop.\n");
                    return 0;
                }
                break;
            default:
                break;
        }
    }

    endwin();
    return 0;
}