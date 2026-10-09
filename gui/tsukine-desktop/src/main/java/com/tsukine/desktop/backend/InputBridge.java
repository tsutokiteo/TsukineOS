package com.tsukine.desktop.backend;

import javafx.scene.input.MouseEvent;

public class InputBridge {

    private JsEngine jsEngine;

    public void setJsEngine(JsEngine engine) {
        this.jsEngine = engine;
    }

    public void dispatch(MouseEvent event) {
        if (jsEngine == null) return;

        String type = event.getEventType().getName();
        double x = event.getX();
        double y = event.getY();
        int button = event.getButton().ordinal();

        jsEngine.call("onMouse", type, x, y, button);
    }

    public void onKey(String key, String text) {
        if (jsEngine == null) return;
        jsEngine.call("onKey", key, text);
    }
}