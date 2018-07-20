#ifndef ARCTRACKER_GAIN_H
#define ARCTRACKER_GAIN_H

#include "play_mod.h"

#define LOGARITHMIC_GAIN_MAX 255
#define LOGARITHMIC_GAIN_MIN 0

typedef struct
{
    long l;
    long r;
} stereo_frame_t;

void set_master_gain(int gain);

stereo_frame_t apply_gain(unsigned char mu_law, voice_t *voice);

#endif //ARCTRACKER_GAIN_H
