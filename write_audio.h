#include "play_mod.h"

#ifndef ARCTRACKER_WRITE_AUDIO_H
#define ARCTRACKER_WRITE_AUDIO_H

static long *channel_buffer;
static long frames_filled;
static int channels;
static int channel_buffer_stride_length;
static int master_gain;

void initialise_audio(audio_api_t audio_api, long channels, int volume);

void write_audio_data(
        audio_api_t audio_api,
        channel_info *voice,
        long p_nframes);

#endif // ARCTRACKER_WRITE_AUDIO_H