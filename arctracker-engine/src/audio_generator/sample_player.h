#ifndef ARCTRACKER_RESAMPLE_H
#define ARCTRACKER_RESAMPLE_H

#include "audio_out/audio_channel.h"
#include "audio_out/interpolation_type.h"

typedef struct {
    float phase_increment_per_period;
    double fine_tuning;
    bool sample_repeats;
    int sample_end;
    int repeat_length;
    interpolation_type_t interpolation_type;
    const float *sample_data;
} player_sample_t;

typedef struct {
    uint32_t offset;
    uint32_t length;
} player_sample_slice_t;

typedef struct {
    bool enabled;
    uint8_t rate;
    uint8_t depth;
    bool retrigger;
    pt_waveform_t waveform;
    uint8_t phase;
} lfo_effect_t;

typedef struct {
    bool enabled;
    int bottom_note;
    int interval_1;
    int interval_2;
    int16_t chord[3];
    int counter;
    int speed;
} pt_arpeggiator_state_t;

struct sampler_state {
    const player_sample_t *sample;
    int sample_end;
    interpolation_type_t interpolation_type;
    float phase_accumulator;
    int period;
    int vibrato_period_modulation;
    int arpeggio_period_modulation;
    bool glissando_on;
    int pitch_slide_rate;
    bool pitch_slide_fine;
    bool tone_portamento_on;
    int tone_portamento_target_period;
    int tone_portamento_slide_rate;
    uint8_t volume;
    int volume_slide_rate;
    bool volume_slide_fine;
    int silence_delay;
    int retrigger_delay;
    int volume_modulation;
    pt_arpeggiator_state_t arpeggio;
    lfo_effect_t vibrato;
    lfo_effect_t tremolo;
    const float *gain_curve;
};

audio_generator_t init_sampler(
    int note,
    const player_sample_t *sample,
    player_sample_slice_t slice,
    uint8_t volume,
    const float *gain_curve,
    sampler_state_t *sampler_state
);

#endif // ARCTRACKER_RESAMPLE_H
