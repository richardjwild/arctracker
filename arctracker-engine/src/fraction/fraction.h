#ifndef ARCTRACKER_ENGINE_FRACTION_H
#define ARCTRACKER_ENGINE_FRACTION_H

typedef struct {
    int numerator;
    int denominator;
} fraction_t;

fraction_t fraction_create(int numerator, int denominator);

fraction_t fraction_add_numerator(fraction_t fraction, int numerator_to_add);

int whole_part(fraction_t fraction);

fraction_t fractional_part(fraction_t fraction);

fraction_t fraction_subtract(fraction_t fraction, int whole_to_subtract);

#endif //ARCTRACKER_ENGINE_FRACTION_H
