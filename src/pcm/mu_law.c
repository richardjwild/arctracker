#include "mu_law.h"
#include <memory/bits.h>
#include <memory/heap.h>

const int BIAS = 0x84;
const unsigned int SIGN_BIT = 0x80;
const unsigned int QUANTIZATION_BITS_MASK = 0xf;
const unsigned int SEGMENT_NUMBER_MASK = 0x70;
const double EXPANDED_MAX = 32124.0;

double mu_law_to_linear(int8_t mu_law)
{
    int normal_mu_law = ~mu_law;
    int biased_quantization_bits = ((normal_mu_law & QUANTIZATION_BITS_MASK) << 3) + BIAS;
    unsigned int segment_number = ((unsigned) normal_mu_law & SEGMENT_NUMBER_MASK) >> 4;
    int linear = normal_mu_law & SIGN_BIT
            ? (BIAS - (biased_quantization_bits << segment_number))
            : ((biased_quantization_bits << segment_number) - BIAS);
    return linear / EXPANDED_MAX;
}

void calculate_vidc_encoding(double *encoding)
{
    for (int i = 0; i <= 127; i++)
    {
        encoding[i * 2] = mu_law_to_linear(127 - i);
        encoding[(i * 2) + 1] = mu_law_to_linear(127 - i) * -1;
    }
}

double *convert_vidc_encoded_sample(const uint8_t *mu_law_encoded, const int no_samples)
{
    // extend by 1 sample so that we don't have to worry about falling off
    // the end when interpolating between two samples
    double *linear = allocate_array(no_samples + 1, sizeof(double));
    double encoding[256];
    calculate_vidc_encoding(encoding);
    for (int i = 0; i < no_samples; i++)
    {
        uint8_t encoded = mu_law_encoded[i];
        linear[i] = encoding[encoded];
    }
    // set the final sample to zero so that it will be interpolated correctly
    linear[no_samples] = 0.0;
    return linear;
}
