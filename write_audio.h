#include "play_mod.h"

#ifndef ARCTRACKER_WRITE_AUDIO_H
#define ARCTRACKER_WRITE_AUDIO_H

static long *channel_buffer;
static audio_api_t audio_output;
static long frames_filled;
static int channels;
static int channel_buffer_stride_length;
static int master_gain;

void initialise_audio(audio_api_t audio_output, long channels, int master_gain);

void write_audio_data(channel_info *voices, long frames_requested);

#endif // ARCTRACKER_WRITE_AUDIO_H