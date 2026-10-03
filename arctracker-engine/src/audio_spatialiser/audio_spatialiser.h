#ifndef ARCTRACKER_ENGINE_AUDIO_SPATIALISER_H
#define ARCTRACKER_ENGINE_AUDIO_SPATIALISER_H

#include <stdint.h>
#include "audio_api/audio_api.h"

typedef struct panner_state panner_state_t;

typedef union {
    panner_state_t *panner;
} audio_spatialiser_state_t;

typedef struct {
    audio_spatialiser_state_t state;
    void (*spatialise)(audio_spatialiser_state_t *state, const float *mono, stereo_frame_t *stereo, int num_frames);
    void (*set_amount)(audio_spatialiser_state_t *state, uint8_t amount);
    uint8_t (*get_amount)(audio_spatialiser_state_t *state);
    void (*slide_on)(audio_spatialiser_state_t *state, int slide_rate, int slide_amount, bool dtt_mode);
    void (*slide_off)(audio_spatialiser_state_t *state);
    void (*tick)(audio_spatialiser_state_t *state, int tick, int ticks_per_event);
} audio_spatialiser_t;

#endif //ARCTRACKER_ENGINE_AUDIO_SPATIALISER_H
