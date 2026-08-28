pub mod arctracker;
mod ffi;
pub mod state;

use crate::arctracker::{default_module_params, AudioDeviceInfo, InterpolationType, MidiDeviceInfo, NewModuleParams, UiPlayerSnapshot, VolumeMappingType};
use crate::arctracker::{
    InstrumentUpdate, Module, PatternEvent, PatternLine, PlayerEvent, Sample, UiExportState,
    UiPeakLevels,
};
pub use crate::state::AppState;
use std::sync::Arc;
use tauri::AppHandle;

#[tauri::command]
fn get_available_outputs(state: tauri::State<Arc<AppState>>) -> Result<Vec<AudioDeviceInfo>, String> {
    let mut tracker = state.tracker.lock().unwrap();
    tracker.get_available_outputs()
}

#[tauri::command]
fn use_output(state: tauri::State<Arc<AppState>>, device_index: i32, name: String, host_api_name: String) -> Result<(), String> {
    let mut tracker = state.tracker.lock().unwrap();
    tracker.use_output(device_index, &name, &host_api_name).map_err(|e| e)?;
    Ok(())
}

#[tauri::command]
fn use_default_output(state: tauri::State<Arc<AppState>>) -> Result<(), String> {
    let mut tracker = state.tracker.lock().unwrap();
    tracker.use_default_output().map_err(|e| e)?;
    Ok(())
}

#[tauri::command]
fn get_available_midi_devices(state: tauri::State<Arc<AppState>>) -> Result<Vec<MidiDeviceInfo>, String> {
    let mut midi = state.midi.lock().unwrap();
    if let Some(midi) = midi.as_mut() {
        let midi_devices = midi.get_available_midi_devices().map_err(|e| e)?;
        return Ok(midi_devices);
    }
    Ok(vec![])
}

#[tauri::command]
fn use_midi_device(state: tauri::State<Arc<AppState>>, device_name: String) -> Result<(), String> {
    let mut midi = state.midi.lock().unwrap();
    if let Some(midi) = midi.as_mut() {
        midi.use_midi_device(&device_name).map_err(|e| e)?;
    }
    Ok(())
}

#[tauri::command]
fn set_midi_playback_channel(state: tauri::State<Arc<AppState>>, channel: i32) {
    let mut midi = state.midi.lock().unwrap();
    if let Some(midi) = midi.as_mut() {
        midi.set_playback_channel(channel);
    }
}

#[tauri::command]
fn set_midi_playback_instrument(state: tauri::State<Arc<AppState>>, instrument: u8) {
    let mut midi = state.midi.lock().unwrap();
    if let Some(midi) = midi.as_mut() {
        midi.set_playback_instrument(instrument);
    }
}

#[tauri::command]
fn current_module(state: tauri::State<Arc<AppState>>) -> Result<Module, String> {
    let mut tracker = state.tracker.lock().unwrap();
    tracker.current_module()
}

#[tauri::command]
fn load_module(path: String, state: tauri::State<Arc<AppState>>) -> Result<Module, String> {
    let mut tracker = state.tracker.lock().unwrap();
    let info = tracker.load_module_with_info(&path).map_err(|e| e)?;
    tracker.start().map_err(|e| e.message)?;
    Ok(info)
}

#[tauri::command]
fn create_module(state: tauri::State<Arc<AppState>>) -> Result<Module, String> {
    let mut tracker = state.tracker.lock().unwrap();
    let info = tracker.create_module(default_module_params()).map_err(|e| e.message)?;
    tracker.start().map_err(|e| e.message)?;
    Ok(info)
}

#[tauri::command]
fn create_module_using_defaults(params: NewModuleParams, state: tauri::State<Arc<AppState>>) -> Result<Module, String> {
    let mut tracker = state.tracker.lock().unwrap();
    let info = tracker.create_module(params).map_err(|e| e.message)?;
    tracker.start().map_err(|e| e.message)?;
    Ok(info)
}

#[tauri::command]
fn save_module(
    path: String,
    format: i32,
    state: tauri::State<Arc<AppState>>,
) -> Result<(), String> {
    let mut tracker = state.tracker.lock().unwrap();
    tracker.save_module(&path, format)?;
    Ok(())
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
    let mut tracker = state
        .tracker
        .lock()
        .map_err(|_| "Failed to lock Arctracker state".to_string())?;
    Ok(tracker.poll_playback_events())
}

#[tauri::command]
fn poll_export_events(state: tauri::State<Arc<AppState>>) -> Result<Vec<PlayerEvent>, String> {
    let mut tracker = state
        .tracker
        .lock()
        .map_err(|_| "Failed to lock Arctracker state".to_string())?;
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
fn seek(
    state: tauri::State<Arc<AppState>>,
    new_sequence_pos: i32,
    new_pattern_pos: i32,
) -> Result<(), String> {
    let mut tracker = state.tracker.lock().unwrap();
    tracker.seek(new_sequence_pos, new_pattern_pos);
    Ok(())
}

#[tauri::command]
fn toggle_track_mute(state: tauri::State<Arc<AppState>>, track: i32) -> Result<(), String> {
    let mut tracker = state.tracker.lock().unwrap();
    tracker.toggle_track_mute(track);
    Ok(())
}

#[tauri::command]
fn set_effects_displayed(
    state: tauri::State<Arc<AppState>>,
    track: i32,
    effects_displayed: i32,
) -> Result<(), String> {
    let mut tracker = state.tracker.lock().unwrap();
    tracker.set_effects_displayed(track, effects_displayed);
    Ok(())
}

#[tauri::command]
fn get_player_snapshot(
    state: tauri::State<Arc<AppState>>,
    displayed_pattern_no: Option<u32>,
    num_tracks: i32,
) -> UiPlayerSnapshot {
    let mut tracker = state.tracker.lock().unwrap();
    tracker.get_player_snapshot(displayed_pattern_no, num_tracks)
}

#[tauri::command]
fn get_and_reset_peak_levels(state: tauri::State<Arc<AppState>>) -> UiPeakLevels {
    let mut tracker = state.tracker.lock().unwrap();
    tracker.get_and_reset_peak_levels()
}

#[tauri::command]
fn get_pattern(
    pattern_no: i32,
    num_lines: i32,
    num_tracks: i32,
    state: tauri::State<Arc<AppState>>,
) -> Vec<PatternLine> {
    let mut tracker = state.tracker.lock().unwrap();
    tracker.get_pattern(pattern_no, num_lines, num_tracks)
}

#[tauri::command]
fn default_save_path(module_path: String) -> String {
    std::path::Path::new(&module_path)
        .with_extension("arctm")
        .to_string_lossy()
        .to_string()
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
fn export_sample(instrument_no: i32, export_path: String, state: tauri::State<Arc<AppState>>) -> Result<(), String> {
    let mut tracker = state.tracker.lock().unwrap();
    tracker.export_sample(instrument_no, &export_path).map_err(|e| e.message)?;
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
    let mut midi = state.midi.lock().unwrap();
    if let Some(midi) = midi.as_mut() {
        midi.keyboard_note_on(note);
    }
    Ok(())
}

#[tauri::command]
fn set_master_gain(state: tauri::State<Arc<AppState>>, master_gain: f32) -> Result<(), String> {
    let mut tracker = state.tracker.lock().unwrap();
    tracker.set_master_gain(master_gain);
    Ok(())
}

#[tauri::command]
fn edit_get_event(
    state: tauri::State<Arc<AppState>>,
    pattern_no: i32,
    pattern_index: i32,
    track: i32,
) -> Result<PatternEvent, String> {
    let mut tracker = state.tracker.lock().unwrap();
    let result = tracker
        .edit_get_event(pattern_no, pattern_index, track)
        .map_err(|e| e.message)?;
    Ok(result)
}

#[tauri::command]
fn edit_set_event(
    state: tauri::State<Arc<AppState>>,
    pattern_no: i32,
    pattern_index: i32,
    track: i32,
    new_event: PatternEvent,
) -> Result<(), String> {
    let mut tracker = state.tracker.lock().unwrap();
    tracker
        .edit_set_event(pattern_no, pattern_index, track, new_event)
        .map_err(|e| e.message)?;
    Ok(())
}

#[tauri::command]
fn edit_get_sequence(
    state: tauri::State<Arc<AppState>>,
    expected_sequence_len: i32,
) -> Result<Vec<i32>, String> {
    let mut tracker = state.tracker.lock().unwrap();
    let result = tracker
        .edit_get_sequence(expected_sequence_len)
        .map_err(|e| e.message)?;
    Ok(result)
}

#[tauri::command]
fn edit_set_sequence(
    state: tauri::State<Arc<AppState>>,
    new_sequence: Vec<i32>,
) -> Result<(), String> {
    let mut tracker = state.tracker.lock().unwrap();
    tracker
        .edit_set_sequence(&new_sequence)
        .map_err(|e| e.message)?;
    Ok(())
}

#[tauri::command]
fn edit_create_pattern(
    state: tauri::State<Arc<AppState>>,
    pattern_length: i32,
) -> Result<i32, String> {
    let mut tracker = state.tracker.lock().unwrap();
    let pattern_no = tracker
        .edit_create_pattern(pattern_length)
        .map_err(|e| e.message)?;
    Ok(pattern_no)
}

#[tauri::command]
fn edit_delete_pattern(state: tauri::State<Arc<AppState>>, pattern_no: i32) -> Result<(), String> {
    let mut tracker = state.tracker.lock().unwrap();
    tracker
        .edit_delete_pattern(pattern_no)
        .map_err(|e| e.message)?;
    Ok(())
}

#[tauri::command]
fn edit_set_pattern_length(
    state: tauri::State<Arc<AppState>>,
    pattern_no: i32,
    new_length: i32,
) -> Result<(), String> {
    let mut tracker = state.tracker.lock().unwrap();
    tracker
        .edit_set_pattern_length(pattern_no, new_length)
        .map_err(|e| e.message)?;
    Ok(())
}

#[tauri::command]
fn edit_update_instrument(
    state: tauri::State<Arc<AppState>>,
    instrument_index: u8,
    instrument_update: InstrumentUpdate,
) -> Result<(), String> {
    let mut tracker = state.tracker.lock().unwrap();
    tracker
        .edit_update_instrument(instrument_index, instrument_update)
        .map_err(|e| e.message)?;
    Ok(())
}

#[tauri::command]
fn edit_load_sample(state: tauri::State<Arc<AppState>>, path: String) -> Result<Sample, String> {
    let mut tracker = state.tracker.lock().unwrap();
    let sample = tracker.edit_load_sample(&path).map_err(|e| e.message)?;
    Ok(sample)
}

#[tauri::command]
fn edit_set_module_meta_data(
    state: tauri::State<Arc<AppState>>,
    name: String,
    author: String,
    default_pattern_length: u16,
    interpolation_type: InterpolationType,
    volume_mapping_type: VolumeMappingType,
) -> Result<(), String> {
    let mut tracker = state.tracker.lock().unwrap();
    tracker
        .edit_set_module_meta_data(name, author, default_pattern_length, interpolation_type, volume_mapping_type)
        .map_err(|e| e.message)?;
    Ok(())
}

#[tauri::command]
fn edit_set_num_tracks(state: tauri::State<Arc<AppState>>, num_tracks: i32) -> Result<(), String> {
    let mut tracker = state.tracker.lock().unwrap();
    tracker
        .edit_set_num_tracks(num_tracks)
        .map_err(|e| e.message)?;
    Ok(())
}

#[tauri::command]
fn edit_set_tempo(
    state: tauri::State<Arc<AppState>>,
    lines_per_beat: u8,
    beats_per_minute: u8,
) -> Result<(), String> {
    let mut tracker = state.tracker.lock().unwrap();
    tracker
        .edit_set_tempo(lines_per_beat, beats_per_minute)
        .map_err(|e| e.message)?;
    Ok(())
}

#[tauri::command]
fn exit_successfully(app: AppHandle) {
    app.exit(0);
}

#[tauri::command]
fn exit_unsuccessfully(app: AppHandle) {
    app.exit(1);
}

pub fn build_app(app_state: Arc<AppState>) -> tauri::Builder<tauri::Wry> {
    tauri::Builder::default()
        .plugin(tauri_plugin_fs::init())
        .plugin(tauri_plugin_store::Builder::new().build())
        .plugin(tauri_plugin_dialog::init())
        .manage(app_state)
        .invoke_handler(tauri::generate_handler![
            get_available_outputs,
            use_output,
            use_default_output,
            get_available_midi_devices,
            use_midi_device,
            set_midi_playback_channel,
            set_midi_playback_instrument,
            current_module,
            load_module,
            save_module,
            create_module,
            create_module_using_defaults,
            restart_player,
            poll_playback_events,
            poll_export_events,
            toggle_play,
            toggle_loop,
            seek,
            toggle_track_mute,
            set_effects_displayed,
            get_player_snapshot,
            get_and_reset_peak_levels,
            get_pattern,
            default_save_path,
            default_export_path,
            export_audio,
            export_sample,
            get_export_state,
            export_cleanup,
            keyboard_note_on,
            set_master_gain,
            edit_get_event,
            edit_set_event,
            edit_get_sequence,
            edit_set_sequence,
            edit_create_pattern,
            edit_delete_pattern,
            edit_set_pattern_length,
            edit_update_instrument,
            edit_load_sample,
            edit_set_module_meta_data,
            edit_set_num_tracks,
            edit_set_tempo,
            exit_successfully,
            exit_unsuccessfully,
        ])
}
