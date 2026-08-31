#ifndef ARCTRACKER_RESAMPLE_H
#define ARCTRACKER_RESAMPLE_H

#include "voice.h"
#include "audio_api/api.h"

float *allocate_resample_buffer(int no_of_frames);

void resample(voice_t *voice, float *resample_buffer, int frames_to_write, interpolation_type_t interpolation_type);

#endif // ARCTRACKER_RESAMPLE_H
