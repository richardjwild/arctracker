#ifndef ARCTRACKER_GAIN_H
#define ARCTRACKER_GAIN_H

#include <playroutine/play_mod.h>

static const int INTERNAL_GAIN_MIN = 0;
static const int INTERNAL_GAIN_MAX = 255;

typedef struct
{
    double l;
    double r;
} stereo_frame_t;

void set_master_gain(int gain);

void set_module_gain_characteristics(double *curve, int gain_maximum);

int get_internal_gain(int module_gain);

stereo_frame_t apply_gain(double pcm, voice_t *voice);

#endif //ARCTRACKER_GAIN_H
