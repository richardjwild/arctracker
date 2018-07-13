#ifndef ARCTRACKER_PLAY_MOD_H
#define ARCTRACKER_PLAY_MOD_H

#include "arctracker.h"
#include "audio_api.h"

typedef struct {
    double phase_accumulator;
    int period;
    int target_period;
    bool sample_repeats;
    long sample_end;
    long repeat_length;
    void *sample_pointer;
    int gain;
    bool channel_playing;
    int panning;
    unsigned char arpeggio_counter;
    __uint8_t last_data_byte;
    int note_currently_playing;
} voice_t;

void play_module(module_t *module, audio_api_t audio_api);

#endif //ARCTRACKER_PLAY_MOD_H
