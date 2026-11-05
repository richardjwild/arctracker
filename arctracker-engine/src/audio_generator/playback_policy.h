#ifndef ARCTRACKER_ENGINE_PLAYBACK_POLICY_H
#define ARCTRACKER_ENGINE_PLAYBACK_POLICY_H

#include <stdbool.h>
#include "audio_out/interpolation_type.h"
#include "audio_out/gainer.h"

typedef struct {
    bool initialised;
    interpolation_type_t interpolation_type;
    bool relative_pitch_bend;
    volume_mapping_type_t volume_mapping;
} playback_policy_t;

#endif //ARCTRACKER_ENGINE_PLAYBACK_POLICY_H
