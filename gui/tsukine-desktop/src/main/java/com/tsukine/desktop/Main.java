package com.tsukine.desktop;

import com.tsukine.desktop.backend.JsEngine;
import com.tsukine.desktop.backend.Renderer;
import com.tsukine.desktop.backend.WaylandWindow;
import javafx.application.Application;
import javafx.stage.Stage;

public class Main extends Application {

    @Override
    public void start(Stage stage) {
        WaylandWindow waylandWindow = new WaylandWindow(stage);
        Renderer renderer = waylandWindow.getRenderer();

        JsEngine jsEngine = new JsEngine(renderer, waylandWindow);
        waylandWindow.getInputBridge().setJsEngine(jsEngine);

        jsEngine.loadResource("/js/ui.js");
        jsEngine.loadResource("/js/app.js");

        jsEngine.call("render", new Object[0]);

        waylandWindow.startRenderLoop();
    }

    @Override
    public void stop() {
        // cleanup
    }

    public static void main(String[] args) {
        launch(args);
    }
}