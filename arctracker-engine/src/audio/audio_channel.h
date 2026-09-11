#ifndef ARCTRACKER_AUDIO_CHANNEL_H
#define ARCTRACKER_AUDIO_CHANNEL_H

#include <stdbool.h>
#include <stdint.h>
#include "audio/audio_generator.h"

typedef struct {
    audio_generator_t audio_generator;
    bool playing;
    bool muted;
    uint8_t panning;
    float gain;
} audio_channel_t;

#endif //ARCTRACKER_AUDIO_CHANNEL_H
