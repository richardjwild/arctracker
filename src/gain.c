#include "gain.h"
#include "mu_law.h"
#include "play_mod.h"

static int master_gain;

const unsigned char max_logarithmic_gain = 255;
const long pan_l[] = {256, 212, 172, 128, 84, 44, 0};
const long pan_r[] = {0, 44, 84, 128, 172, 212, 256};

void set_master_gain(int gain)
{
    master_gain = gain;
}

static inline
unsigned char adjust_logarithmic_gain(const unsigned char mlaw, const unsigned char gain)
{
    const unsigned char adjustment = max_logarithmic_gain - gain;
    if (mlaw > adjustment)
        return mlaw - adjustment;
    else
        return 0;
}

stereo_frame_t apply_gain(unsigned char mu_law, voice_t *voice)
{
    stereo_frame_t stereo_frame;
    mu_law = adjust_logarithmic_gain(mu_law, voice->gain);
    const long pcm = linear_pcm[mu_law];
    stereo_frame.l = master_gain * pcm * pan_l[voice->panning] >> 16;
    stereo_frame.r = master_gain * pcm * pan_r[voice->panning] >> 16;
    return stereo_frame;
}