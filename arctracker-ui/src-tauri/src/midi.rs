use std::sync::{Arc};
use midir::{Ignore, MidiInput};
use arctracker_ui_lib::AppState;

const MIDI_CHAN_1_NOTE_ON: u8 = 144;

pub fn start_midi_thread(
    state: Arc<AppState>
) {
    // TODO: 1. Don't panic if MIDI unavailable.
    // TODO: 2. Allow MIDI devices to be configured by the user.
    let mut midi_in = MidiInput::new("arctracker-midi").expect("failed to create midi input");
    midi_in.ignore(Ignore::None);
    let ports = midi_in.ports();
    if ports.is_empty() {
        println!("No MIDI ports found");
        return;
    }
    let port = &ports[0];
    let _connection = midi_in
        .connect(port, "arctracker-input", move |_timestamp, message, _| {
            handle_midi_message(message, &state);
        }, /* data */ ())
        .expect("failed to connect midi");
    loop {
        std::thread::sleep(
            std::time::Duration::from_secs(1)
        );
    }
}

fn handle_midi_message(message: &[u8], state: &AppState) {
    if message.len() < 3 {
        return;
    }
    let status = message[0];
    let midi_note = message[1];
    let velocity = message[2];
    if status == MIDI_CHAN_1_NOTE_ON {
        let mut tracker = state.tracker.lock().unwrap();
        let editor = state.editor.lock().unwrap();
        if velocity > 0 {
            let tracker_note = (midi_note as i32) - 47;
            tracker.midi_note_on(
                tracker_note,
                editor.selected_instrument,
                editor.selected_channel,
            );
        // TODO: Implement note-off only when sample repeats.
        // } else {
        //     tracker.midi_note_off(editor.selected_channel);
        }
    }
}
