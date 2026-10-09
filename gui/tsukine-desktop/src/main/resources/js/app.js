print("TsukineOS Desktop JS layer loaded");

globalThis.render = function() {
    renderFrame();
};

globalThis.onMouse = function(type, x, y, button) {
    handleMouse(type, x, y, button);
    renderFrame();
};

globalThis.onKey = function(key, text) {
    handleKey(key, text);
};