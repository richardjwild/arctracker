#include "mu_law.h"
#include "../memory/bits.h"
#include "../memory/heap.h"

static int vidc_pcm_encoding[256];
static bool encoding_done = false;
const __uint16_t MULAW_BIAS = 33;
const float EXPANDED_MAX = 8097.0;

int expand_mu_law(__int8_t mu_law);

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
