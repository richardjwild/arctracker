#include "arctracker.h"

#ifndef ARCTRACKER_PLAY_MOD_H
#define ARCTRACKER_PLAY_MOD_H

typedef struct {
    double phase_accumulator;
    int period;
    int target_period;
    bool sample_repeats;
    long sample_length;
    long repeat_length;
    void *sample_pointer;
    unsigned char gain;
    bool channel_playing;
    int panning;
    unsigned char arpeggio_counter;
    unsigned char last_data_byte;
    unsigned char note_currently_playing;
} channel_info;

return_status play_module(
        mod_details *p_module,
        sample_details *samples,
        audio_api_t audio_api,
        unsigned int *p_periods,
        program_arguments *p_args);

void stop_playback();

#endif //ARCTRACKER_PLAY_MOD_H
