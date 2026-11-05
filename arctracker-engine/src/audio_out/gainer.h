#ifndef ARCTRACKER_ENGINE_GAINER_H
#define ARCTRACKER_ENGINE_GAINER_H

#include <stdbool.h>
#include "volume_mapping_type.h"
#include "player/lfo.h"

#define INTERNAL_VOLUME_MAX 255

typedef struct {
    bool initialised;
    float exponential[INTERNAL_VOLUME_MAX + 1];
    float linear[INTERNAL_VOLUME_MAX + 1];
} gain_curves_t;

typedef struct {
    int volume;
    int modulation;
    int slide_rate;
    bool slide_fine;
    lfo_effect_t tremolo;
    volume_mapping_type_t volume_mapping;
    const gain_curves_t *gain_curves;
} gainer_state_t;

void gainer_init_curves(gain_curves_t *gain_curves);

gainer_state_t gainer_init(const gain_curves_t *gain_curves, volume_mapping_type_t volume_mapping);

void gainer_apply(gainer_state_t *state, float *channel_buffer, int frames_requested);

void gainer_tick(gainer_state_t *state, int ticks, int ticks_per_event);

void gainer_set_volume(gainer_state_t *state, int volume);

void gainer_volume_slide_on(gainer_state_t *state, int slide_rate, bool fine);

void gainer_volume_slide_off(gainer_state_t *state);

void gainer_tremolo_on(gainer_state_t *state, int rate, int depth, pt_waveform_t waveform, bool retrigger);

void gainer_tremolo_off(gainer_state_t *state);

void gainer_set_volume_mapping(gainer_state_t *state, volume_mapping_type_t volume_mapping_type);

#endif //ARCTRACKER_ENGINE_GAINER_H
