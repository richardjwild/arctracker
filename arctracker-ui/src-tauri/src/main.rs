mod midi;

use arctracker_ui_lib::arctracker::Arctracker;
use arctracker_ui_lib::state::EditorState;
use arctracker_ui_lib::AppState;
use std::error::Error;
use std::sync::{Arc, Mutex};
use tauri::menu::{Menu, MenuItem, PredefinedMenuItem, Submenu};
use tauri::{App, AppHandle, Emitter, RunEvent, Runtime, WindowEvent};

const QUIT_MENU_ID: &str = "quit-arctracker";
const EXIT_REQUESTED_EVENT: &str = "exit-requested";

fn main() {
    let app_state = create_app_state();
    start_midi(app_state.clone());
    let app = arctracker_ui_lib::build_app(app_state)
        .setup(setup_app)
        .build(tauri::generate_context!())
        .expect("error while building Tauri app");
    app.run(handle_run_event);
}

fn create_app_state() -> Arc<AppState> {
    let mut tracker = Arctracker::new().expect("failed to create tracker");
    tracker
        .create_module(8)
        .expect("failed to initialise module");
    tracker
        .start()
        .expect("failed to initialise audio subsystem");
    Arc::new(AppState {
        tracker: Arc::new(Mutex::new(tracker)),
        editor: Mutex::new(EditorState {
            selected_instrument: 0,
            selected_channel: 0,
        }),
    })
}

fn start_midi(app_state: Arc<AppState>) {
    std::thread::spawn(move || {
        midi::start_midi_thread(app_state);
    });
}

fn setup_app<R: Runtime>(app: &mut App<R>) -> Result<(), Box<dyn Error>> {
    install_menu(app)?; 
    app.on_menu_event(|app_handle, event| {
        if event.id().as_ref() == QUIT_MENU_ID {
            request_exit_confirmation(app_handle);
        }
    });
    Ok(())
}

fn install_menu<R: Runtime>(app: &App<R>) -> tauri::Result<()> {
    let quit = MenuItem::with_id(
        app,
        QUIT_MENU_ID,
        "Quit Arctracker",
        true,
        Some("CmdOrCtrl+Q"),
    )?;
    let about = PredefinedMenuItem::about(app, Some("About Arctracker"), None)?;
    let separator = PredefinedMenuItem::separator(app)?;
    let app_menu = Submenu::with_items(app, "Arctracker", true, &[&about, &separator, &quit])?;
    let menu = Menu::with_items(app, &[&app_menu])?;
    app.set_menu(menu)?;
    Ok(())
}

fn handle_run_event<R: Runtime>(app_handle: &AppHandle<R>, event: RunEvent) {
    match event {
        RunEvent::WindowEvent {
            event: WindowEvent::CloseRequested { api, .. },
            ..
        } => {
            api.prevent_close();
            request_exit_confirmation(app_handle);
        }
        RunEvent::ExitRequested { api, code, .. } if code.is_none() => {
            api.prevent_exit();
            request_exit_confirmation(app_handle);
        }
        RunEvent::ExitRequested { .. } => {
            // Programmatic exit or restart: allow it through.
        }
        _ => {}
    }
}

fn request_exit_confirmation<R: Runtime>(app_handle: &AppHandle<R>) {
    if let Err(err) = app_handle.emit(EXIT_REQUESTED_EVENT, ()) {
        eprintln!("Failed to emit {EXIT_REQUESTED_EVENT}: {err}");
    }
}
