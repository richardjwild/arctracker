#include "arctracker.h"

#ifndef ARCTRACKER_PLAY_MOD_H
#define ARCTRACKER_PLAY_MOD_H

typedef struct {
    double phase_accumulator;
    int period;
    int target_period;
    bool sample_repeats;
    long sample_end;
    long repeat_length;
    void *sample_pointer;
    unsigned char gain;
    bool channel_playing;
    int panning;
    unsigned char arpeggio_counter;
    unsigned char last_data_byte;
    unsigned char note_currently_playing;
} voice_t;

void play_module(const module_t *p_module, audio_api_t audio_api);

#endif //ARCTRACKER_PLAY_MOD_H
