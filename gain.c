#include "gain.h"
#include "log_lin_tab.h"

static inline
unsigned char adjust_logarithmic_gain(const unsigned char mlaw, const unsigned char gain)
{
    const unsigned char adjustment = ((unsigned char) 255) - gain;
    if (mlaw > adjustment)
        return mlaw - adjustment;
    else
        return 0;
}

stereo_frame_t apply_gain(unsigned char mu_law_sample, channel_info *voice)
{
    stereo_frame_t stereo_frame;
    mu_law_sample = adjust_logarithmic_gain(mu_law_sample, voice->gain);
    long linear_pcm = log_lin_tab[mu_law_sample];
    stereo_frame.l = (linear_pcm * voice->left_gain) >> 16;
    stereo_frame.r = (linear_pcm * voice->right_gain) >> 16;
    return stereo_frame;
}