package com.tsukine.desktop.backend;

import javafx.animation.AnimationTimer;
import javafx.scene.Scene;
import javafx.scene.canvas.Canvas;
import javafx.scene.input.MouseEvent;
import javafx.scene.layout.Pane;
import javafx.stage.Stage;

public class WaylandWindow {

    public static final int SCREEN_W = 1920;
    public static final int SCREEN_H = 1080;

    private final Stage stage;
    private final Canvas canvas;
    private final Renderer renderer;
    private final InputBridge inputBridge;

    public WaylandWindow(Stage stage) {
        this.stage = stage;
        this.canvas = new Canvas(SCREEN_W, SCREEN_H);
        this.renderer = new Renderer(canvas);
        this.inputBridge = new InputBridge();

        Pane root = new Pane(canvas);
        Scene scene = new Scene(root, SCREEN_W, SCREEN_H);

        stage.setTitle("TsukineOS Desktop");
        stage.setScene(scene);
        stage.setFullScreen(true);
        stage.show();

        scene.addEventFilter(MouseEvent.ANY, event -> {
            inputBridge.dispatch(event);
        });

        scene.setOnKeyPressed(event -> {
            inputBridge.onKey(event.getCode().getName(), event.getText());
        });
    }

    public Renderer getRenderer() {
        return renderer;
    }

    public void startRenderLoop() {
        new AnimationTimer() {
            @Override
            public void handle(long now) {
                renderer.flush();
            }
        }.start();
    }

    public InputBridge getInputBridge() {
        return inputBridge;
    }

    public Canvas getCanvas() {
        return canvas;
    }
}