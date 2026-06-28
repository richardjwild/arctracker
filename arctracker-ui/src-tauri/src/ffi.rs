use std::os::raw::{c_char, c_int};

#[repr(C)]
pub struct ArctrackerHandle {
    _private: [u8; 0],
}

#[derive(Copy, Clone)]
#[repr(i32)]
pub enum PlayerEventType {
    PlayerError = 0,
    UserMidiNoteOn = 1,
    AudioOverflowed = 2,
}

#[repr(C)]
pub struct PlayerEvent {
    pub event_type: PlayerEventType,
    pub error_message: [c_char; 256],
    pub midi_note: c_int,
}

#[derive(Copy, Clone)]
#[repr(i32)]
pub enum PlayerCommandType {
    TogglePlay = 0,
    Seek = 1,
    MidiNoteOn = 2,
    MidiNoteOff = 3,
    KeyboardNoteOn = 4,
    KeyboardNoteOff = 5,
    ToggleLoop = 6,
}

#[repr(C)]
pub struct PlayerCommand {
    pub cmd_type: PlayerCommandType,
    pub new_sequence_pos: c_int,
    pub new_pattern_pos: c_int,
    pub track: c_int,
    pub note: c_int,
    pub instrument_no: u8,
}

#[repr(C)]
pub struct UiSampleInfo {
    pub sample_index: c_int,
    pub sample_length: c_int,
}

#[repr(C)]
pub struct UiInstrumentInfo {
    pub assigned: bool,
    pub name: [c_char; 33],
    pub default_volume: c_int,
    pub transpose: c_int,
    pub repeats: bool,
    pub repeat_offset: c_int,
    pub repeat_length: c_int,
    pub sample_info: UiSampleInfo,
}

#[repr(C)]
pub struct UiInstrumentUpdate {
    pub assigned: bool,
    pub name: [c_char; 33],
    pub default_volume: c_int,
    pub transpose: c_int,
    pub sample_index: c_int,
    pub repeats: bool,
    pub repeat_offset: c_int,
    pub repeat_length: c_int,
}

#[repr(C)]
pub struct UiModuleInfo {
    pub name: [c_char; 65],
    pub author: [c_char; 65],
    pub num_tracks: c_int,
    pub tune_length: c_int,
    pub num_patterns: c_int,
}

#[repr(C)]
#[derive(Clone, Copy, Debug)]
pub struct UiEffect {
    pub effect_code: c_char,
    pub effect_data: [c_int; 2],
}

#[repr(C)]
#[derive(Clone, Copy)]
pub struct UiPatternEvent {
    pub note: c_int,
    pub sample_no: c_int,
    pub effects: [UiEffect; 4],
}

#[repr(C)]
pub struct UiTransportState {
    pub playing: bool,
    pub looping: bool,
    pub sequence_pos: c_int,
    pub pattern_index: c_int,
    pub pattern_no: c_int,
    pub pattern_length: c_int,
}

#[repr(C)]
pub struct UiExportState {
    pub completed: bool,
    pub percent_complete: c_int,
}

#[repr(C)]
pub struct ApiResult {
    pub success: bool,
    pub error_message: [c_char; 256],
}

#[link(name = "arctracker")]
extern "C" {
    pub fn arctracker_create() -> *mut ArctrackerHandle;
    pub fn arctracker_get_current_module(
        handle: *mut ArctrackerHandle,
        module_info: *mut UiModuleInfo,
    ) -> ApiResult;
    pub fn arctracker_module_load(
        handle: *mut ArctrackerHandle,
        mod_filename: *const c_char,
        module_info: *mut UiModuleInfo,
    ) -> ApiResult;
    pub fn arctracker_module_save(
        handle: *mut ArctrackerHandle,
        mod_filename: *const c_char,
        format: c_int
    ) -> ApiResult;
    pub fn arctracker_module_create(
        handle: *mut ArctrackerHandle,
        num_tracks: c_int,
        module_info: *mut UiModuleInfo,
    ) -> ApiResult;
    pub fn arctracker_get_instrument_info(
        handle: *mut ArctrackerHandle,
        slot: u8,
        instrument_info: *mut UiInstrumentInfo,
    ) -> ApiResult;
    pub fn arctracker_get_pattern_lengths(
        handle: *mut ArctrackerHandle,
        pattern_lengths: *mut c_int,
        num_patterns: c_int
    ) -> ApiResult;
    pub fn arctracker_player_start(handle: *mut ArctrackerHandle) -> ApiResult;
    pub fn arctracker_player_cmd(
        handle: *mut ArctrackerHandle,
        command: *const PlayerCommand,
    ) -> bool;
    pub fn arctracker_poll_playback_event(handle: *mut ArctrackerHandle, event: *mut PlayerEvent) -> bool;
    pub fn arctracker_poll_export_event(handle: *mut ArctrackerHandle, event: *mut PlayerEvent) -> bool;
    pub fn arctracker_get_transport_state(
        handle: *mut ArctrackerHandle,
        out_state: *mut UiTransportState,
    );
    pub fn arctracker_get_pattern(
        handle: *mut ArctrackerHandle,
        pattern_no: c_int,
        pattern_buffer: *mut UiPatternEvent,
        requested_lines: c_int,
        requested_tracks: c_int,
    );
    pub fn arctracker_export_audio(
        handle: *mut ArctrackerHandle,
        output_filename: *const c_char,
    ) -> ApiResult;
    pub fn arctracker_get_export_state(handle: *mut ArctrackerHandle, export_state: *mut UiExportState);
    pub fn arctracker_export_cleanup(handle: *mut ArctrackerHandle) -> ApiResult;
    pub fn arctracker_edit_get_event(handle: *mut ArctrackerHandle, pattern_no: c_int, pattern_index: c_int, track: c_int, event: *mut UiPatternEvent) -> ApiResult;
    pub fn arctracker_edit_set_event(handle: *mut ArctrackerHandle, pattern_no: c_int, pattern_index: c_int, track: c_int, event: *mut UiPatternEvent) -> ApiResult;
    pub fn arctracker_edit_get_sequence(handle: *mut ArctrackerHandle, sequence: *mut c_int, expected_sequence_len: c_int) -> ApiResult;
    pub fn arctracker_edit_set_sequence(handle: *mut ArctrackerHandle, new_sequence: *const c_int, new_sequence_len: c_int) -> ApiResult;
    pub fn arctracker_edit_create_pattern(handle: *mut ArctrackerHandle, pattern_length: c_int, new_pattern_no: *mut c_int) -> ApiResult;
    pub fn arctracker_edit_delete_pattern(handle: *mut ArctrackerHandle, pattern_no: c_int) -> ApiResult;
    pub fn arctracker_edit_set_pattern_length(handle: *mut ArctrackerHandle, pattern_no: c_int, new_length: c_int) -> ApiResult;
    pub fn arctracker_edit_set_instrument(handle: *mut ArctrackerHandle, slot: u8, new_instrument: UiInstrumentUpdate) -> ApiResult;
    pub fn arctracker_edit_load_sample(handle: *mut ArctrackerHandle, filename: *const c_char, sample_info: *mut UiSampleInfo) -> ApiResult;
    pub fn arctracker_edit_set_module_title(handle: *mut ArctrackerHandle, name: *const c_char, author: *const c_char) -> ApiResult;
    pub fn arctracker_edit_set_num_tracks(handle: *mut ArctrackerHandle, num_tracks: c_int) -> ApiResult;
    pub fn arctracker_player_shutdown(handle: *mut ArctrackerHandle) -> ApiResult;
    pub fn arctracker_destroy(handle: *mut ArctrackerHandle) -> ApiResult;
}
