#include "gain.h"
#include <pcm/mu_law.h>

// 1024 determined by trial and error to give acceptable volume
// with reasonably low likelihood of clipping
static const double MASTER_GAIN_FACTOR = 1024.0;

static double master_gain;
static double module_gain_factor;

const double PAN_L[] = {1.0, 0.828, 0.672, 0.5, 0.328, 0.172, 0.0};
const double PAN_R[] = {0.0, 0.172, 0.328, 0.5, 0.672, 0.828, 1.0};

void set_master_gain(int gain)
{
    master_gain = gain / MASTER_GAIN_FACTOR;
}

void module_gain_goes_to(int module_gain_maximum)
{
    module_gain_factor = (double) INTERNAL_GAIN_MAX / module_gain_maximum;
}

int get_internal_gain(int module_gain)
{
    return (int) (module_gain * module_gain_factor);
}

stereo_frame_t apply_gain(double pcm, voice_t *voice)
{
    double voice_gain = convert_to_linear_gain(voice->gain);
    stereo_frame_t stereo_frame;
    double adjusted_pcm = master_gain * voice_gain * pcm;
    if (voice->panning >= 0 && voice->panning <= 6)
    {
        stereo_frame.l = adjusted_pcm * PAN_L[voice->panning];
        stereo_frame.r = adjusted_pcm * PAN_R[voice->panning];
    }
    else
    {
        stereo_frame.l = adjusted_pcm * 0.5f;
        stereo_frame.r = adjusted_pcm * 0.5f;
    }
    return stereo_frame;
}