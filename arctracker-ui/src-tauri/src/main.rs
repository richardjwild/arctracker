use arctracker_ui_lib::arctracker::{default_module_params, initialise, NewModuleParams};
use arctracker_ui_lib::AppState;
use std::error::Error;
use std::sync::{Arc, Mutex};
use tauri::menu::{Menu, MenuItem, PredefinedMenuItem, Submenu};
use tauri::{App, AppHandle, Emitter, Manager, RunEvent, Runtime, WindowEvent};

const OPEN_SETTINGS_MENU_ID: &str = "open_settings";
const QUIT_MENU_ID: &str = "quit-arctracker";
const NEW_MODULE_MENU_ID: &str = "new-module";
const NEW_MODULE_USING_DEFAULTS_MENU_ID: &str = "new-module-using-defaults";
const OPEN_MODULE_MENU_ID: &str = "open-module";
const SAVE_MODULE_MENU_ID: &str = "save-module";
const SAVE_MODULE_AS_MENU_ID: &str = "save-module-as";
const EXPORT_AUDIO_MENU_ID: &str = "export-audio";
const EDIT_DETAILS_MENU_ID: &str = "edit-module-details";
const SET_TRACK_COUNT_MENU_ID: &str = "set-track-count";
const SET_TEMPO_MENU_ID: &str = "set-tempo";
const UNDO_MENU_ID: &str = "undo";
const REDO_MENU_ID: &str = "redo";
const TOGGLE_EDIT_MENU_ID: &str = "toggle-edit";
const CUT_EVENTS_MENU_ID: &str = "cut-events";
const COPY_EVENTS_MENU_ID: &str = "copy-events";
const PASTE_EVENTS_MENU_ID: &str = "paste-events";
const CUT_TRACK_MENU_ID: &str = "cut-track";
const COPY_TRACK_MENU_ID: &str = "copy-track";
const PASTE_TRACK_MENU_ID: &str = "paste-track";
const CUT_PATTERN_MENU_ID: &str = "cut-pattern";
const COPY_PATTERN_MENU_ID: &str = "copy-pattern";
const PASTE_PATTERN_MENU_ID: &str = "paste-pattern";
const INSERT_SEQUENCE_BEFORE_MENU_ID: &str = "insert-sequence-before";
const INSERT_SEQUENCE_AFTER_MENU_ID: &str = "insert-sequence-after";
const INSERT_SEQUENCE_BEFORE_WITH_NEW_MENU_ID: &str = "insert-sequence-before-with-new";
const INSERT_SEQUENCE_AFTER_WITH_NEW_MENU_ID: &str = "insert-sequence-after-with-new";
const DELETE_SEQUENCE_POSITION_MENU_ID: &str = "delete-sequence-position";
const SET_PATTERN_LENGTH_MENU_ID: &str = "set-pattern-length";
const OPEN_SETTINGS_REQUESTED_EVENT: &str = "open-settings-requested";
const EXIT_REQUESTED_EVENT: &str = "exit-requested";
const NEW_MODULE_REQUESTED_EVENT: &str = "new-module-requested";
const NEW_MODULE_USING_DEFAULTS_REQUESTED_EVENT: &str = "new-module-using-defaults-requested";
const OPEN_MODULE_REQUESTED_EVENT: &str = "open-module-requested";
const SAVE_MODULE_REQUESTED_EVENT: &str = "save-module-requested";
const SAVE_MODULE_AS_REQUESTED_EVENT: &str = "save-module-as-requested";
const EXPORT_AUDIO_REQUESTED_EVENT: &str = "export-audio-requested";
const EDIT_DETAILS_REQUESTED_EVENT: &str = "edit-module-details-requested";
const SET_TRACK_COUNT_REQUESTED_EVENT: &str = "set-track-count-requested";
const SET_TEMPO_REQUESTED_EVENT: &str = "set-tempo-requested";
const UNDO_REQUESTED_EVENT: &str = "undo-requested";
const REDO_REQUESTED_EVENT: &str = "redo-requested";
const TOGGLE_EDIT_REQUESTED_EVENT: &str = "toggle-edit-requested";
const CUT_EVENTS_REQUESTED_EVENT: &str = "cut-events-requested";
const COPY_EVENTS_REQUESTED_EVENT: &str = "copy-events-requested";
const PASTE_EVENTS_REQUESTED_EVENT: &str = "paste-events-requested";
const CUT_TRACK_REQUESTED_EVENT: &str = "cut-track-requested";
const COPY_TRACK_REQUESTED_EVENT: &str = "copy-track-requested";
const PASTE_TRACK_REQUESTED_EVENT: &str = "paste-track-requested";
const CUT_PATTERN_REQUESTED_EVENT: &str = "cut-pattern-requested";
const COPY_PATTERN_REQUESTED_EVENT: &str = "copy-pattern-requested";
const PASTE_PATTERN_REQUESTED_EVENT: &str = "paste-pattern-requested";
const INSERT_SEQUENCE_BEFORE_REQUESTED_EVENT: &str = "insert-sequence-before-requested";
const INSERT_SEQUENCE_AFTER_REQUESTED_EVENT: &str = "insert-sequence-after-requested";
const INSERT_SEQUENCE_BEFORE_WITH_NEW_REQUESTED_EVENT: &str =
    "insert-sequence-before-with-new-requested";
const INSERT_SEQUENCE_AFTER_WITH_NEW_REQUESTED_EVENT: &str =
    "insert-sequence-after-with-new-requested";
const DELETE_SEQUENCE_POSITION_REQUESTED_EVENT: &str = "delete-sequence-position-requested";
const SET_PATTERN_LENGTH_REQUESTED_EVENT: &str = "set-pattern-length-requested";

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
    install_menu(app)?;
    app.on_menu_event(|app_handle, event| match event.id().as_ref() {
        OPEN_SETTINGS_MENU_ID => request_event(app_handle, OPEN_SETTINGS_REQUESTED_EVENT),
        QUIT_MENU_ID => request_event(app_handle, EXIT_REQUESTED_EVENT),
        NEW_MODULE_MENU_ID => request_event(app_handle, NEW_MODULE_REQUESTED_EVENT),
        NEW_MODULE_USING_DEFAULTS_MENU_ID => request_event(app_handle, NEW_MODULE_USING_DEFAULTS_REQUESTED_EVENT),
        OPEN_MODULE_MENU_ID => request_event(app_handle, OPEN_MODULE_REQUESTED_EVENT),
        SAVE_MODULE_MENU_ID => request_event(app_handle, SAVE_MODULE_REQUESTED_EVENT),
        SAVE_MODULE_AS_MENU_ID => request_event(app_handle, SAVE_MODULE_AS_REQUESTED_EVENT),
        EXPORT_AUDIO_MENU_ID => request_event(app_handle, EXPORT_AUDIO_REQUESTED_EVENT),
        EDIT_DETAILS_MENU_ID => request_event(app_handle, EDIT_DETAILS_REQUESTED_EVENT),
        SET_TRACK_COUNT_MENU_ID => request_event(app_handle, SET_TRACK_COUNT_REQUESTED_EVENT),
        SET_TEMPO_MENU_ID => request_event(app_handle, SET_TEMPO_REQUESTED_EVENT),
        UNDO_MENU_ID => request_event(app_handle, UNDO_REQUESTED_EVENT),
        REDO_MENU_ID => request_event(app_handle, REDO_REQUESTED_EVENT),
        TOGGLE_EDIT_MENU_ID => request_event(app_handle, TOGGLE_EDIT_REQUESTED_EVENT),
        CUT_EVENTS_MENU_ID => request_event(app_handle, CUT_EVENTS_REQUESTED_EVENT),
        COPY_EVENTS_MENU_ID => request_event(app_handle, COPY_EVENTS_REQUESTED_EVENT),
        PASTE_EVENTS_MENU_ID => request_event(app_handle, PASTE_EVENTS_REQUESTED_EVENT),
        CUT_TRACK_MENU_ID => request_event(app_handle, CUT_TRACK_REQUESTED_EVENT),
        COPY_TRACK_MENU_ID => request_event(app_handle, COPY_TRACK_REQUESTED_EVENT),
        PASTE_TRACK_MENU_ID => request_event(app_handle, PASTE_TRACK_REQUESTED_EVENT),
        CUT_PATTERN_MENU_ID => request_event(app_handle, CUT_PATTERN_REQUESTED_EVENT),
        COPY_PATTERN_MENU_ID => request_event(app_handle, COPY_PATTERN_REQUESTED_EVENT),
        PASTE_PATTERN_MENU_ID => request_event(app_handle, PASTE_PATTERN_REQUESTED_EVENT),
        INSERT_SEQUENCE_BEFORE_MENU_ID => {
            request_event(app_handle, INSERT_SEQUENCE_BEFORE_REQUESTED_EVENT)
        }
        INSERT_SEQUENCE_AFTER_MENU_ID => {
            request_event(app_handle, INSERT_SEQUENCE_AFTER_REQUESTED_EVENT)
        }
        INSERT_SEQUENCE_BEFORE_WITH_NEW_MENU_ID => {
            request_event(app_handle, INSERT_SEQUENCE_BEFORE_WITH_NEW_REQUESTED_EVENT)
        }
        INSERT_SEQUENCE_AFTER_WITH_NEW_MENU_ID => {
            request_event(app_handle, INSERT_SEQUENCE_AFTER_WITH_NEW_REQUESTED_EVENT)
        }
        DELETE_SEQUENCE_POSITION_MENU_ID => {
            request_event(app_handle, DELETE_SEQUENCE_POSITION_REQUESTED_EVENT)
        }
        SET_PATTERN_LENGTH_MENU_ID => request_event(app_handle, SET_PATTERN_LENGTH_REQUESTED_EVENT),
        _ => {}
    });
    Ok(())
}

fn install_menu<R: Runtime>(app: &App<R>) -> tauri::Result<()> {
    let app_menu = build_app_menu(app)?;
    let file_menu = build_file_menu(app)?;
    let edit_menu = build_edit_menu(app)?;
    let module_menu = build_module_menu(app)?;
    let sequence_menu = build_sequence_menu(app)?;
    let pattern_menu = build_pattern_menu(app)?;
    let menu = Menu::with_items(
        app,
        &[
            &app_menu,
            &file_menu,
            &edit_menu,
            &module_menu,
            &sequence_menu,
            &pattern_menu,
        ],
    )?;
    app.set_menu(menu)?;
    Ok(())
}

fn build_app_menu<R: Runtime>(app: &App<R>) -> tauri::Result<Submenu<R>> {
    let open_settings = MenuItem::with_id(
        app,
        OPEN_SETTINGS_MENU_ID,
        "Settings...",
        true,
        None::<String>,
    )?;
    let quit = MenuItem::with_id(
        app,
        QUIT_MENU_ID,
        "Quit Arctracker",
        true,
        Some("CmdOrCtrl+Q"),
    )?;
    let about = PredefinedMenuItem::about(app, Some("About Arctracker"), None)?;
    let separator = PredefinedMenuItem::separator(app)?;
    Submenu::with_items(app, "Arctracker", true, &[&about, &separator, &open_settings, &separator, &quit])
}

fn build_file_menu<R: Runtime>(app: &App<R>) -> tauri::Result<Submenu<R>> {
    let new_module = MenuItem::with_id(
        app,
        NEW_MODULE_MENU_ID,
        "New Module",
        true,
        Some("CmdOrCtrl+N"),
    )?;
    let new_module_using_defaults = MenuItem::with_id(
        app,
        NEW_MODULE_USING_DEFAULTS_MENU_ID,
        "New Module Using Defaults",
        true,
        Some("Shift+CmdOrCtrl+N"),
    )?;
    let open_module = MenuItem::with_id(
        app,
        OPEN_MODULE_MENU_ID,
        "Open Module...",
        true,
        Some("CmdOrCtrl+O"),
    )?;
    let save_module = MenuItem::with_id(
        app,
        SAVE_MODULE_MENU_ID,
        "Save Module",
        true,
        Some("CmdOrCtrl+S"),
    )?;
    let save_module_as = MenuItem::with_id(
        app,
        SAVE_MODULE_AS_MENU_ID,
        "Save Module As...",
        true,
        Some("Shift+CmdOrCtrl+S"),
    )?;
    let export_module_audio = MenuItem::with_id(
        app,
        EXPORT_AUDIO_MENU_ID,
        "Export Audio...",
        true,
        Some("CmdOrCtrl+B"),
    )?;
    let separator = PredefinedMenuItem::separator(app)?;
    Submenu::with_items(
        app,
        "File",
        true,
        &[
            &new_module,
            &new_module_using_defaults,
            &open_module,
            &save_module,
            &save_module_as,
            &separator,
            &export_module_audio,
        ],
    )
}

fn build_edit_menu<R: Runtime>(app: &App<R>) -> tauri::Result<Submenu<R>> {
    let undo = MenuItem::with_id(app, UNDO_MENU_ID, "Undo", true, Some("CmdOrCtrl+Z"))?;
    let redo = MenuItem::with_id(app, REDO_MENU_ID, "Redo", true, Some("Shift+CmdOrCtrl+Z"))?;
    let toggle_edit = MenuItem::with_id(app, TOGGLE_EDIT_MENU_ID, "Toggle Edit Mode", true, Some("Esc"))?;
    let cut_events = MenuItem::with_id(
        app,
        CUT_EVENTS_MENU_ID,
        "Cut Events",
        true,
        Some("CmdOrCtrl+X"),
    )?;
    let copy_events = MenuItem::with_id(
        app,
        COPY_EVENTS_MENU_ID,
        "Copy Events",
        true,
        Some("CmdOrCtrl+C"),
    )?;
    let paste_events = MenuItem::with_id(
        app,
        PASTE_EVENTS_MENU_ID,
        "Paste Events",
        true,
        Some("CmdOrCtrl+V"),
    )?;
    let cut_track = MenuItem::with_id(app, CUT_TRACK_MENU_ID, "Cut Track", true, Some("Shift+F3"))?;
    let copy_track = MenuItem::with_id(
        app,
        COPY_TRACK_MENU_ID,
        "Copy Track",
        true,
        Some("Shift+F4"),
    )?;
    let paste_track = MenuItem::with_id(
        app,
        PASTE_TRACK_MENU_ID,
        "Paste Track",
        true,
        Some("Shift+F5"),
    )?;
    let cut_pattern = MenuItem::with_id(
        app,
        CUT_PATTERN_MENU_ID,
        "Cut Pattern",
        true,
        Some("CmdOrCtrl+F3"),
    )?;
    let copy_pattern = MenuItem::with_id(
        app,
        COPY_PATTERN_MENU_ID,
        "Copy Pattern",
        true,
        Some("CmdOrCtrl+F4"),
    )?;
    let paste_pattern = MenuItem::with_id(
        app,
        PASTE_PATTERN_MENU_ID,
        "Paste Pattern",
        true,
        Some("CmdOrCtrl+F5"),
    )?;
    let separator = PredefinedMenuItem::separator(app)?;
    Submenu::with_items(
        app,
        "Edit",
        true,
        &[
            &undo,
            &redo,
            &separator,
            &toggle_edit,
            &separator,
            &cut_events,
            &copy_events,
            &paste_events,
            &separator,
            &cut_track,
            &copy_track,
            &paste_track,
            &separator,
            &cut_pattern,
            &copy_pattern,
            &paste_pattern,
        ],
    )
}

fn build_module_menu<R: Runtime>(app: &App<R>) -> tauri::Result<Submenu<R>> {
    let edit_details = MenuItem::with_id(
        app,
        EDIT_DETAILS_MENU_ID,
        "Edit Details",
        true,
        None::<String>,
    )?;
    let set_track_count = MenuItem::with_id(
        app,
        SET_TRACK_COUNT_MENU_ID,
        "Set Track Count",
        true,
        Some("CmdOrCtrl+T"),
    )?;
    let set_tempo = MenuItem::with_id(app, SET_TEMPO_MENU_ID, "Set Tempo", true, None::<String>)?;
    Submenu::with_items(
        app,
        "Module",
        true,
        &[&edit_details, &set_track_count, &set_tempo],
    )
}

fn build_sequence_menu<R: Runtime>(app: &App<R>) -> tauri::Result<Submenu<R>> {
    let insert_before = MenuItem::with_id(
        app,
        INSERT_SEQUENCE_BEFORE_MENU_ID,
        "Insert Position Before",
        true,
        Some("F1"),
    )?;
    let insert_after = MenuItem::with_id(
        app,
        INSERT_SEQUENCE_AFTER_MENU_ID,
        "Insert Position After",
        true,
        Some("F2"),
    )?;
    let insert_before_with_new = MenuItem::with_id(
        app,
        INSERT_SEQUENCE_BEFORE_WITH_NEW_MENU_ID,
        "Insert Before With New Pattern",
        true,
        Some("Shift+F1"),
    )?;
    let insert_after_with_new = MenuItem::with_id(
        app,
        INSERT_SEQUENCE_AFTER_WITH_NEW_MENU_ID,
        "Insert After With New Pattern",
        true,
        Some("Shift+F2"),
    )?;
    let delete_position = MenuItem::with_id(
        app,
        DELETE_SEQUENCE_POSITION_MENU_ID,
        "Delete Position",
        true,
        None::<String>,
    )?;
    Submenu::with_items(
        app,
        "Sequence",
        true,
        &[
            &insert_before,
            &insert_after,
            &insert_before_with_new,
            &insert_after_with_new,
            &delete_position,
        ],
    )
}

fn build_pattern_menu<R: Runtime>(app: &App<R>) -> tauri::Result<Submenu<R>> {
    let set_length = MenuItem::with_id(
        app,
        SET_PATTERN_LENGTH_MENU_ID,
        "Set Length",
        true,
        Some("CmdOrCtrl+L"),
    )?;
    Submenu::with_items(app, "Pattern", true, &[&set_length])
}

fn handle_run_event<R: Runtime>(app_handle: &AppHandle<R>, event: RunEvent) {
    match event {
        RunEvent::WindowEvent {
            event: WindowEvent::CloseRequested { api, .. },
            ..
        } => {
            api.prevent_close();
            request_event(app_handle, EXIT_REQUESTED_EVENT);
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
