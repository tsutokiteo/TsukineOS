// ui.js — 所有函数直接挂 globalThis，不用 ui 对象包装

globalThis.renderFrame = function() {
    var tree = {
        type: "desktop",
        background: "#1a1a2e",
        children: [
            {
                type: "menubar",
                x: 0, y: 0, w: 1920, h: 28,
                background: "#16213e",
                children: [
                    { type: "label", text: "TsukineOS", x: 10, y: 4, color: "#eee", fontSize: 14 }
                ]
            },
            {
                type: "dock",
                x: 800, y: 1000, w: 320, h: 60,
                background: "#0f3460",
                radius: 8,
                children: [
                    { type: "icon", appId: "terminal", x: 10, y: 10, w: 40, h: 40, icon: "🖥" },
                    { type: "icon", appId: "files", x: 60, y: 10, w: 40, h: 40, icon: "📁" },
                    { type: "icon", appId: "settings", x: 110, y: 10, w: 40, h: 40, icon: "⚙" }
                ]
            }
        ]
    };

    // 如果有窗口状态，也画上去
    if (typeof windows !== 'undefined') {
        for (var i = 0; i < windows.length; i++) {
            tree.children.push(windows[i]);
        }
    }

    java.renderFrame(tree);
};

globalThis.handleMouse = function(type, x, y, button) {
    if (type === "press") {
        // 简单的窗口拖拽检测
        for (var i = windows.length - 1; i >= 0; i--) {
            var w = windows[i];
            if (x >= w.x && x <= w.x + w.w && y >= w.y && y <= w.y + 28) {
                activeWindowId = w.id;
                dragging = w.id;
                dragOffsetX = x - w.x;
                dragOffsetY = y - w.y;
                return;
            }
        }
    }
    if (type === "release") {
        dragging = null;
    }
    if (type === "move" && dragging) {
        for (var j = 0; j < windows.length; j++) {
            if (windows[j].id === dragging) {
                windows[j].x = x - dragOffsetX;
                windows[j].y = y - dragOffsetY;
                break;
            }
        }
    }
};

globalThis.handleKey = function(key, text) {
    // 预留
};

// 窗口状态也挂全局
globalThis.windows = [];
globalThis.nextZ = 10;
globalThis.activeWindowId = null;
globalThis.dragging = null;
globalThis.dragOffsetX = 0;
globalThis.dragOffsetY = 0;

globalThis.launchApp = function(appId) {
    var w = {
        id: "wnd_" + Date.now(),
        type: "window",
        appId: appId,
        title: appId,
        x: 200 + Math.random() * 200,
        y: 100 + Math.random() * 100,
        w: 700, h: 500,
        z: nextZ++,
        background: "#1e1e2e",
        children: [
            { type: "titlebar", text: appId, x: 0, y: 0, w: 700, h: 28, background: "#272740" },
            { type: "closeBtn", x: 670, y: 4, w: 20, h: 20, color: "#e74c3c" }
        ]
    };
    windows.push(w);
    activeWindowId = w.id;
    renderFrame();
};

print("ui.js loaded");