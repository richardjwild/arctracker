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

typedef struct player_command {
    cmd_type_t cmd_type;
    int new_sequence_pos;
    int new_pattern_pos;
    int track;
    int note;
    uint8_t instrument_no;
    float master_gain;
} player_command_t;

#endif //ARCTRACKER_PLAYER_COMMAND_H
