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

void audio_channel_silence(audio_channel_t *);

void audio_channel_set_stereo(audio_channel_t *, uint8_t stereo);

#endif //ARCTRACKER_AUDIO_CHANNEL_H
