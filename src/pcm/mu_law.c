#include "mu_law.h"
#include "../memory/bits.h"
#include "../memory/heap.h"

static int vidc_pcm_encoding[256];
static float gain_conversion[256];
static bool encoding_done = false;
const __uint16_t MULAW_BIAS = 33;
const float EXPANDED_MAX = 8097.0;
const float GAIN_MAX = 8031.0;

int expand_mu_law(__int8_t mu_law);

void calculate_logarithmic_gain()
{
    for (int i = 0; i <= 127; i++)
    {
        gain_conversion[(i * 2) + 1] = expand_mu_law(255 - i) / GAIN_MAX;
        if (i >= 1)
            gain_conversion[i * 2] = (gain_conversion[(i * 2) - 1] + gain_conversion[(i * 2) + 1]) / 2;
    }
    gain_conversion[0] = 0.0;
    gain_conversion[1] = gain_conversion[2] / 2;
}

void calculate_pcm_encoding()
{
    if (!encoding_done)
    {
        for (int i = 0; i <= 127; i++)
        {
            vidc_pcm_encoding[i * 2] = expand_mu_law(127 - i);
            vidc_pcm_encoding[(i * 2) + 1] = expand_mu_law(127 - i) * -1;
        }
        encoding_done = true;
        calculate_logarithmic_gain();
    }
}

int expand_mu_law(__int8_t mu_law)
{
    __int8_t number = ~mu_law;
    int sign = (number & 0x80) ? -1 : 1;
    if (sign == -1)
    {
        number &= ~(1 << 7);
    }
    int position = HIGH_NYBBLE(number) + 5;
    return sign * ((1 << position) | (LOW_NYBBLE(number) << (position - 4))
                   | (1 << (position - 5))) - MULAW_BIAS;
}

float *convert_mu_law_to_linear_pcm(const __uint8_t *mu_law_encoded, const int no_samples)
{
    calculate_pcm_encoding();
    float *linear = allocate_array(no_samples, sizeof(float));
    for (int i = 0; i < no_samples; i++)
    {
        __uint8_t encoded = mu_law_encoded[i];
        linear[i] = vidc_pcm_encoding[encoded] / EXPANDED_MAX;
    }
    return linear;
}

float convert_to_linear_gain(const int logarithmic_gain)
{
    return gain_conversion[logarithmic_gain];
}