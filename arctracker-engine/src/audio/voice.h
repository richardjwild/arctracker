#ifndef ARCTRACKER_VOICE_H
#define ARCTRACKER_VOICE_H

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    float phase_accumulator;
    int period;
    int tone_portamento_target_period;
    bool sample_repeats;
    int sample_end;
    int repeat_length;
    float *sample_pointer;
    uint8_t volume;
    bool channel_playing;
    uint8_t panning;
    bool arpeggiator_on;
    int arpeggio_counter;
    int current_note;
    uint8_t effect_memory[4];
} voice_t;

#endif //ARCTRACKER_VOICE_H
