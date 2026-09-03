#ifndef ARCTRACKER_VOICE_H
#define ARCTRACKER_VOICE_H

#include <stdbool.h>
#include <stdint.h>
#include "player/lfo.h"

typedef struct {
    float phase_increment_per_period;
    double fine_tuning;
    bool sample_repeats;
    int sample_end;
    int repeat_length;
    const float *sample_pointer;
} player_sample_t;

typedef struct {
    bool enabled;
    bool retrigger;
    pt_waveform_t waveform;
    uint8_t phase;
} lfo_effect_t;

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
    float phase_accumulator;
    player_sample_t *sample;
    int period;
    int period_modulation;
    bool glissando_on;
    int tone_portamento_target_period;
    int instrument_no;
    uint8_t volume;
    int volume_modulation;
    bool channel_playing;
    bool muted;
    uint8_t panning;
    bool arpeggiator_on;
    int arpeggio_counter;
    int current_note;
    lfo_effect_t vibrato;
    lfo_effect_t tremolo;
    pt_loop_state_t loop_state;
    effect_memory_t effect_memory;
} voice_t;

#endif //ARCTRACKER_VOICE_H
