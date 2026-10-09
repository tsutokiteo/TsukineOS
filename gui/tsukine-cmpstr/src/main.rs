use smithay::backend::winit;
use smithay::reexports::wayland_server::Display;
use smithay::backend::renderer::gles::GlesRenderer;
use tracing::info;

mod desktop;
mod shell;
mod lpac;
mod input;
mod render;

fn main() -> Result<(), Box<dyn std::error::Error>> {
    tracing_subscriber::fmt::init();
    info!("TsukineCmpstr starting...");

    let mut display: Display<()> = Display::new()?;
    info!("Wayland display created");

    // 指定 GlesRenderer 作为渲染器类型
    let (mut backend, mut event_loop) = winit::init::<GlesRenderer>()?;
    info!("Winit backend initialized — window should appear");

    let mut running = true;
    while running {
        event_loop.dispatch_new_events(|event| match event {
            winit::WinitEvent::CloseRequested => {
                info!("Window close requested, exiting");
                running = false;
            }
            winit::WinitEvent::Resized { .. } => {
                // backend auto-resizes
            }
            _ => {}
        });

        display.dispatch_clients(&mut ())?;
        display.flush_clients()?;
    }

    info!("TsukineCmpstr exited");
    Ok(())
}