use crate::ffi;
use std::ffi::{c_char, c_int, CStr, CString};
use serde::{Deserialize, Serialize};

pub struct Arctracker {
    handle: *mut ffi::ArctrackerHandle,
}

#[derive(Serialize)]
#[serde(rename_all = "camelCase")]
pub enum PlayerEventType {
    PlayerError = 0,
    UserMidiNoteOn = 1,
}

impl From<ffi::PlayerEventType> for PlayerEventType {
    fn from(event_type: ffi::PlayerEventType) -> Self {
        match event_type {
            ffi::PlayerEventType::PLAYER_ERROR => {
                PlayerEventType::PlayerError
            }
            ffi::PlayerEventType::USER_MIDI_NOTE_ON => {
                PlayerEventType::UserMidiNoteOn
            }
        }
    }
}

#[derive(Serialize)]
#[serde(rename_all = "camelCase")]
pub struct PlayerEvent {
    pub event_type: PlayerEventType,
    pub error_message: String,
    pub midi_note: i32,
}

impl From<ffi::PlayerEvent> for PlayerEvent {
    fn from(event: ffi::PlayerEvent) -> Self {
        Self {
            event_type: event.event_type.into(),
            error_message: c_string_to_rust(&event.error_message),
            midi_note: event.midi_note,
        }
    }
}

unsafe impl Send for Arctracker {}

pub enum PlayerCommandType {
    TogglePlay = 0,
    Seek = 1,
    MidiNoteOn = 2,
    MidiNoteOff = 3,
    KeyboardNoteOn = 4,
    KeyboardNoteOff = 5,
    ToggleLoop = 6,
}

pub struct PlayerCommand {
    pub cmd_type: PlayerCommandType,
    pub new_sequence_pos: i32,
    pub new_pattern_pos: i32,
    pub channel_no: i32,
    pub note: i32,
    pub sample_no: i32,
}

#[derive(Serialize)]
#[serde(rename_all = "camelCase")]
pub struct Sample {
    pub(crate) name: String,
    pub default_gain: i32,
    pub sample_length: i32,
    pub repeats: bool,
    pub repeat_offset: i32,
    pub repeat_length: i32,
    pub transpose: i32,
}

#[derive(Serialize)]
#[serde(rename_all = "camelCase")]
pub struct Module {
    pub(crate) name: String,
    pub(crate) author: String,
    pub num_channels: i32,
    pub tune_length: i32,
    pub num_samples: i32,
    pub num_patterns: i32,
    pub pattern_lengths: Vec<i32>,
    pub samples: Vec<Sample>,
}

#[derive(Serialize, Deserialize, Clone, Debug)]
#[serde(rename_all = "camelCase")]
pub struct Effect {
    pub effect_code: String,
    pub effect_data: [i32; 2],
}

#[derive(Serialize, Deserialize, Clone, Debug)]
#[serde(rename_all = "camelCase")]
pub struct PatternEvent {
    pub note: i32,
    pub sample_no: i32,
    pub effects: Vec<Effect>,
}

#[derive(Serialize, Clone, Debug)]
#[serde(rename_all = "camelCase")]
pub struct PatternLine {
    pub row: i32,
    pub events: Vec<PatternEvent>,
}

impl From<ffi::UiEffect> for Effect {
    fn from(effect: ffi::UiEffect) -> Self {
        let code = effect.effect_code as u8 as char;
        Self {
            effect_code: if code == '\0' {
                String::new()
            } else {
                code.to_string()
            },
            effect_data: effect.effect_data,
        }
    }
}

impl TryFrom<Effect> for ffi::UiEffect {
    type Error = String;
    fn try_from(effect: Effect) -> Result<Self, Self::Error> {
        let code = match effect.effect_code.as_str() {
            "" => 0,
            s if s.len() == 1 => s.as_bytes()[0],
            other => return Err(format!("Invalid effect code: {other}")),
        };
        Ok(Self {
            effect_code: code as c_char,
            effect_data: effect.effect_data,
        })
    }
}

impl From<ffi::UiPatternEvent> for PatternEvent {
    fn from(event: ffi::UiPatternEvent) -> Self {
        Self {
            note: event.note,
            sample_no: event.sample_no,
            effects: event.effects.into_iter().map(Effect::from).collect(),
        }
    }
}

impl TryFrom<PatternEvent> for ffi::UiPatternEvent {
    type Error = String;
    fn try_from(event: PatternEvent) -> Result<Self, Self::Error> {
        let effects: Vec<ffi::UiEffect> = event
            .effects
            .into_iter()
            .map(ffi::UiEffect::try_from)
            .collect::<Result<Vec<_>, _>>()?;
        let effects: [ffi::UiEffect; 4] = effects
            .try_into()
            .map_err(|_| "PatternEvent must contain exactly 4 effects".to_string())?;
        Ok(Self {
            note: event.note,
            sample_no: event.sample_no,
            effects,
        })
    }
}

#[derive(Serialize)]
#[serde(rename_all = "camelCase")]
pub struct UiTransportState {
    pub playing: bool,
    pub looping: bool,
    pub sequence_pos: i32,
    pub pattern_index: i32,
    pub pattern_no: i32,
    pub pattern_length: i32,
}

#[derive(Serialize, Clone, Debug)]
#[serde(rename_all = "camelCase")]
pub struct UiExportState {
    pub completed: bool,
    pub percent_complete: i32,
}

pub struct ApiResult {
    SUCCESS: bool,
    ERROR_MESSAGE: String,
}

#[derive(Debug)]
pub struct ArctrackerError {
    pub message: String,
}

fn c_string_to_rust(buffer: &[c_char]) -> String {
    unsafe {
        CStr::from_ptr(buffer.as_ptr())
            .to_string_lossy()
            .into_owned()
    }
}

impl Arctracker {
    pub fn new() -> Result<Self, ArctrackerError> {
        let handle = unsafe { ffi::arctracker_create() };
        if handle.is_null() {
            Err(ArctrackerError {
                message: "Failed to create Arctracker".into(),
            })
        } else {
            Ok(Self { handle })
        }
    }

    pub fn current_module(&mut self) -> Result<Module, String> {
        let mut module = std::mem::MaybeUninit::<ffi::UiModuleInfo>::uninit();
        let result = unsafe {
            ffi::arctracker_get_current_module(self.handle, module.as_mut_ptr())
        };
        if !result.success {
            return Err(c_string_to_rust(&result.error_message));
        }
        let module = unsafe { module.assume_init() };
        let mut samples = Vec::with_capacity(module.num_samples as usize);
        for sample_no in 0..module.num_samples {
            let sample = self.get_sample_info(sample_no).map_err(|e| format!("Failed to get sample info: {}", e.message))?;
            samples.push(sample);
        }
        let pattern_lengths = self.get_pattern_lengths(module.num_patterns)
            .map_err(|e| format!("Failed to get pattern lengths: {}", e.message))?;
        Ok(Module {
            name: c_string_to_rust(&module.name),
            author: c_string_to_rust(&module.author),
            num_channels: module.num_channels,
            tune_length: module.tune_length,
            num_samples: module.num_samples,
            num_patterns: module.num_patterns,
            pattern_lengths,
            samples,
        })
    }

    fn get_pattern_lengths(&mut self, num_patterns: i32) -> Result<Vec<i32>, ArctrackerError> {
        if num_patterns <= 0 {
            return Err(ArctrackerError {
                message: "Invalid number of patterns".to_string(),
            });
        }
        let mut pattern_lengths = vec![0 as c_int; num_patterns as usize];
        let result = unsafe {
            ffi::arctracker_get_pattern_lengths(
                self.handle,
                pattern_lengths.as_mut_ptr(),
                num_patterns,
            )
        };
        if !result.success {
            return Err(ArctrackerError {
                message: c_string_to_rust(&result.error_message),
            });
        }
        Ok(pattern_lengths.into_iter().map(|x| x as i32).collect())
    }

    pub fn load_module_with_info(&mut self, path: &str) -> Result<Module, String> {
        let c_filename = CString::new(path).map_err(|_| "Invalid filename")?;
        let mut module = std::mem::MaybeUninit::<ffi::UiModuleInfo>::uninit();
        let result = unsafe {
            ffi::arctracker_module_load(self.handle, c_filename.as_ptr(), module.as_mut_ptr())
        };
        if !result.success {
            return Err(c_string_to_rust(&result.error_message));
        }
        let module = unsafe { module.assume_init() };
        let mut samples = Vec::with_capacity(module.num_samples as usize);
        for sample_no in 0..module.num_samples {
            let sample = self.get_sample_info(sample_no).map_err(|e| format!("Failed to get sample info: {}", e.message))?;
            samples.push(sample);
        }
        let pattern_lengths = self.get_pattern_lengths(module.num_patterns)
            .map_err(|e| format!("Failed to get pattern lengths: {}", e.message))?;
        Ok(Module {
            name: c_string_to_rust(&module.name),
            author: c_string_to_rust(&module.author),
            num_channels: module.num_channels,
            tune_length: module.tune_length,
            num_samples: module.num_samples,
            num_patterns: module.num_patterns,
            pattern_lengths,
            samples,
        })
    }

    pub fn get_pattern(&mut self, pattern_no: i32, num_rows: i32, num_channels: i32) -> Vec<PatternLine> {
        let len = (num_rows * num_channels) as usize;
        let empty_effect = ffi::UiEffect {
            effect_code: 0,
            effect_data: [0, 0],
        };
        let empty_event = ffi::UiPatternEvent {
            note: 0,
            sample_no: 0,
            effects: [empty_effect; 4],
        };
        let mut ffi_events = vec![empty_event; len];
        unsafe {
            ffi::arctracker_get_pattern(
                self.handle,
                pattern_no,
                ffi_events.as_mut_ptr(),
                num_rows,
                num_channels,
            );
        }
        ffi_events
            .chunks(num_channels as usize)
            .enumerate()
            .map(|(row, line)| PatternLine {
                row: row as i32,
                events: line.iter().copied().map(PatternEvent::from).collect(),
            })
            .collect()
    }

    pub fn create_module(&mut self, num_channels: i32) -> Result<Module, ArctrackerError> {
        let mut module_info = std::mem::MaybeUninit::<ffi::UiModuleInfo>::uninit();
        let result = unsafe {
            ffi::arctracker_module_create(self.handle, num_channels, module_info.as_mut_ptr())
        };
        if !result.success {
            return Err(ArctrackerError {
                message: c_string_to_rust(&result.error_message),
            });
        }
        let module_info = unsafe { module_info.assume_init() };
        let pattern_lengths = self.get_pattern_lengths(module_info.num_patterns).map_err(|e| e)?;
        Ok(Module {
            name: c_string_to_rust(&module_info.name),
            author: c_string_to_rust(&module_info.author),
            num_channels: module_info.num_channels,
            tune_length: module_info.tune_length,
            num_samples: module_info.num_samples,
            num_patterns: module_info.num_patterns,
            pattern_lengths,
            samples: Vec::new(),
        })
    }

    fn get_sample_info(&mut self, sample_no: i32) -> Result<Sample, ArctrackerError> {
        let mut sample_info = std::mem::MaybeUninit::<ffi::UiSampleInfo>::uninit();
        let result = unsafe {
            ffi::arctracker_get_sample_info(self.handle, sample_no, sample_info.as_mut_ptr())
        };
        if !result.success {
            return Err(ArctrackerError {
                message: c_string_to_rust(&result.error_message),
            });
        }
        let sample_info = unsafe { sample_info.assume_init() };
        Ok(Sample {
            name: c_string_to_rust(&sample_info.name),
            default_gain: sample_info.default_gain,
            sample_length: sample_info.sample_length,
            repeats: sample_info.repeats,
            repeat_offset: sample_info.repeat_offset,
            repeat_length: sample_info.repeat_length,
            transpose: sample_info.transpose,
        })
    }

    pub fn start(&mut self) -> Result<(), ArctrackerError> {
        let result = unsafe { ffi::arctracker_player_start(self.handle) };
        if result.success {
            Ok(())
        } else {
            Err(ArctrackerError {
                message: c_string_to_rust(&result.error_message),
            })
        }
    }
    
    pub fn shutdown(&mut self) -> Result<(), ArctrackerError> {
        let result = unsafe { ffi::arctracker_player_shutdown(self.handle) };
        if result.success {
            Ok(())
        } else {
            Err(ArctrackerError {
                message: c_string_to_rust(&result.error_message),
            })
        }
    }

    pub fn get_transport_state(&mut self) -> UiTransportState {
        let mut transport_state = std::mem::MaybeUninit::<ffi::UiTransportState>::uninit();
        unsafe {
            ffi::arctracker_get_transport_state(self.handle, transport_state.as_mut_ptr())
        };
        let transport_state = unsafe { transport_state.assume_init() };
        UiTransportState {
            playing: transport_state.playing,
            looping: transport_state.looping,
            sequence_pos: transport_state.sequence_pos,
            pattern_index: transport_state.pattern_index,
            pattern_no: transport_state.pattern_no,
            pattern_length: transport_state.pattern_length,
        }
    }

    pub fn poll_playback_events(&mut self) -> Vec<PlayerEvent> {
        let mut events = Vec::new();
        while let Some(event) = self.poll_playback_event() {
            events.push(event);
        }
        events
    }

    fn poll_playback_event(&mut self) -> Option<PlayerEvent> {
        let mut event = std::mem::MaybeUninit::<ffi::PlayerEvent>::uninit();
        let got_event = unsafe {
            ffi::arctracker_poll_playback_event(self.handle, event.as_mut_ptr())
        };
        if !got_event {
            return None;
        }
        let event = unsafe { event.assume_init() };
        Some(event.into())
    }

    pub fn poll_export_events(&mut self) -> Vec<PlayerEvent> {
        let mut events = Vec::new();
        while let Some(event) = self.poll_export_event() {
            events.push(event);
        }
        events
    }

    fn poll_export_event(&mut self) -> Option<PlayerEvent> {
        let mut event = std::mem::MaybeUninit::<ffi::PlayerEvent>::uninit();
        let got_event = unsafe {
            ffi::arctracker_poll_export_event(self.handle, event.as_mut_ptr())
        };
        if !got_event {
            return None;
        }
        let event = unsafe { event.assume_init() };
        Some(PlayerEvent {
            event_type: PlayerEventType::PlayerError,
            error_message: c_string_to_rust(&event.error_message),
            midi_note: 0, // All but error events are filtered out.
        })
    }

    pub fn toggle_play(&mut self) {
        let command = ffi::PlayerCommand {
            cmd_type: ffi::PlayerCommandType::TOGGLE_PLAY,
            new_sequence_pos: 0,
            new_pattern_pos: 0,
            channel_no: 0,
            note: 0,
            sample_no: 0,
        };
        unsafe {
            ffi::arctracker_player_cmd(self.handle, &command);
        }
    }

    pub fn toggle_loop(&mut self) {
        let command = ffi::PlayerCommand {
            cmd_type: ffi::PlayerCommandType::TOGGLE_LOOP,
            new_sequence_pos: 0,
            new_pattern_pos: 0,
            channel_no: 0,
            note: 0,
            sample_no: 0,
        };
        unsafe {
            ffi::arctracker_player_cmd(self.handle, &command);
        }
    }

    pub fn seek(&mut self, new_sequence_pos: i32, new_pattern_pos: i32) {
        let command = ffi::PlayerCommand {
            cmd_type: ffi::PlayerCommandType::SEEK,
            new_sequence_pos,
            new_pattern_pos,
            channel_no: 0,
            note: 0,
            sample_no: 0,
        };
        unsafe {
            ffi::arctracker_player_cmd(self.handle, &command);
        }
    }

    pub fn midi_note_on(&mut self, note: i32, sample_no: i32, channel_no: i32)
    {
        let command = ffi::PlayerCommand {
            cmd_type: ffi::PlayerCommandType::MIDI_NOTE_ON,
            new_sequence_pos: 0,
            new_pattern_pos: 0,
            channel_no,
            note,
            sample_no,
        };
        unsafe {
            ffi::arctracker_player_cmd(self.handle, &command);
        }
    }

    pub fn keyboard_note_on(&mut self, note: i32, sample_no: i32, channel_no: i32)
    {
        let command = ffi::PlayerCommand {
            cmd_type: ffi::PlayerCommandType::KEYBOARD_NOTE_ON,
            new_sequence_pos: 0,
            new_pattern_pos: 0,
            channel_no,
            note,
            sample_no,
        };
        unsafe {
            ffi::arctracker_player_cmd(self.handle, &command);
        }
    }

    pub fn export_audio(&mut self, path: &str) -> Result<(), ArctrackerError> {
        let c_filename = CString::new(path).unwrap();
        let result = unsafe {
            ffi::arctracker_export_audio(self.handle, c_filename.as_ptr())
        };
        if result.success {
            Ok(())
        } else {
            Err(ArctrackerError {
                message: c_string_to_rust(&result.error_message),
            })
        }
    }

    pub fn get_export_state(&mut self) -> UiExportState {
        let mut export_state = std::mem::MaybeUninit::<ffi::UiExportState>::uninit();
        unsafe {
            ffi::arctracker_get_export_state(self.handle, export_state.as_mut_ptr())
        };
        let export_state = unsafe { export_state.assume_init() };
        UiExportState {
            completed: export_state.completed,
            percent_complete: export_state.percent_complete,
        }
    }

    pub fn export_cleanup(&mut self) -> Result<(), ArctrackerError> {
        let result = unsafe { ffi::arctracker_export_cleanup(self.handle) };
        if result.success {
            Ok(())
        } else {
            Err(ArctrackerError {
                message: c_string_to_rust(&result.error_message),
            })
        }
    }

    pub fn edit_get_event(&mut self, pattern_no: i32, pattern_index: i32, channel_no: i32) -> Result<PatternEvent, ArctrackerError> {
        let mut pattern_event = std::mem::MaybeUninit::<ffi::UiPatternEvent>::uninit();
        let result = unsafe { ffi::arctracker_edit_get_event(self.handle, pattern_no, pattern_index, channel_no, pattern_event.as_mut_ptr()) };
        if result.success {
            let pattern_event = unsafe { pattern_event.assume_init() };
            Ok(PatternEvent::from(pattern_event))
        } else {
            Err(ArctrackerError {
                message: c_string_to_rust(&result.error_message),
            })
        }
    }

    pub fn edit_get_sequence(&mut self, sequence_len: i32) -> Result<Vec<i32>, ArctrackerError> {
        let mut sequence = vec![0 as c_int; sequence_len as usize];
        let result = unsafe {
            ffi::arctracker_edit_get_sequence(
                self.handle,
                sequence.as_mut_ptr(),
                sequence_len,
            )
        };
        if !result.success {
            return Err(ArctrackerError {
                message: c_string_to_rust(&result.error_message),
            });
        }
        Ok(sequence.into_iter().map(|x| x as i32).collect())
    }

    pub fn edit_set_sequence(&mut self, new_sequence: &[i32]) -> Result<(), ArctrackerError> {
        let result = unsafe {
            ffi::arctracker_edit_set_sequence(
                self.handle,
                new_sequence.as_ptr(),
                new_sequence.len() as c_int,
            )
        };
        if !result.success {
            return Err(ArctrackerError {
                message: c_string_to_rust(&result.error_message),
            });
        }
        Ok(())
    }

    pub fn edit_set_event(&mut self, pattern_no: i32, pattern_index: i32, channel_no: i32, new_event: PatternEvent) -> Result<(), ArctrackerError> {
        let mut event = ffi::UiPatternEvent::try_from(new_event)
            .map_err(|message| ArctrackerError { message })?;
        let result = unsafe { ffi::arctracker_edit_set_event(self.handle, pattern_no, pattern_index, channel_no, &mut event) };
        if result.success {
            Ok(())
        } else {
            Err(ArctrackerError {
                message: c_string_to_rust(&result.error_message),
            })
        }
    }
}

impl Drop for Arctracker {
    fn drop(&mut self) {
        unsafe {
            ffi::arctracker_destroy(self.handle);
        }
    }
}
