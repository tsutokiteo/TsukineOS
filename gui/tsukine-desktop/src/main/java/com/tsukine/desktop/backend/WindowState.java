package com.tsukine.desktop.backend;

public class WindowState {
    public long id;
    public String name;
    public String title;
    public double x, y, w, h;
    public boolean minimized;
    public boolean active;
    public int z;
    public String content;

    public WindowState() {}

    public WindowState(long id, String name, String title, double x, double y, double w, double h,
                       boolean minimized, boolean active, int z, String content) {
        this.id = id;
        this.name = name;
        this.title = title;
        this.x = x;
        this.y = y;
        this.w = w;
        this.h = h;
        this.minimized = minimized;
        this.active = active;
        this.z = z;
        this.content = content;
    }
}