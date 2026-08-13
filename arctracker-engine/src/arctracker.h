#ifndef ARCTRACKER_ARCTRACKER_H
#define ARCTRACKER_ARCTRACKER_H

#include <pthread.h>
#include "player/player.h"
#include "ui/player_event_queue.h"

typedef struct {
    bool initialised;
    player_t *player;
    player_event_queue_t *event_queue;
    bool thread_active;
    pthread_t audio_thread;
} audio_subsystem_t;

typedef enum {
    IDLE, RUNNING, COMPLETE
} export_state_t;

typedef struct arctracker_handle {
    module_t *module;
    audio_api_t playback_audio_api;
    audio_subsystem_t playback;
    audio_subsystem_t export;
    export_state_t export_state;
} arctracker_t;

#endif //ARCTRACKER_ARCTRACKER_H
