#include "vidc.h"
#include <assert.h>

const int BIAS = 0x84;
const unsigned int QUANTIZATION_BITS_MASK = 0xf;
const unsigned int SEGMENT_NUMBER_MASK = 0x70;
const float EXPANDED_MAX = 32124.0f;

static float mu_law_to_linear(int);

float vidc_to_linear(const int vidc_encoded)
{
    const float linear = mu_law_to_linear(127 - vidc_encoded / 2);
    return vidc_encoded & 1 ? linear * -1.0f : linear;
}

static float mu_law_to_linear(const int mu_law)
{
    //
    // We ignore the usual mu_law sign bit (0x80) because VIDC used bit 0 for this instead and it is already handled.
    //
    assert(mu_law >= 0 && mu_law <= 127);
    const int normal_mu_law = ~mu_law;
    const int biased_quantization_bits = (int) ((normal_mu_law & QUANTIZATION_BITS_MASK) << 3) + BIAS;
    const unsigned int segment_number = ((unsigned) normal_mu_law & SEGMENT_NUMBER_MASK) >> 4;
    const int linear = (biased_quantization_bits << segment_number) - BIAS;
    return (float) linear / EXPANDED_MAX;
}
