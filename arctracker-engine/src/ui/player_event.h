#ifndef ARCTRACKER_ENGINE_UI_EVENT_H
#define ARCTRACKER_ENGINE_UI_EVENT_H

#define ERROR_MESSAGE_MAX_LENGTH 256

typedef enum player_event_type {
    PLAYER_ERROR = 0,
    USER_MIDI_NOTE_ON = 1,
} player_event_type_t;

typedef struct {
    char error_message[ERROR_MESSAGE_MAX_LENGTH];
} player_error_event_data_t;

typedef struct {
    int midi_note;
} player_midi_note_event_t;

typedef union {
    player_error_event_data_t player_error;
    player_midi_note_event_t midi_note;
} player_event_data_t;

typedef struct player_event {
    player_event_type_t type;
    player_event_data_t data;
} player_event_t;

#endif //ARCTRACKER_ENGINE_UI_EVENT_H
