use std::os::raw::{c_char, c_int};

#[repr(C)]
pub struct ArctrackerHandle {
    _private: [u8; 0],
}

#[derive(Copy, Clone)]
#[repr(i32)]
pub enum PlayerEventType {
    PLAYER_ERROR = 0,
    USER_MIDI_NOTE_ON = 1,
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
    TOGGLE_PLAY = 0,
    SEEK = 1,
    MIDI_NOTE_ON = 2,
    MIDI_NOTE_OFF = 3,
    KEYBOARD_NOTE_ON = 4,
    KEYBOARD_NOTE_OFF = 5,
    TOGGLE_LOOP = 6,
}

#[repr(C)]
pub struct PlayerCommand {
    pub cmd_type: PlayerCommandType,
    pub new_sequence_pos: c_int,
    pub new_pattern_pos: c_int,
    pub channel_no: c_int,
    pub note: c_int,
    pub sample_no: c_int,
}

#[repr(C)]
pub struct UiSampleInfo {
    pub name: [c_char; 33],
    pub default_gain: c_int,
    pub sample_length: c_int,
    pub repeats: bool,
    pub repeat_offset: c_int,
    pub repeat_length: c_int,
    pub transpose: c_int,
}

#[repr(C)]
pub struct UiModuleInfo {
    pub name: [c_char; 65],
    pub author: [c_char; 65],
    pub num_channels: c_int,
    pub tune_length: c_int,
    pub num_samples: c_int,
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
    pub fn arctracker_module_create(
        handle: *mut ArctrackerHandle,
        num_channels: c_int,
        module_info: *mut UiModuleInfo,
    ) -> ApiResult;
    pub fn arctracker_get_sample_info(
        handle: *mut ArctrackerHandle,
        sample_no: c_int,
        sample_info: *mut UiSampleInfo,
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
        requested_channels: c_int,
    );
    pub fn arctracker_export_audio(
        handle: *mut ArctrackerHandle,
        output_filename: *const c_char,
    ) -> ApiResult;
    pub fn arctracker_get_export_state(handle: *mut ArctrackerHandle, export_state: *mut UiExportState);
    pub fn arctracker_export_cleanup(handle: *mut ArctrackerHandle) -> ApiResult;
    pub fn arctracker_edit_get_event(handle: *mut ArctrackerHandle, pattern_no: c_int, pattern_index: c_int, channel_no: c_int, event: *mut UiPatternEvent) -> ApiResult;
    pub fn arctracker_edit_set_event(handle: *mut ArctrackerHandle, pattern_no: c_int, pattern_index: c_int, channel_no: c_int, event: *mut UiPatternEvent) -> ApiResult;
    pub fn arctracker_edit_get_sequence(handle: *mut ArctrackerHandle, sequence: *mut c_int, expected_sequence_len: c_int) -> ApiResult;
    pub fn arctracker_edit_set_sequence(handle: *mut ArctrackerHandle, new_sequence: *const c_int, new_sequence_len: c_int) -> ApiResult;
    pub fn arctracker_player_shutdown(handle: *mut ArctrackerHandle) -> ApiResult;
    pub fn arctracker_destroy(handle: *mut ArctrackerHandle) -> ApiResult;
}
