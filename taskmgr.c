// taskmgr.c - Simple Task Manager GUI
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main() {
    Display *dpy;
    Window win;
    XEvent ev;
    int screen;
    char *msg = "Task Manager - TsukineOS";
    XSizeHints hints;

    dpy = XOpenDisplay(NULL);
    if (dpy == NULL) {
        fprintf(stderr, "Cannot open display. Check DISPLAY variable.\n");
        return 1;
    }

    screen = DefaultScreen(dpy);
    win = XCreateSimpleWindow(dpy, RootWindow(dpy, screen),
                              200, 200, 450, 250, 2,
                              BlackPixel(dpy, screen),
                              WhitePixel(dpy, screen));
    XStoreName(dpy, win, "Task Manager");
    hints.flags = PMinSize | PMaxSize;
    hints.min_width = hints.max_width = 450;
    hints.min_height = hints.max_height = 250;
    XSetWMNormalHints(dpy, win, &hints);

    XSelectInput(dpy, win, ExposureMask | KeyPressMask | StructureNotifyMask);
    XMapWindow(dpy, win);

    while (1) {
        XNextEvent(dpy, &ev);
        if (ev.type == Expose) {
            GC gc = XCreateGC(dpy, win, 0, NULL);
            XSetForeground(dpy, gc, BlackPixel(dpy, screen));
            XDrawString(dpy, win, gc, 20, 50, msg, strlen(msg));
            XDrawString(dpy, win, gc, 20, 80, "Process list: placeholder", 24);
            XDrawString(dpy, win, gc, 20, 110, "Real process list will be added later.", 39);
            XFreeGC(dpy, gc);
        } else if (ev.type == KeyPress) {
            break;
        } else if (ev.type == DestroyNotify) {
            break;
        }
    }

    XDestroyWindow(dpy, win);
    XCloseDisplay(dpy);
    return 0;
}