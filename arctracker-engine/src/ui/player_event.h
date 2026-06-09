#ifndef ARCTRACKER_ENGINE_UI_EVENT_H
#define ARCTRACKER_ENGINE_UI_EVENT_H

#define ERROR_MESSAGE_MAX_LENGTH 256

typedef enum player_event_type {
    PLAYER_ERROR = 0,
    USER_MIDI_NOTE_ON = 1,
    AUDIO_OVERFLOWED = 2,
} player_event_type_t;

typedef struct player_event {
    player_event_type_t type;
    char error_message[ERROR_MESSAGE_MAX_LENGTH];
    int midi_note;
} player_event_t;

#endif //ARCTRACKER_ENGINE_UI_EVENT_H
