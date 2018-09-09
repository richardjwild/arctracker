#ifndef ARCTRACKER_MU_LAW_H
#define ARCTRACKER_MU_LAW_H

#include <stdint.h>

double mu_law_to_linear(int8_t mu_law);

double *convert_vidc_encoded_sample(const uint8_t *mu_law_encoded, const int no_samples);

#endif // ARCTRACKER_MU_LAW_H