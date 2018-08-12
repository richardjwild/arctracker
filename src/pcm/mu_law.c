#include "mu_law.h"
#include <memory/bits.h>
#include <memory/heap.h>

static int vidc_pcm_encoding[256];
static float gain_conversion[256];

static bool encoding_done = false;

const float EXPANDED_MAX = 32124.0;
const float GAIN_MAX = 8031.0;
const int BIAS = 0x84;
const unsigned int SIGN_BIT = 0x80;
const unsigned int QUANTIZATION_BITS_MASK = 0xf;
const unsigned int SEGMENT_SHIFT = 4;
const unsigned int SEGMENT_NUMBER_MASK = 0x70;

int mu_law_to_linear(__int8_t mu_law)
{
    int normal_mu_law = ~mu_law;
    int biased_quantization_bits = ((normal_mu_law & QUANTIZATION_BITS_MASK) << 3) + BIAS;
    unsigned int segment_number = ((unsigned) normal_mu_law & SEGMENT_NUMBER_MASK) >> SEGMENT_SHIFT;
    return normal_mu_law & SIGN_BIT
            ? (BIAS - (biased_quantization_bits << segment_number))
            : ((biased_quantization_bits << segment_number) - BIAS);
}

void calculate_logarithmic_gain()
{
    for (int i = 0; i <= 127; i++)
    {
        gain_conversion[(i * 2) + 1] = mu_law_to_linear(255 - i) / GAIN_MAX;
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
            vidc_pcm_encoding[i * 2] = mu_law_to_linear(127 - i);
            vidc_pcm_encoding[(i * 2) + 1] = mu_law_to_linear(127 - i) * -1;
        }
        encoding_done = true;
        calculate_logarithmic_gain();
    }
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