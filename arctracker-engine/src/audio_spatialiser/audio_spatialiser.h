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
    void (*spatialise)(audio_spatialiser_state_t *state, float mono, stereo_frame_t *stereo);
    void (*set_amount)(audio_spatialiser_state_t *state, uint8_t amount);
    uint8_t (*get_amount)(audio_spatialiser_state_t *state);
} audio_spatialiser_t;

#endif //ARCTRACKER_ENGINE_AUDIO_SPATIALISER_H
