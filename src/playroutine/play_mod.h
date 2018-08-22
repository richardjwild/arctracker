#ifndef ARCTRACKER_PLAY_MOD_H
#define ARCTRACKER_PLAY_MOD_H

#include <arctracker.h>
#include <audio/audio_api.h>

typedef struct {
    float phase_accumulator;
    int period;
    int tone_portamento_target_period;
    int tone_portamento_speed;
    bool sample_repeats;
    long sample_end;
    long repeat_length;
    float *sample_pointer;
    int gain;
    bool channel_playing;
    int panning;
    bool arpeggiator_on;
    int arpeggio_counter;
    int note_playing;
} voice_t;

void play_module(module_t *module, audio_api_t audio_api);

#endif //ARCTRACKER_PLAY_MOD_H
