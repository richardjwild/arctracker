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
    bool enabled;
    uint16_t chord[3];
    int counter;
} pt_arpeggiator_state_t;

typedef struct {
    player_sample_t *sample;
    float phase_accumulator;
    int period;
    int period_modulation;
    bool glissando_on;
    int tone_portamento_target_period;
    uint8_t volume;
    int volume_modulation;
    pt_arpeggiator_state_t arpeggio;
    lfo_effect_t vibrato;
    lfo_effect_t tremolo;
} sampler_state_t;

typedef struct {
    float phase_accumulator; /* sampler_state_t */
    player_sample_t *sample; /* sampler_state_t */
    int period; /* sampler_state_t */
    int period_modulation; /* sampler_state_t */
    bool glissando_on; /* sampler_state_t */
    int tone_portamento_target_period; /* sampler_state_t */
    uint8_t volume; /* sampler_state_t */
    int volume_modulation; /* sampler_state_t */
    bool channel_playing;
    bool muted;
    uint8_t panning;
    bool arpeggiator_on; /* arpeggiator_state_t */
    int arpeggio_counter; /* arpeggiator_state_t */
    lfo_effect_t vibrato; /* sampler_state_t */
    lfo_effect_t tremolo; /* sampler_state_t */
} voice_t;

#endif //ARCTRACKER_VOICE_H
