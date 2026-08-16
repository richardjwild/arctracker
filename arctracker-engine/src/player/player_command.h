#ifndef ARCTRACKER_PLAYER_COMMAND_H
#define ARCTRACKER_PLAYER_COMMAND_H

#include <stdint.h>

typedef enum cmd_type {
    TOGGLE_PLAY = 0,
    SEEK = 1,
    MIDI_NOTE_ON = 2,
    MIDI_NOTE_OFF = 3, // Reserved for future use.
    KEYBOARD_NOTE_ON = 4,
    KEYBOARD_NOTE_OFF = 5, // Reserved for future use.
    TOGGLE_LOOP = 6,
    SET_MASTER_GAIN = 7,
    TRACK_MUTE_STATE_CHANGED = 8,
} cmd_type_t;

typedef struct {
    uint8_t unused;
} no_data_command_t;

typedef struct {
    int new_sequence_pos;
    int new_pattern_pos;
} seek_command_t;

typedef struct {
    int track;
    uint8_t instrument_no;
    int note;
    uint8_t velocity;
} midi_note_on_command_t;

typedef struct {
    int track;
    uint8_t instrument_no;
    int note;
} keyboard_note_on_command_t;

typedef struct {
    int track;
    uint8_t instrument_no;
} note_off_command_t;

typedef struct {
    float master_gain;
} master_gain_command_t;

typedef struct {
    int track;
} track_mute_command_t;

typedef union {
    no_data_command_t no_data;
    seek_command_t seek;
    midi_note_on_command_t midi_note_on;
    keyboard_note_on_command_t keyboard_note_on;
    note_off_command_t note_off;
    master_gain_command_t master_gain;
    track_mute_command_t track_mute;
} player_command_data_t;

typedef struct player_command {
    cmd_type_t cmd_type;
    player_command_data_t data;
} player_command_t;

#endif //ARCTRACKER_PLAYER_COMMAND_H
