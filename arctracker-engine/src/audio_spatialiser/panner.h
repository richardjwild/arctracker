#ifndef ARCTRACKER_ENGINE_PANNER_H
#define ARCTRACKER_ENGINE_PANNER_H

#include "audio_spatialiser/audio_spatialiser.h"

struct panner_state {
    uint8_t panning;
    float left_gain;
    float right_gain;
};

audio_spatialiser_t init_panner(panner_state_t *panner_state, uint8_t initial_amount);

#endif //ARCTRACKER_ENGINE_PANNER_H
