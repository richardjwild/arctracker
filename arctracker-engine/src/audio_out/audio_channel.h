#ifndef ARCTRACKER_AUDIO_CHANNEL_H
#define ARCTRACKER_AUDIO_CHANNEL_H

#include <stdbool.h>
#include <stdint.h>
#include "audio_generator/audio_generator.h"

typedef struct {
    audio_generator_t audio_generator;
    bool muted;
    uint8_t panning;
    float gain;
} audio_channel_t;

void silence_channel(audio_channel_t *channel);

#endif //ARCTRACKER_AUDIO_CHANNEL_H
