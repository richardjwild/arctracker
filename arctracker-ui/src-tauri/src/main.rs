use arctracker_ui_lib::arctracker::{default_module_params, initialise};
use arctracker_ui_lib::state::PendingOpenRequest;
use arctracker_ui_lib::AppState;
use std::error::Error;
use std::sync::{Arc, Mutex};
use tauri::{App, AppHandle, Emitter, Manager, RunEvent, Runtime};

const EXIT_REQUESTED_EVENT: &str = "exit-requested";

fn main() {
    let app_state = create_app_state();
    let app = arctracker_ui_lib::build_app(app_state)
        .setup(setup_app)
        .build(tauri::generate_context!())
        .expect("error while building Tauri app");
    app.run(handle_run_event);
}

fn create_app_state() -> Arc<AppState> {
    let mut init = initialise().expect("failed to initialize arctracker");
    init.tracker
        .create_module(default_module_params())
        .expect("failed to initialise module");
    Arc::new(AppState {
        tracker: Arc::new(Mutex::new(init.tracker)),
        midi: Mutex::new(init.midi),
    })
}

fn setup_app<R: Runtime>(app: &mut App<R>) -> Result<(), Box<dyn Error>> {
    //
    // If Arctracker has received a command line argument, assume it is a module and try opening it.
    //
    if let Some(path) = std::env::args_os()
        .skip(1)
        .map(std::path::PathBuf::from)
        .find(|path| path.is_file())
    {
        let pending = app.state::<PendingOpenRequest>();
        pending.set(path);
    }
    Ok(())
}

fn handle_run_event<R: Runtime>(app_handle: &AppHandle<R>, event: RunEvent) {
    match event {
        #[cfg(target_os = "macos")]
        RunEvent::Opened { urls } => {
            if let Some(path) = urls
                .into_iter()
                .filter_map(|url| url.to_file_path().ok())
                .next()
            {
                let pending = app_handle.state::<PendingOpenRequest>();
                pending.set(path);
                // Notify the frontend that a request is waiting.
                let _ = app_handle.emit("open-requested-files", ());
            }
        }
        RunEvent::ExitRequested { api, code, .. } if code.is_none() => {
            api.prevent_exit();
            request_event(app_handle, EXIT_REQUESTED_EVENT);
        }
        RunEvent::ExitRequested { .. } => {
            // Programmatic exit or restart: allow it through.
        }
        RunEvent::Exit => {
            let state = app_handle.state::<Arc<AppState>>();
            {
                let mut midi = state.midi.lock().unwrap();
                if let Some(midi) = midi.as_mut() {
                    midi.destroy();
                }
            }
            {
                let mut tracker = state.tracker.lock().unwrap();
                tracker.destroy();
            }
        }
        _ => {}
    }
}

fn request_event<R: Runtime>(app_handle: &AppHandle<R>, event: &str) {
    if let Err(err) = app_handle.emit(event, ()) {
        eprintln!("Failed to emit {event}: {err}");
    }
}
