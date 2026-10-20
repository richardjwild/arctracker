#ifndef ARCTRACKER_ARCTRACKER_H
#define ARCTRACKER_ARCTRACKER_H

#include <pthread.h>
#include "portaudio.h"
#include "audio_export/audio_export.h"
#include "player/player.h"
#include "portaudio/portaudio_api.h"
#include "ui/player_event_queue.h"

typedef enum {
    IDLE, RENDERING, WRITING, COMPLETE, FAILED
} export_state_t;

typedef struct {
    player_t *player;
    player_event_queue_t *event_queue;
    bool stream_started;
    PaStream *pa_stream;
} playback_audio_subsystem_t;

typedef struct {
    player_t *player;
    player_event_queue_t *event_queue;
    bool thread_active;
    pthread_t audio_thread;
    wav_export_t wav_export;
} export_audio_subsystem_t;

typedef struct arctracker_handle {
    bool portaudio_initialised;
    portaudio_config_t audio_config;
    module_t *module;
    playback_audio_subsystem_t playback;
    export_audio_subsystem_t export;
    export_state_t export_state;
    midi_command_queue_t *midi_queue;
} arctracker_t;

#endif //ARCTRACKER_ARCTRACKER_H
