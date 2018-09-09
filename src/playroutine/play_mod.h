#ifndef ARCTRACKER_PLAY_MOD_H
#define ARCTRACKER_PLAY_MOD_H

#include <arctracker.h>
#include <audio_api/api.h>

#define MAX_EFFECTS 4

typedef struct {
    double phase_accumulator;
    int period;
    int tone_portamento_target_period;
    bool sample_repeats;
    long sample_end;
    long repeat_length;
    double *sample_pointer;
    int gain;
    bool channel_playing;
    int panning;
    bool arpeggiator_on;
    int arpeggio_counter;
    int current_note;
    uint8_t effect_memory[MAX_EFFECTS];
} voice_t;

void play_module(module_t *module, audio_api_t audio_api);

#endif //ARCTRACKER_PLAY_MOD_H
