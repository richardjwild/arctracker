#include "fraction.h"
#include <assert.h>

fraction_t fraction_create(int numerator, const int denominator)
{
    assert(numerator >= 0);
    assert(denominator > 0);
    int whole_part = 0;
    while (numerator >= denominator)
    {
        numerator -= denominator;
        whole_part += 1;
    }
    return (fraction_t) {
        .numerator = numerator,
        .denominator = denominator,
        .whole_part = whole_part,
    };
}

fraction_t fraction_accumulate(const fraction_t fraction, const int numerator_to_add)
{
    assert(numerator_to_add >= 0);
    int numerator = fraction.numerator + numerator_to_add;
    int whole_part = fraction.whole_part;
    while (numerator >= fraction.denominator)
    {
        numerator -= fraction.denominator;
        whole_part += 1;
    }
    return (fraction_t) {
        .numerator = numerator,
        .denominator = fraction.denominator,
        .whole_part = whole_part,
    };
}

int fraction_whole_part(const fraction_t fraction)
{
    return fraction.whole_part;
}

int fraction_remainder(const fraction_t fraction)
{
    return fraction.numerator;
}
