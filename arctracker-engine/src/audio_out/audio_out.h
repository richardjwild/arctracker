#ifndef ARCTRACKER_WRITE_AUDIO_H
#define ARCTRACKER_WRITE_AUDIO_H

#include <stdatomic.h>
#include "audio_channel.h"
#include "audio_api/audio_api.h"

#define SAMPLE_RATE 44100
#define AUDIO_BUFFER_SIZE_FRAMES 512

typedef struct {
    int num_channels;
    float master_gain;
    float *mono_channel_buffer;
    stereo_frame_t *stereo_channel_buffer;
    atomic_uint peak_l;
    atomic_uint peak_r;
    unsigned int frames_until_next_tick;
} audio_out_t;

typedef struct {
    bool success;
    const char *error_message;
} audio_out_result_t;

bool initialise_audio(audio_out_t *audio_out, int num_channels, float master_gain);

void write_audio_data(audio_out_t *, audio_channel_t *, unsigned int, float *);

void destroy_audio_resources(const audio_out_t *);

#endif // ARCTRACKER_WRITE_AUDIO_H
