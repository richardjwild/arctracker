#ifndef ARCTRACKER_MU_LAW_H
#define ARCTRACKER_MU_LAW_H

#include <arctracker.h>

void precalculate_mu_law();

float *convert_mu_law_to_linear_pcm(const __uint8_t *mu_law_encoded, int no_samples);

float convert_to_linear_gain(const int logarithmic_gain);

#endif // ARCTRACKER_MU_LAW_H