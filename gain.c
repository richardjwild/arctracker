#include "gain.h"
#include "mu_law.h"
#include "play_mod.h"

static int master_gain;

const long pan_gain_l[] = {256, 212, 172, 128, 84, 44, 0};
const long pan_gain_r[] = {0, 44, 84, 128, 172, 212, 256};

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

stereo_frame_t apply_gain(unsigned char mu_law, channel_info *voice)
{
    stereo_frame_t stereo_frame;
    mu_law = adjust_logarithmic_gain(mu_law, voice->gain);
    long pcm = linear_pcm[mu_law];
    stereo_frame.l = master_gain * pcm * pan_gain_l[voice->panning] >> 16;
    stereo_frame.r = master_gain * pcm * pan_gain_r[voice->panning] >> 16;
    return stereo_frame;
}