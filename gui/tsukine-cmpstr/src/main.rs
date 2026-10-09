use smithay::backend::winit;
use smithay::backend::renderer::gles::GlesRenderer;
use smithay::reexports::wayland_server::Display;
use smithay::wayland::compositor::{
    CompositorState, CompositorHandler, CompositorClientState,
};
use smithay::wayland::shell::xdg::{
    XdgShellState, XdgShellHandler, ToplevelSurface, PopupSurface, PositionerState,
};
use smithay::input::{SeatState, SeatHandler};
use smithay::utils::Serial;
use tracing::info;

mod desktop;
mod shell;
mod lpac;
mod input;
mod render;

// 临时占位 — 后面接真实 window/surface 聚焦管理
#[derive(Debug)]
struct DummyFocus;

struct State {
    compositor_state: CompositorState,
    xdg_shell_state: XdgShellState,
    seat_state: SeatState,
}

smithay::delegate_compositor!(State);
smithay::delegate_xdg_shell!(State);
smithay::delegate_seat!(State);

impl CompositorHandler for State {
    fn compositor_state(&mut self) -> &mut CompositorState {
        &mut self.compositor_state
    }

    fn client_compositor_state<'a>(&self, _client: &'a smithay::reexports::wayland_server::Client) -> &'a CompositorClientState {
        panic!("client_compositor_state not implemented yet");
    }

    fn commit(&mut self, surface: &smithay::reexports::wayland_server::protocol::wl_surface::WlSurface) {
        info!("surface commit: {:?}", surface);
    }
}

impl XdgShellHandler for State {
    fn xdg_shell_state(&mut self) -> &mut XdgShellState {
        &mut self.xdg_shell_state
    }

    fn new_toplevel(&mut self, surface: ToplevelSurface) {
        info!("New xdg toplevel: {:?}", surface);
    }

    fn new_popup(&mut self, surface: PopupSurface, _positioner: PositionerState) {
        info!("New xdg popup: {:?}", surface);
    }

    fn grab(&mut self, surface: PopupSurface, _seat: smithay::reexports::wayland_server::protocol::wl_seat::WlSeat, _serial: Serial) {
        info!("Popup grab: {:?}", surface);
    }

    fn reposition_request(&mut self, surface: PopupSurface, _positioner: PositionerState, token: u32) {
        info!("Popup reposition request: token={}", token);
    }
}

impl SeatHandler for State {
    type KeyboardFocus = DummyFocus;
    type PointerFocus = DummyFocus;
    type TouchFocus = DummyFocus;

    fn seat_state(&mut self) -> &mut SeatState {
        &mut self.seat_state
    }
}

fn main() -> Result<(), Box<dyn std::error::Error>> {
    tracing_subscriber::fmt::init();
    info!("TsukineCmpstr starting...");

    let mut display: Display<State> = Display::new()?;
    let dh = display.handle();

    let socket_source = smithay::wayland::socket::ListeningSocketSource::new_auto()?;
    let socket_name = socket_source.socket_name().to_string_lossy().to_string();
    info!("Wayland socket: {}", socket_name);
    println!("WAYLAND_DISPLAY={}", socket_name);

    let mut state = State {
        compositor_state: CompositorState::new::<State>(&dh),
        xdg_shell_state: XdgShellState::new::<State>(&dh),
        seat_state: SeatState::new(),
    };

    info!("Compositor + xdg-shell + seat initialized");

    let (_backend, mut event_loop) = winit::init::<GlesRenderer>()?;
    info!("Winit backend initialized");

    let mut running = true;
    while running {
        event_loop.dispatch_new_events(|event| match event {
            winit::WinitEvent::CloseRequested => {
                info!("Window close requested, exiting");
                running = false;
            }
            winit::WinitEvent::Resized { .. } => {}
            _ => {}
        });

        display.dispatch_clients(&mut state)?;
        display.flush_clients()?;
    }

    info!("TsukineCmpstr exited");
    Ok(())
}