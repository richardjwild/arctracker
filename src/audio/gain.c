#include "gain.h"
#include <pcm/mu_law.h>

// 1024 determined by trial and error to give acceptable volume
// with reasonably low likelihood of clipping
const float MASTER_GAIN_FACTOR = 1024.0;
static float master_gain;

const int INTERNAL_GAIN_MAXIMUM = 255;
static int module_gain_maximum;

const float PAN_L[] = {1.0, 0.828, 0.672, 0.5, 0.328, 0.172, 0.0};
const float PAN_R[] = {0.0, 0.172, 0.328, 0.5, 0.672, 0.828, 1.0};

void set_master_gain(int gain)
{
    master_gain = gain / MASTER_GAIN_FACTOR;
}

// Tracker module volume max: 255
// Desktop Tracker module volume max: 127
void gain_goes_to(int maximum_value)
{
    module_gain_maximum = maximum_value;
}

// Convert module volume values to common internal gain value
int relative_gain(int value)
{
    return value * INTERNAL_GAIN_MAXIMUM / module_gain_maximum;
}

stereo_frame_t apply_gain(float pcm, voice_t *voice)
{
    float voice_gain = convert_to_linear_gain(voice->gain);
    stereo_frame_t stereo_frame;
    stereo_frame.l = master_gain * voice_gain * pcm * PAN_L[voice->panning];
    stereo_frame.r = master_gain * voice_gain * pcm * PAN_R[voice->panning];
    return stereo_frame;
}