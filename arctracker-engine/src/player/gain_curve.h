#ifndef ARCTRACKER_ENGINE_GAIN_CURVE_H
#define ARCTRACKER_ENGINE_GAIN_CURVE_H

#include "audio_out/volume_mapping_type.h"

void calculate_gain_curve(float *gain_curve, volume_mapping_type_t volume_mapping);

#endif //ARCTRACKER_ENGINE_GAIN_CURVE_H
