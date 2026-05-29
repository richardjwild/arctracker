#include "mu_law.h"
#include "memory/heap.h"

const int BIAS = 0x84;
const unsigned int SIGN_BIT = 0x80;
const unsigned int QUANTIZATION_BITS_MASK = 0xf;
const unsigned int SEGMENT_NUMBER_MASK = 0x70;
const float EXPANDED_MAX = 32124.0f;

static float *encoding = NULL;

static void calculate_vidc_encoding(float *encoding);

float mu_law_to_linear(int8_t mu_law)
{
    int normal_mu_law = ~mu_law;
    int biased_quantization_bits = ((normal_mu_law & QUANTIZATION_BITS_MASK) << 3) + BIAS;
    unsigned int segment_number = ((unsigned) normal_mu_law & SEGMENT_NUMBER_MASK) >> 4;
    int linear = normal_mu_law & SIGN_BIT
            ? (BIAS - (biased_quantization_bits << segment_number))
            : ((biased_quantization_bits << segment_number) - BIAS);
    return linear / EXPANDED_MAX;
}

bool convert_vidc_encoded_sample(float *linear, const uint8_t *vidc_encoded_sample, const int no_samples)
{
    if (encoding == NULL)
    {
        encoding = allocate_array(MODULE, 256, sizeof(float));
        if (encoding == NULL)
            return false;
        calculate_vidc_encoding(encoding);
    }
    for (int i = 0; i < no_samples; i++)
    {
        uint8_t encoded = vidc_encoded_sample[i];
        linear[i] = encoding[encoded];
    }
    // Set the final samples to zero so that the interpolation will be done correctly.
    linear[no_samples] = 0.0f;
    linear[no_samples + 1] = 0.0f;
    return true;
}

static void calculate_vidc_encoding(float *encoding)
{
    for (int i = 0; i <= 127; i++)
    {
        encoding[i * 2] = mu_law_to_linear(127 - i);
        encoding[(i * 2) + 1] = mu_law_to_linear(127 - i) * -1;
    }
}

void destroy_encoding_buffer(void)
{
    if (encoding != NULL) deallocate(MODULE, encoding);
    encoding = NULL;
}
