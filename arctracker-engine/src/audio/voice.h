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
    float phase_accumulator;
    player_sample_t *sample;
    int period;
    int period_modulation;
    int tone_portamento_target_period;
    int instrument_no;
    uint8_t volume;
    bool channel_playing;
    bool muted;
    uint8_t panning;
    bool arpeggiator_on;
    int arpeggio_counter;
    int current_note;
    bool vibrato_on;
    bool vibrato_retrigger_on;
    pt_waveform_t vibrato_type;
    uint8_t vibrato_phase;
    uint8_t effect_memory[4];
} voice_t;

#endif //ARCTRACKER_VOICE_H
