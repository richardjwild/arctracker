#ifndef ARCTRACKER_ENGINE_LFO_H
#define ARCTRACKER_ENGINE_LFO_H

#include <stdbool.h>
#include <stdint.h>

#define PT_LFO_WAVELENGTH 64

typedef enum { PT_WAVEFORM_SINE, PT_WAVEFORM_RAMP, PT_WAVEFORM_SQUARE } pt_waveform_t;

typedef struct {
    bool enabled;
    uint8_t rate;
    uint8_t depth;
    bool retrigger;
    pt_waveform_t waveform;
    uint8_t phase;
} lfo_effect_t;

void lfo_init_waveforms(void);

float lfo_pt_waveform(pt_waveform_t type, unsigned int phase);

#endif //ARCTRACKER_ENGINE_LFO_H
