#include "vidc.h"
#include <assert.h>

const int BIAS = 0x84;
const unsigned int QUANTIZATION_BITS_MASK = 0xf;
const unsigned int SEGMENT_NUMBER_MASK = 0x70;
const float EXPANDED_MAX = 32124.0f;

static float mu_law_to_linear(int);

float vidc_to_linear(const int vidc_encoded)
{
    const float linear = mu_law_to_linear(vidc_encoded / 2);
    return vidc_encoded & 1 ? linear * -1.0f : linear;
}

static float mu_law_to_linear(const int mu_law)
{
    //
    // VIDC represented sample magnitude using seven logarithmic bits. Its exponential DAC consisted of a 3-bit
    // exponential "chord" selector and a 4-bit linear DAC selecting one of 16 positions within that chord. This
    // approximated the Bell Labs mu-255 law and is structurally very similar to G.711 mu-law.
    //
    // This function therefore adapts the 7-bit VIDC magnitude to the magnitude-expansion part of a conventional
    // G.711 mu-law decoder. The sign is not handled here: G.711 normally uses bit 7 for sign, whereas VIDC uses
    // bit 0, which has already been removed and handled by the caller.
    //
    // The mu-law magnitude is interpreted as:
    //   0EEEMMMM
    // where:
    //   EEE  is the exponent (segment/chord number)
    //   MMMM is the mantissa (quantisation bits)
    //
    // Each exponent selects a straight-line segment whose step size is proportional to 2^EEE. The segments must
    // occupy successively higher ranges rather than all beginning at zero. After scaling the mantissa, a bias is
    // therefore added before applying the exponent shift. Because the bias is shifted along with the mantissa,
    // it supplies the increasing offset for successive segments. The bias is subtracted again afterwards so that
    // the complete transfer function begins at zero.
    //
    // 0x84 is the standard G.711 mu-law bias. It is currently assumed, but not proven, that this gives the correct
    // chord boundaries for VIDC's DAC.
    //
    assert(mu_law >= 0 && mu_law <= 127);
    const int biased_quantization_bits = (int) ((mu_law & QUANTIZATION_BITS_MASK) << 3) + BIAS;
    const unsigned int segment_number = ((unsigned) mu_law & SEGMENT_NUMBER_MASK) >> 4;
    const int linear = (biased_quantization_bits << segment_number) - BIAS;
    return (float) linear / EXPANDED_MAX;
}
