#ifndef ARCTRACKER_VOICE_H
#define ARCTRACKER_VOICE_H

#include <stdbool.h>
#include <stdint.h>
#include "player/lfo.h"
#include "audio/interpolation_type.h"
#include "audio/volume_mapping_type.h"

typedef struct {
    float phase_increment_per_period;
    double fine_tuning;
    bool sample_repeats;
    int sample_end;
    int repeat_length;
    interpolation_type_t interpolation_type;
    const float *sample_pointer;
} player_sample_t;

typedef struct {
    uint32_t offset;
    uint32_t length;
} player_sample_slice_t;

typedef struct {
    bool assigned;
    int transpose;
    float *gain_curve;
    player_sample_slice_t sample_slices[256];
    player_sample_t sample;
} player_instrument_t;

typedef struct {
    bool enabled;
    bool retrigger;
    pt_waveform_t waveform;
    uint8_t phase;
} lfo_effect_t;

typedef struct {
    bool enabled;
    int16_t chord[3];
    int counter;
} pt_arpeggiator_state_t;

typedef struct {
    player_sample_t *sample;
    interpolation_type_t interpolation_type;
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
    float *gain_curve;
} sampler_state_t;

typedef struct {
    sampler_state_t sampler_state;
    bool channel_playing;
    bool muted;
    uint8_t panning;
} voice_t;

#endif //ARCTRACKER_VOICE_H
