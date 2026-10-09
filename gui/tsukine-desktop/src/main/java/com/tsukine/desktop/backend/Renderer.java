package com.tsukine.desktop.backend;

import javafx.scene.canvas.Canvas;
import javafx.scene.canvas.GraphicsContext;
import javafx.scene.paint.Color;
import javafx.scene.paint.CycleMethod;
import javafx.scene.paint.LinearGradient;
import javafx.scene.paint.Stop;
import javafx.scene.text.Font;

import java.util.ArrayList;
import java.util.List;
import java.util.Map;

public class Renderer {

    private final Canvas canvas;
    private final GraphicsContext g;
    private Map<String, Object> currentTree;

    public Renderer(Canvas canvas) {
        this.canvas = canvas;
        this.g = canvas.getGraphicsContext2D();
    }

    @SuppressWarnings("unchecked")
    public void renderFrame(Object treeObj) {
        if (treeObj instanceof Map) {
            this.currentTree = (Map<String, Object>) treeObj;
        }
    }

    public void flush() {
        if (currentTree == null) return;

        g.clearRect(0, 0, canvas.getWidth(), canvas.getHeight());

        // 背景渐变
        LinearGradient bg = new LinearGradient(0, 0, 0, canvas.getHeight(),
                false, CycleMethod.NO_CYCLE,
                new Stop(0, Color.rgb(26, 26, 46)),
                new Stop(1, Color.rgb(15, 15, 30)));
        g.setFill(bg);
        g.fillRect(0, 0, canvas.getWidth(), canvas.getHeight());

        List<Map<String, Object>> children =
                (List<Map<String, Object>>) currentTree.get("children");
        if (children == null) return;

        children.stream()
                .sorted((a, b) -> {
                    int za = ((Number) a.getOrDefault("z", 0)).intValue();
                    int zb = ((Number) b.getOrDefault("z", 0)).intValue();
                    return Integer.compare(za, zb);
                })
                .forEach(this::drawNode);
    }

    @SuppressWarnings("unchecked")
    private void drawNode(Map<String, Object> node) {
        String type = (String) node.get("type");
        if (type == null) return;

        switch (type) {
            case "menubar": drawMenubar(node); break;
            case "desktop-icons": drawDesktopIcons(node); break;
            case "window": drawWindow(node); break;
            case "dock": drawDock(node); break;
            default: break;
        }
    }

    private void drawMenubar(Map<String, Object> node) {
        g.setFill(Color.rgb(20, 20, 35, 0.9));
        g.fillRect(0, 0, canvas.getWidth(), 28);

        g.setStroke(Color.rgb(60, 60, 90, 0.5));
        g.setLineWidth(1);
        g.strokeRect(0, 0, canvas.getWidth(), 28);

        g.setFill(Color.WHITE);
        g.setFont(Font.font("Sans", 13));

        List<String> items = (List<String>) node.get("items");
        if (items == null) items = List.of("TsukineOS", "文件", "编辑", "显示", "窗口", "帮助");

        int x = 12;
        for (String item : items) {
            g.fillText(item, x, 18);
            x += (int)(g.getFont().getSize() * item.length() * 0.6) + 20;
        }
    }

    @SuppressWarnings("unchecked")
    private void drawDesktopIcons(Map<String, Object> node) {
        List<Map<String, Object>> icons = (List<Map<String, Object>>) node.get("icons");
        if (icons == null) return;

        int col = 0, row = 0;
        for (Map<String, Object> icon : icons) {
            double ix = 20 + col * 80;
            double iy = 50 + row * 90;

            g.setFill(Color.rgb(60, 60, 90, 0.6));
            g.fillRoundRect(ix, iy, 64, 64, 12, 12);

            g.setFill(Color.WHITE);
            g.setFont(Font.font("Sans", 10));
            String name = (String) icon.get("name");
            g.fillText(name, ix + 2, iy + 80);

            col++;
            if (col >= 2) { col = 0; row++; }
        }
    }

    private void drawWindow(Map<String, Object> node) {
        double x = toDouble(node.get("x"));
        double y = toDouble(node.get("y"));
        double w = toDouble(node.get("w"));
        double h = toDouble(node.get("h"));
        String title = (String) node.getOrDefault("title", "");
        boolean active = (boolean) node.getOrDefault("active", false);

        // 阴影
        g.setFill(Color.rgb(0, 0, 0, 0.3));
        g.fillRoundRect(x + 4, y + 4, w, h, 14, 14);

        // 主体
        g.setFill(Color.rgb(40, 40, 60, 0.95));
        g.fillRoundRect(x, y, w, h, 12, 12);

        // 标题栏
        if (active) {
            g.setFill(Color.rgb(70, 90, 130, 0.9));
        } else {
            g.setFill(Color.rgb(50, 50, 75, 0.9));
        }
        g.fillRoundRect(x, y, w, 30, 12, 12);
        g.fillRect(x, y + 15, w, 15);

        // 红黄绿
        g.setFill(Color.rgb(255, 95, 86));
        g.fillOval(x + 10, y + 9, 12, 12);
        g.setFill(Color.rgb(255, 189, 46));
        g.fillOval(x + 26, y + 9, 12, 12);
        g.setFill(Color.rgb(39, 201, 63));
        g.fillOval(x + 42, y + 9, 12, 12);

        // 标题
        g.setFill(Color.WHITE);
        g.setFont(Font.font("Sans", 12));
        g.fillText(title, x + 60, y + 19);

        // 内容
        String content = (String) node.getOrDefault("content", "");
        drawWindowContent(x, y, w, h, content);
    }

    private void drawWindowContent(double x, double y, double w, double h, String content) {
        double cx = x + 1, cy = y + 31;
        double cw = w - 2, ch = h - 31;

        switch (content) {
            case "终端":
                g.setFill(Color.rgb(10, 10, 20));
                g.fillRect(cx, cy, cw, ch);
                g.setFill(Color.rgb(0, 255, 0));
                g.setFont(Font.font("Monospace", 12));
                g.fillText("tsukine@desktop ~ $ ", cx + 8, cy + 20);
                g.fillText("Welcome to TsukineOS Terminal", cx + 8, cy + 40);
                g.fillText("Type 'help' for available commands.", cx + 8, cy + 56);
                break;
            case "文件":
                g.setFill(Color.rgb(25, 25, 40));
                g.fillRect(cx, cy, cw, ch);
                g.setFill(Color.WHITE);
                g.setFont(Font.font("Sans", 11));
                g.fillText("📁 主目录", cx + 10, cy + 20);
                g.fillText("📁 文档", cx + 10, cy + 40);
                g.fillText("📁 下载", cx + 10, cy + 60);
                g.fillText("📁 图片", cx + 10, cy + 80);
                break;
            case "设置":
                g.setFill(Color.rgb(35, 30, 50));
                g.fillRect(cx, cy, cw, ch);
                g.setFill(Color.WHITE);
                g.setFont(Font.font("Sans", 11));
                g.fillText("⚙ 显示", cx + 10, cy + 20);
                g.fillText("⚙ 声音", cx + 10, cy + 40);
                g.fillText("⚙ 网络", cx + 10, cy + 60);
                g.fillText("⚙ LPAC", cx + 10, cy + 80);
                break;
            case "TsukiAmdro":
                g.setFill(Color.rgb(20, 30, 50));
                g.fillRect(cx, cy, cw, ch);
                g.setFill(Color.rgb(100, 150, 255));
                g.setFont(Font.font("Sans", 14));
                g.fillText("TsukiAmdro", cx + 10, cy + 20);
                g.setFill(Color.rgb(180, 200, 255));
                g.setFont(Font.font("Sans", 11));
                g.fillText("(iframe placeholder)", cx + 10, cy + 40);
                break;
            default:
                g.setFill(Color.rgb(30, 30, 50, 0.8));
                g.fillRect(cx, cy, cw, ch);
        }
    }

    private void drawDock(Map<String, Object> node) {
        @SuppressWarnings("unchecked")
        List<String> items = (List<String>) node.get("items");
        if (items == null) return;

        double dockH = 64;
        double dockY = canvas.getHeight() - dockH - 8;
        double totalW = items.size() * 56 + 16;
        double dockX = (canvas.getWidth() - totalW) / 2;

        g.setFill(Color.rgb(20, 20, 35, 0.75));
        g.fillRoundRect(dockX, dockY, totalW, dockH, 16, 16);

        g.setStroke(Color.rgb(80, 80, 120, 0.4));
        g.setLineWidth(1);
        g.strokeRoundRect(dockX, dockY, totalW, dockH, 16, 16);

        int i = 0;
        for (String item : items) {
            double ix = dockX + 8 + i * 56;
            double iy = dockY + 8;

            g.setFill(Color.rgb(60, 60, 100, 0.8));
            g.fillRoundRect(ix, iy, 48, 48, 10, 10);

            g.setFill(Color.WHITE);
            g.setFont(Font.font("Sans", 9));
            g.fillText(item, ix + 4, iy + 28);

            i++;
        }
    }

    private double toDouble(Object o) {
        if (o instanceof Number) return ((Number) o).doubleValue();
        return 0;
    }
}