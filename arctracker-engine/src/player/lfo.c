#include "lfo.h"
#include <math.h>

static float pt_sine_waveform[64];
static float pt_ramp_waveform[64];
static float pt_square_waveform[64];
static const float PI = 3.14159265358979323846f;

void lfo_init_waveforms(void)
{
    for (int phase = 0; phase < 64; phase++)
    {
        pt_sine_waveform[phase] = sinf(2 * PI * (float) phase / 64.0f);
        pt_ramp_waveform[phase] = 1.0f - (float) phase / 32.0f;
        pt_square_waveform[phase] = phase < 32 ? 1.0f : -1.0f;
    }
}

float lfo_pt_waveform(const pt_waveform_t type, const unsigned int phase)
{
    switch (type)
    {
        case PT_WAVEFORM_SINE: return pt_sine_waveform[phase % 64];
        case PT_WAVEFORM_RAMP: return pt_ramp_waveform[phase % 64];
        default: return pt_square_waveform[phase % 64];
    }
}