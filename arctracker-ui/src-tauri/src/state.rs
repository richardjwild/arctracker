use std::sync::{Arc, Mutex};

use crate::arctracker::{Arctracker, ArctrackerMidi};

pub struct AppState {
    pub tracker: Arc<Mutex<Arctracker>>,
    pub midi: Mutex<Option<ArctrackerMidi>>,
}
