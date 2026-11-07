#ifndef ARCTRACKER_ENGINE_FRACTION_H
#define ARCTRACKER_ENGINE_FRACTION_H

typedef struct {
    int numerator;
    int denominator;
    int whole_part;
} fraction_t;

fraction_t fraction_create(int numerator, int denominator);

fraction_t fraction_accumulate(fraction_t fraction, int numerator_to_add);

int fraction_whole_part(fraction_t fraction);

int fraction_remainder(fraction_t fraction);

#endif //ARCTRACKER_ENGINE_FRACTION_H
