#ifndef ARCTRACKER_GAIN_H
#define ARCTRACKER_GAIN_H

#include "play_mod.h"

typedef struct
{
    long l;
    long r;
} stereo_frame_t;

void set_master_gain(int gain);

stereo_frame_t apply_gain(unsigned char mu_law, channel_info *voice);

#endif //ARCTRACKER_GAIN_H
