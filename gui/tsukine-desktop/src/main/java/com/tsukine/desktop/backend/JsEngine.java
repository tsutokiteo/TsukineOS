package com.tsukine.desktop.backend;

import org.graalvm.polyglot.Context;
import org.graalvm.polyglot.Value;

import java.io.InputStream;
import java.io.InputStreamReader;
import java.io.BufferedReader;
import java.io.IOException;

public class JsEngine {

    private final Context context;
    private final Renderer renderer;
    private final WaylandWindow waylandWindow;

    public JsEngine(Renderer renderer, WaylandWindow waylandWindow) {
        this.renderer = renderer;
        this.waylandWindow = waylandWindow;

        this.context = Context.newBuilder("js")
                .allowAllAccess(true)
                .option("js.ecmascript-version", "2022")
                .build();

        JavaBridge bridge = new JavaBridge(renderer, waylandWindow, this);
        context.getBindings("js").putMember("java", bridge);
    }

    public void loadResource(String path) {
        try (InputStream is = getClass().getResourceAsStream(path)) {
            if (is == null) {
                System.err.println("JS resource not found: " + path);
                return;
            }
            BufferedReader reader = new BufferedReader(new InputStreamReader(is));
            StringBuilder sb = new StringBuilder();
            String line;
            while ((line = reader.readLine()) != null) {
                sb.append(line).append('\n');
            }
            context.eval("js", sb.toString());
        } catch (IOException e) {
            System.err.println("Failed to load JS: " + path);
            e.printStackTrace();
        }
    }

    public void load(String code) {
        context.eval("js", code);
    }

    public Value call(String funcName, Object... args) {
        Value func = context.getBindings("js").getMember(funcName);
        if (func == null || !func.canExecute()) return null;
        return func.execute(args);
    }

    public void close() {
        context.close();
    }

    public static class JavaBridge {
        private final Renderer renderer;
        private final WaylandWindow waylandWindow;
        private final JsEngine engine;

        public JavaBridge(Renderer renderer, WaylandWindow waylandWindow, JsEngine engine) {
            this.renderer = renderer;
            this.waylandWindow = waylandWindow;
            this.engine = engine;
        }

        public void renderFrame(Object tree) {
            renderer.renderFrame(tree);
        }

        public void launchProcess(String name) {
            System.out.println("[Java] launchProcess: " + name);
        }

        public void log(String msg) {
            System.out.println("[JS] " + msg);
        }

        public Object getScreenSize() {
            return new java.util.HashMap<String, Object>() {{
                put("w", WaylandWindow.SCREEN_W);
                put("h", WaylandWindow.SCREEN_H);
            }};
        }
    }
}