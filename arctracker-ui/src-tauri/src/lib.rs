pub mod arctracker;
mod ffi;
pub mod state;

use std::sync::Arc;
use tauri::{AppHandle};
use crate::arctracker::{PatternLine, PlayerEvent, Module, UiExportState, PatternEvent};
use crate::arctracker::UiTransportState;
pub use crate::state::AppState;

#[tauri::command]
fn current_module(state: tauri::State<Arc<AppState>>) -> Result<Module, String> {
    let mut tracker = state.tracker.lock().unwrap();
    tracker.current_module()
}

#[tauri::command]
fn load_module(path: String, state: tauri::State<Arc<AppState>>) -> Result<Module, String> {
    let mut tracker = state.tracker.lock().unwrap();
    let info = tracker.load_module_with_info(&path);
    tracker.start().map_err(|e| e.message)?;
    info
}

#[tauri::command]
fn create_module(num_channels: i32, state: tauri::State<Arc<AppState>>) -> Result<Module, String> {
    let mut tracker = state.tracker.lock().unwrap();
    let info = tracker.create_module(num_channels).map_err(|e| e.message)?;
    tracker.start().map_err(|e| e.message)?;
    Ok(info)
}

#[tauri::command]
fn restart_player(state: tauri::State<Arc<AppState>>) -> Result<(), String> {
    let mut tracker = state.tracker.lock().unwrap();
    tracker.shutdown().map_err(|e| e.message)?;
    tracker.start().map_err(|e| e.message)?;
    Ok(())
}

#[tauri::command]
fn poll_playback_events(state: tauri::State<Arc<AppState>>) -> Result<Vec<PlayerEvent>, String> {
    let mut tracker = state.tracker.lock().map_err(|_| "Failed to lock Arctracker state".to_string())?;
    Ok(tracker.poll_playback_events())
}

#[tauri::command]
fn poll_export_events(state: tauri::State<Arc<AppState>>) -> Result<Vec<PlayerEvent>, String> {
    let mut tracker = state.tracker.lock().map_err(|_| "Failed to lock Arctracker state".to_string())?;
    Ok(tracker.poll_export_events())
}

#[tauri::command]
fn toggle_play(state: tauri::State<Arc<AppState>>) -> Result<(), String> {
    let mut tracker = state.tracker.lock().unwrap();
    tracker.toggle_play();
    Ok(())
}

#[tauri::command]
fn toggle_loop(state: tauri::State<Arc<AppState>>) -> Result<(), String> {
    let mut tracker = state.tracker.lock().unwrap();
    tracker.toggle_loop();
    Ok(())
}

#[tauri::command]
fn seek(state: tauri::State<Arc<AppState>>, new_sequence_pos: i32, new_pattern_pos: i32) -> Result<(), String> {
    let mut tracker = state.tracker.lock().unwrap();
    tracker.seek(new_sequence_pos, new_pattern_pos);
    Ok(())
}

#[tauri::command]
fn get_transport_state(state: tauri::State<Arc<AppState>>) -> UiTransportState {
    let mut tracker = state.tracker.lock().unwrap();
    tracker.get_transport_state()
}

#[tauri::command]
fn get_pattern(
    pattern_no: i32,
    num_lines: i32,
    num_channels: i32,
    state: tauri::State<Arc<AppState>>,
) -> Vec<PatternLine> {
    let mut tracker = state.tracker.lock().unwrap();
    tracker.get_pattern(pattern_no, num_lines, num_channels)
}

#[tauri::command]
fn set_selected_sample(
    sample_no: i32,
    state: tauri::State<Arc<AppState>>,
) {
    let mut editor = state.editor.lock().unwrap();
    editor.selected_sample = sample_no;
}

#[tauri::command]
fn set_selected_channel(
    channel_no: i32,
    state: tauri::State<Arc<AppState>>,
) {
    let mut editor = state.editor.lock().unwrap();
    editor.selected_channel = channel_no;
}

#[tauri::command]
fn default_export_path(module_path: String) -> String {
    std::path::Path::new(&module_path)
        .with_extension("wav")
        .to_string_lossy()
        .to_string()
}

#[tauri::command]
fn export_audio(export_path: String, state: tauri::State<Arc<AppState>>) -> Result<(), String> {
    let mut tracker = state.tracker.lock().unwrap();
    tracker.export_audio(&export_path).map_err(|e| e.message)?;
    Ok(())
}

#[tauri::command]
fn get_export_state(state: tauri::State<Arc<AppState>>) -> UiExportState {
    let mut tracker = state.tracker.lock().unwrap();
    tracker.get_export_state()
}

#[tauri::command]
fn export_cleanup(state: tauri::State<Arc<AppState>>) -> Result<(), String> {
    let mut tracker = state.tracker.lock().unwrap();
    tracker.export_cleanup().map_err(|e| e.message)?;
    Ok(())
}

#[tauri::command]
fn keyboard_note_on(state: tauri::State<Arc<AppState>>, note: i32) -> Result<(), String> {
    let mut tracker = state.tracker.lock().unwrap();
    let editor = state.editor.lock().unwrap();
    tracker.keyboard_note_on(note, editor.selected_sample, editor.selected_channel);
    Ok(())
}

#[tauri::command]
fn edit_get_event(state: tauri::State<Arc<AppState>>, pattern_no: i32, pattern_index: i32, channel_no: i32) -> Result<PatternEvent, String> {
    let mut tracker = state.tracker.lock().unwrap();
    let result = tracker.edit_get_event(pattern_no, pattern_index, channel_no).map_err(|e| e.message)?;
    Ok(result)
}

#[tauri::command]
fn edit_set_event(state: tauri::State<Arc<AppState>>, pattern_no: i32, pattern_index: i32, channel_no: i32, new_event: PatternEvent) -> Result<(), String> {
    let mut tracker = state.tracker.lock().unwrap();
    tracker.edit_set_event(pattern_no, pattern_index, channel_no, new_event).map_err(|e| e.message)?;
    Ok(())
}

#[tauri::command]
fn edit_get_sequence(state: tauri::State<Arc<AppState>>, expected_sequence_len: i32) -> Result<Vec<i32>, String> {
    let mut tracker = state.tracker.lock().unwrap();
    let result = tracker.edit_get_sequence(expected_sequence_len).map_err(|e| e.message)?;
    Ok(result)
}

#[tauri::command]
fn edit_set_sequence(state: tauri::State<Arc<AppState>>, new_sequence: Vec<i32>) -> Result<(), String> {
    let mut tracker = state.tracker.lock().unwrap();
    tracker.edit_set_sequence(&new_sequence).map_err(|e| e.message)?;
    Ok(())
}

#[tauri::command]
fn edit_create_pattern(state: tauri::State<Arc<AppState>>, pattern_length: i32) -> Result<i32, String> {
    let mut tracker = state.tracker.lock().unwrap();
    let pattern_no = tracker.edit_create_pattern(pattern_length).map_err(|e| e.message)?;
    Ok(pattern_no)
}

#[tauri::command]
fn edit_delete_pattern(state: tauri::State<Arc<AppState>>, pattern_no: i32) -> Result<(), String> {
    let mut tracker = state.tracker.lock().unwrap();
    tracker.edit_delete_pattern(pattern_no).map_err(|e| e.message)?;
    Ok(())
}

#[tauri::command]
fn shutdown_app(app: AppHandle) {
    app.exit(1);
}

pub fn build_app(app_state: Arc<AppState>) -> tauri::Builder<tauri::Wry> {
    tauri::Builder::default()
        .plugin(tauri_plugin_dialog::init())
        .manage(app_state)
        .invoke_handler(tauri::generate_handler![
            current_module,
            load_module,
            create_module,
            restart_player,
            poll_playback_events,
            poll_export_events,
            toggle_play,
            toggle_loop,
            seek,
            get_transport_state,
            get_pattern,
            set_selected_sample,
            set_selected_channel,
            shutdown_app,
            default_export_path,
            export_audio,
            get_export_state,
            export_cleanup,
            keyboard_note_on,
            edit_get_event,
            edit_set_event,
            edit_get_sequence,
            edit_set_sequence,
            edit_create_pattern,
            edit_delete_pattern,
        ])
}