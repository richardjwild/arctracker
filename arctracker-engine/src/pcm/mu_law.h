#ifndef ARCTRACKER_MU_LAW_H
#define ARCTRACKER_MU_LAW_H

#include <stdbool.h>
#include <stdint.h>

float mu_law_to_linear(int8_t mu_law);

bool convert_vidc_encoded_sample(float *linear, const uint8_t *vidc_encoded_sample, int no_samples);

void destroy_encoding_buffer(void);

#endif // ARCTRACKER_MU_LAW_H
