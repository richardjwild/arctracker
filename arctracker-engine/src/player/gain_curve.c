#include "gain_curve.h"
#include <math.h>
#include "module.h"

void calculate_gain_curve(float *gain_curve, const volume_mapping_type_t volume_mapping)
{
    const float volume_max = (float) INTERNAL_GAIN_MAX;
    if (volume_mapping == VOLUME_AMIGA)
    {
        for (int vol = 0; vol <= INTERNAL_GAIN_MAX; vol++)
            gain_curve[vol] = (float) vol / volume_max;
    }
    else
    {
        gain_curve[0] = 0.0f;
        for (int vol = 1; vol <= INTERNAL_GAIN_MAX; vol++)
        {
            const float exponent = (float) vol / volume_max;
            gain_curve[vol] = (powf(volume_max + 1.0f, exponent) - 1.0f) / volume_max;
        }
    }
}
