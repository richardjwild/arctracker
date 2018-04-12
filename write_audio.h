#include "play_mod.h"

#ifndef ARCTRACKER_WRITE_AUDIO_H
#define ARCTRACKER_WRITE_AUDIO_H

static long *channel_buffer;
static long frames_filled;
int channels;
static int channel_buffer_stride_length;

void initialise_audio(audio_api_t audio_api, long channels);

void write_audio_data(
        audio_api_t audio_api,
        channel_info *voice,
        unsigned char master_gain,
        long p_nframes);

#endif // ARCTRACKER_WRITE_AUDIO_H