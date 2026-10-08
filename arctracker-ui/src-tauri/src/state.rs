use std::path::PathBuf;
use std::sync::{Arc, Mutex};

use crate::arctracker::{Arctracker, ArctrackerMidi};

pub struct AppState {
    pub tracker: Arc<Mutex<Arctracker>>,
    pub midi: Mutex<Option<ArctrackerMidi>>,
}

#[derive(Default)]
pub struct PendingOpenRequest(Mutex<Option<PathBuf>>);

impl PendingOpenRequest {
    pub fn set(&self, path: PathBuf) {
        *self.0.lock().unwrap() = Some(path);
    }
    pub fn take(&self) -> Option<PathBuf> {
        self.0.lock().unwrap().take()
    }
}

