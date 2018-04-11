#include "play_mod.h"

#ifndef ARCTRACKER_WRITE_AUDIO_H
#define ARCTRACKER_WRITE_AUDIO_H

static long *channel_buffer;

void initialise_audio(audio_api_t audio_api, long channels, long p_sample_rate);

void write_audio_data(
        audio_api_t audio_api,
        channel_info *voice,
        int channels,
        unsigned char master_gain,
        long p_nframes);

#endif // ARCTRACKER_WRITE_AUDIO_H