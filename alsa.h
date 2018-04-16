#ifndef ARCTRACKER_ALSA_H
#define ARCTRACKER_ALSA_H

#include "audio_api.h"

#define PCM_DEVICE "plughw:0,0"

audio_api_t initialise_alsa(long sample_rate, int audio_buffer_frames);

#endif //ARCTRACKER_ALSA_H