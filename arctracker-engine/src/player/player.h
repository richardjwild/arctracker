#ifndef ARCTRACKER_PLAY_MOD_H
#define ARCTRACKER_PLAY_MOD_H

#include "module.h"
#include "player_cmd_queue.h"
#include "sequencer.h"
#include "tick_scheduler.h"
#include "audio_api/api.h"
#include "audio/write_audio.h"
#include "ui/player_event_queue.h"

typedef struct {
    const event_t *events;
    int num_tracks;
    int sequence_pos;
    int pattern_pos;
    bool row_advanced;
} frame_t;

typedef struct {
    void (*on_player_error)(const char *);
} ui_event_consumer_t;

typedef struct {
    bool looping;
    int start;
    int counter;
} pt_loop_state_t;

typedef struct {
    uint8_t tone_portamento_speed;
    uint8_t vibrato_speed;
    uint8_t vibrato_depth;
    uint8_t tremolo_speed;
    uint8_t tremolo_depth;
} effect_memory_t;

typedef struct {
    int instrument_no;
    int current_note;
    pt_loop_state_t loop_state;
    effect_memory_t effect_memory;
} player_track_t;

typedef struct {
    bool scheduled;
    uint8_t delay;
    int note;
    const instrument_t *instrument;
    player_sample_t *sample;
    uint8_t slice;
    voice_t *voice;
} scheduled_note_t;

typedef struct player {
    bool running;
    bool playing;
    bool bouncing;
    float master_gain;
    int current_bpm;
    voice_t *voices;
    player_track_t *tracks;
    module_t *module;
    scheduled_note_t *scheduled_notes;
    sequence_t sequence;
    player_instrument_t instruments[256];
    tick_scheduler_t tick_scheduler;
    audio_out_t audio_out;
    ui_event_consumer_t ui_event_consumer;
    frame_t current_frame;
    player_command_queue_t *command_queue;
    player_event_queue_t *player_event_queue;
    const char *error_message;
} player_t;

typedef struct {
    sequence_t sequence;
    tick_scheduler_t tick_scheduler;
} player_restore_state_t;

player_t *player_create(module_t *module, audio_api_t audio_api, player_event_queue_t *player_event_queue);

void player_update_samples(player_t *);

void player_set_sample_finetune(player_t *player, int instrument_no, int8_t finetune);

bool player_run(player_t *);

bool player_queue_command(const player_t *, player_command_t);

void player_shutdown(player_t *);

void player_sequence_changed(player_t *, const module_t *);

player_restore_state_t player_get_restore_state(const player_t *);

void player_restore_state(player_t *player, player_restore_state_t state);

void player_destroy(player_t *);

void player_get_and_reset_peaks(player_t *player, float *peak_l, float *peak_r);

void player_set_bpm(player_t *player, uint8_t beats_per_minute);

#endif //ARCTRACKER_PLAY_MOD_H
