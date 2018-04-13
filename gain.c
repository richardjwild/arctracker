#include "gain.h"
#include "log_lin_tab.h"
#include "play_mod.h"

static int master_gain;

const long left_gain[] = {256, 212, 172, 128, 84, 44, 0};
const long right_gain[] = {0, 44, 84, 128, 172, 212, 256};

void set_master_gain(int gain)
{
    master_gain = gain;
}

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
    stereo_frame.l = master_gain * (linear_pcm * left_gain[voice->panning]) >> 16;
    stereo_frame.r = master_gain * (linear_pcm * right_gain[voice->panning]) >> 16;
    return stereo_frame;
}