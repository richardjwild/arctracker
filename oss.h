#ifndef ARCTRACKER_OSS_H
#define ARCTRACKER_OSS_H

#include "audio_api.h"

#define DEVICE_NAME "/dev/dsp"

audio_api_t initialise_oss(long sample_rate, int audio_buffer_frames);

#endif // ARCTRACKER_OSS_H