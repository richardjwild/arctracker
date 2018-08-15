#ifndef ARCTRACKER_GAIN_H
#define ARCTRACKER_GAIN_H

#include <playroutine/play_mod.h>

static const int INTERNAL_GAIN_MIN = 0;
static const int INTERNAL_GAIN_MAX = 255;

typedef struct
{
    float l;
    float r;
} stereo_frame_t;

void set_master_gain(int gain);

void module_gain_goes_to(int module_gain_maximum);

int get_internal_gain(int module_gain);

stereo_frame_t apply_gain(float pcm, voice_t *voice);

#endif //ARCTRACKER_GAIN_H
