package com.tsukine.desktop.protocol;

public class RenderCommand {
    public enum Type {
        DRAW_RECT, DRAW_TEXT, DRAW_ICON, CLEAR
    }

    private final Type type;
    private final Object[] params;

    public RenderCommand(Type type, Object... params) {
        this.type = type;
        this.params = params;
    }

    public Type getType() { return type; }
    public Object[] getParams() { return params; }
}