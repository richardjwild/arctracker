use std::sync::{Arc, Mutex};

use crate::arctracker::Arctracker;

pub struct EditorState {
    pub selected_instrument: u8,
    pub selected_channel: i32,
}

pub struct AppState {
    pub tracker: Arc<Mutex<Arctracker>>,
    pub editor: Mutex<EditorState>,
}