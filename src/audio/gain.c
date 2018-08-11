#include "gain.h"
#include "../pcm/mu_law.h"

const int MAXIMUM_GAIN = 255;

const float PAN_L[] = {1.0, 0.828, 0.672, 0.5, 0.328, 0.172, 0.0};
const float PAN_R[] = {0.0, 0.172, 0.328, 0.5, 0.672, 0.828, 1.0};

static int module_maximum_gain;
static float master_gain;

void set_master_gain(int gain)
{
    master_gain = (float) gain / 256;
}

void gain_goes_to(int eleven)
{
    module_maximum_gain = eleven;
}

int relative_gain(int absolute_gain)
{
    return MAXIMUM_GAIN * absolute_gain / module_maximum_gain;
}

stereo_frame_t apply_gain(float pcm, voice_t *voice)
{
    float voice_gain = convert_to_linear_gain(voice->gain);
    stereo_frame_t stereo_frame;
    stereo_frame.l = master_gain * voice_gain * pcm * PAN_L[voice->panning];
    stereo_frame.r = master_gain * voice_gain * pcm * PAN_R[voice->panning];
    return stereo_frame;
}