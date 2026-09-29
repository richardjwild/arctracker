#ifndef ARCTRACKER_AUDIO_CHANNEL_H
#define ARCTRACKER_AUDIO_CHANNEL_H

#include <stdbool.h>
#include <stdint.h>
#include "audio_generator/audio_generator.h"
#include "audio_spatialiser/audio_spatialiser.h"

typedef struct {
    audio_generator_t audio_generator;
    audio_spatialiser_t audio_spatialiser;
    bool muted;
    float gain;
} audio_channel_t;

void audio_channel_silence(audio_channel_t *);

void audio_channel_set_stereo(audio_channel_t *, uint8_t stereo);

uint8_t audio_channel_get_stereo(audio_channel_t *);

#endif //ARCTRACKER_AUDIO_CHANNEL_H
