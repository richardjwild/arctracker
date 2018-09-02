#ifndef ARCTRACKER_MU_LAW_H
#define ARCTRACKER_MU_LAW_H

#include <stdint.h>

void precalculate_mu_law();

double *convert_mu_law_to_linear_pcm(const __uint8_t *mu_law_encoded, int no_samples);

double convert_to_linear_gain(int logarithmic_gain);

#endif // ARCTRACKER_MU_LAW_H