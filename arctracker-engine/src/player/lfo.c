#include "lfo.h"
#include <math.h>

static float pt_sine_waveform[PT_LFO_WAVELENGTH];
static float pt_ramp_waveform[PT_LFO_WAVELENGTH];
static float pt_square_waveform[PT_LFO_WAVELENGTH];
static const float PI = 3.14159265358979323846f;

void lfo_init_waveforms(void)
{
    for (int phase = 0; phase < PT_LFO_WAVELENGTH; phase++)
    {
        pt_sine_waveform[phase] = sinf(2 * PI * (float) phase / PT_LFO_WAVELENGTH);
        pt_ramp_waveform[phase] = (float) phase / (PT_LFO_WAVELENGTH / 2) - 1.0f;
        pt_square_waveform[phase] = phase < PT_LFO_WAVELENGTH / 2 ? 1.0f : -1.0f;
    }
}

float lfo_pt_waveform(const pt_waveform_t type, const unsigned int phase)
{
    switch (type)
    {
        case PT_WAVEFORM_SINE: return pt_sine_waveform[phase % PT_LFO_WAVELENGTH];
        case PT_WAVEFORM_RAMP: return pt_ramp_waveform[phase % PT_LFO_WAVELENGTH];
        default: return pt_square_waveform[phase % PT_LFO_WAVELENGTH];
    }
}