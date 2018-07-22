#ifndef ARCTRACKER_OSS_H
#define ARCTRACKER_OSS_H

#include "../audio/audio_api.h"

#define DEVICE_NAME "/dev/dsp"

audio_api_t initialise_oss();

#endif // ARCTRACKER_OSS_H