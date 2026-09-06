#ifndef ARCTRACKER_RESAMPLE_H
#define ARCTRACKER_RESAMPLE_H

#include "voice.h"
#include "audio_api/api.h"

float *allocate_resample_buffer(int no_of_frames);

bool resample(sampler_state_t *sampler, float *channel_buffer, int frames_to_write);

#endif // ARCTRACKER_RESAMPLE_H
