#ifndef ARCTRACKER_ENGINE_MIDI_COMMAND_H
#define ARCTRACKER_ENGINE_MIDI_COMMAND_H

#include <stdint.h>

typedef enum { NOTE_ON, NOTE_OFF } midi_cmd_type_t;

typedef struct {
    int track;
    uint8_t instrument_no;
    int note;
    uint8_t velocity;
    bool is_keyboard;
} midi_note_on_command_t;

typedef struct {
    int track;
    uint8_t instrument_no;
} midi_note_off_command_t;

typedef union {
    midi_note_on_command_t note_on;
    midi_note_off_command_t note_off;
} midi_command_data_t;

typedef struct midi_command {
    midi_cmd_type_t cmd_type;
    midi_command_data_t data;
} midi_command_t;

#endif //ARCTRACKER_ENGINE_MIDI_COMMAND_H
