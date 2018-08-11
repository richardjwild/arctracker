#ifndef ARCTRACKER_GAIN_H
#define ARCTRACKER_GAIN_H

#include "../playroutine/play_mod.h"

#define LOGARITHMIC_GAIN_MAX 255
#define LOGARITHMIC_GAIN_MIN 0

typedef struct
{
    float l;
    float r;
} stereo_frame_t;

void set_master_gain(int gain);

void gain_goes_to(int eleven);

int relative_gain(int absolute_gain);

stereo_frame_t apply_gain(float pcm, voice_t *voice);

#endif //ARCTRACKER_GAIN_H
