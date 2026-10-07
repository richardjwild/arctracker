#ifndef ARCTRACKER_ENGINE_PLAYBACK_POLICY_H
#define ARCTRACKER_ENGINE_PLAYBACK_POLICY_H

#include "audio_out/interpolation_type.h"
#include "player/module.h"

typedef struct {
    interpolation_type_t interpolation_type;
    bool relative_pitch_bend;
    float gain_curve[INTERNAL_GAIN_MAX + 1];
} playback_policy_t;

#endif //ARCTRACKER_ENGINE_PLAYBACK_POLICY_H
