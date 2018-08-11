#include "gain.h"
#include "../pcm/mu_law.h"

static float master_gain;
static float maximum_gain;

const float pan_l[] = {1.0, 0.828, 0.672, 0.5, 0.328, 0.172, 0.0};
const float pan_r[] = {0.0, 0.172, 0.328, 0.5, 0.672, 0.828, 1.0};

void set_master_gain(int gain)
{
    master_gain = (float) gain / 256;
}

void gain_goes_to(int maximum_gain_in)
{
    maximum_gain = (float) maximum_gain_in;
}

stereo_frame_t apply_gain(float pcm, voice_t *voice)
{
    float voice_gain = convert_to_linear_gain(voice->gain);
    stereo_frame_t stereo_frame;
    stereo_frame.l = master_gain * voice_gain * pcm * pan_l[voice->panning];
    stereo_frame.r = master_gain * voice_gain * pcm * pan_r[voice->panning];
    return stereo_frame;
}