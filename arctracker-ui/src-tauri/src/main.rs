mod midi;

use arctracker_ui_lib::arctracker::Arctracker;
use arctracker_ui_lib::state::EditorState;
use arctracker_ui_lib::AppState;
use std::sync::{Arc, Mutex};

fn main() {
    let mut tracker = Arctracker::new().expect("failed to create tracker");
    tracker.create_module(8).expect("failed to initialise module");
    tracker.start().expect("failed to initialise audio subsystem");
    let app_state = Arc::new(AppState {
        tracker: Arc::new(Mutex::new(tracker)),
        editor: Mutex::new(EditorState {
            selected_sample: 0,
            selected_channel: 0,
        }),
    });
    let midi_state = app_state.clone();
    std::thread::spawn(move || {
        midi::start_midi_thread(midi_state);
    });
    arctracker_ui_lib::build_app(app_state)
        .run(tauri::generate_context!())
        .expect("error while running tauri app");
}
