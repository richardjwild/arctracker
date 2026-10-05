#include "fraction.h"

fraction_t fraction_create(const int numerator, const int denominator)
{
    return (fraction_t) {
        .numerator = numerator,
        .denominator = denominator,
    };
}

fraction_t fraction_add_numerator(const fraction_t fraction, const int numerator_to_add)
{
    return (fraction_t) {
        .numerator = fraction.numerator + numerator_to_add,
        .denominator = fraction.denominator
    };
}

int whole_part(const fraction_t fraction)
{
    return fraction.numerator / fraction.denominator;
}

fraction_t fractional_part(const fraction_t fraction)
{
    return fraction_create(fraction.numerator % fraction.denominator, fraction.denominator);
}

fraction_t fraction_subtract(const fraction_t fraction, const int whole_to_subtract)
{
    return (fraction_t) {
        .numerator = fraction.numerator - whole_to_subtract * fraction.denominator,
        .denominator = fraction.denominator
    };
}
