#ifndef ARCTRACKER_ENGINE_PANNER_H
#define ARCTRACKER_ENGINE_PANNER_H

#include "audio_spatialiser/audio_spatialiser.h"
#include <stdint.h>

typedef struct {
    int amount;
    int rate;
} stereo_slide_t;

struct panner_state {
    uint8_t panning;
    stereo_slide_t slide;
    float left_gain;
    float right_gain;
};

audio_spatialiser_t init_panner(panner_state_t *panner_state, uint8_t initial_amount);

#endif //ARCTRACKER_ENGINE_PANNER_H
